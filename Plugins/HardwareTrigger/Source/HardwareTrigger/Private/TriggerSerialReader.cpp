#include "TriggerSerialReader.h"
#include "HAL/RunnableThread.h"
#include "HAL/PlatformProcess.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#endif

FTriggerSerialReader::FTriggerSerialReader()
    : Thread(nullptr)
#if PLATFORM_WINDOWS
    , SerialHandle(INVALID_HANDLE_VALUE)
#else
    , SerialFileDescriptor(-1)
#endif
{
    bRunning = false;
    bConnected = false;
    bTriggerPressedFlag = false;
}

FTriggerSerialReader::~FTriggerSerialReader()
{
    Stop();
}

bool FTriggerSerialReader::Start(const FString& PortName, int32 BaudRate)
{
    if (bRunning) return false;

    if (!OpenSerialPort(PortName, BaudRate))
    {
        UE_LOG(LogTemp, Error, TEXT("Trigger: Failed to open serial port %s"), *PortName);
        return false;
    }

    bConnected = true;
    bRunning = true;

    Thread = FRunnableThread::Create(this, TEXT("TriggerReaderThread"), 0, TPri_AboveNormal);

    UE_LOG(LogTemp, Log, TEXT("Trigger: Reader started on %s @ %d baud"), *PortName, BaudRate);
    return true;
}

void FTriggerSerialReader::Stop()
{
    bRunning = false;

    if (Thread)
    {
        Thread->WaitForCompletion();
        delete Thread;
        Thread = nullptr;
    }

    CloseSerialPort();
    bConnected = false;
}

bool FTriggerSerialReader::ConsumeTriggerPress()
{
    FScopeLock Lock(&TriggerMutex);
    if (bTriggerPressedFlag)
    {
        bTriggerPressedFlag = false;
        return true;
    }
    return false;
}

bool FTriggerSerialReader::IsTriggerPressed() const
{
    FScopeLock Lock(&TriggerMutex);
    return bTriggerPressedFlag;
}

bool FTriggerSerialReader::IsConnected() const
{
    return bConnected;
}

bool FTriggerSerialReader::Init()
{
    return true;
}

uint32 FTriggerSerialReader::Run()
{
    uint8 Buffer[64];

    while (bRunning)
    {
        int32 BytesRead = ReadSerial(Buffer, sizeof(Buffer));

        if (BytesRead > 0)
        {
            for (int32 i = 0; i < BytesRead; i++)
            {
                // Arduino sends 'F' when trigger is pulled
                if (Buffer[i] == 'F')
                {
                    FScopeLock Lock(&TriggerMutex);
                    bTriggerPressedFlag = true;
                }
            }
        }
        else
        {
            FPlatformProcess::Sleep(0.005f);
        }
    }

    return 0;
}

void FTriggerSerialReader::Exit()
{
}

// ============================================================
// Platform-specific serial port (same pattern as VN100 reader)
// ============================================================

bool FTriggerSerialReader::OpenSerialPort(const FString& PortName, int32 BaudRate)
{
#if PLATFORM_WINDOWS
    FString WinPort = FString::Printf(TEXT("\\\\.\\%s"), *PortName);

    SerialHandle = CreateFileW(
        *WinPort, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (SerialHandle == INVALID_HANDLE_VALUE) return false;

    DCB dcb = {};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(SerialHandle, &dcb))
    {
        UE_LOG(LogTemp, Error, TEXT("Trigger: GetCommState failed"));
        CloseHandle(SerialHandle);
        SerialHandle = INVALID_HANDLE_VALUE;
        return false;
    }
    dcb.BaudRate = BaudRate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    if (!SetCommState(SerialHandle, &dcb))
    {
        UE_LOG(LogTemp, Error, TEXT("Trigger: SetCommState failed"));
        CloseHandle(SerialHandle);
        SerialHandle = INVALID_HANDLE_VALUE;
        return false;
    }

    COMMTIMEOUTS timeouts = {};
    timeouts.ReadIntervalTimeout = 10;
    timeouts.ReadTotalTimeoutConstant = 50;
    if (!SetCommTimeouts(SerialHandle, &timeouts))
    {
        UE_LOG(LogTemp, Warning, TEXT("Trigger: SetCommTimeouts failed, continuing anyway"));
    }

    return true;
#else
    SerialFileDescriptor = open(TCHAR_TO_UTF8(*PortName), O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (SerialFileDescriptor < 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Trigger: POSIX open failed: %d"), errno);
        return false;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(SerialFileDescriptor, &tty) != 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Trigger: tcgetattr failed: %d"), errno);
        close(SerialFileDescriptor);
        SerialFileDescriptor = -1;
        return false;
    }

    speed_t speed = B9600; // Arduino default
    if (BaudRate == 115200) speed = B115200;
    else if (BaudRate == 57600) speed = B57600;
    else if (BaudRate == 38400) speed = B38400;
    else if (BaudRate == 19200) speed = B19200;

    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_iflag = IGNPAR;
    tty.c_oflag = 0;
    tty.c_lflag = 0;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(SerialFileDescriptor, TCSANOW, &tty) != 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Trigger: tcsetattr failed: %d"), errno);
        close(SerialFileDescriptor);
        SerialFileDescriptor = -1;
        return false;
    }
    return true;
#endif
}

void FTriggerSerialReader::CloseSerialPort()
{
#if PLATFORM_WINDOWS
    if (SerialHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(SerialHandle);
        SerialHandle = INVALID_HANDLE_VALUE;
    }
#else
    if (SerialFileDescriptor >= 0)
    {
        close(SerialFileDescriptor);
        SerialFileDescriptor = -1;
    }
#endif
}

int32 FTriggerSerialReader::ReadSerial(uint8* Buffer, int32 MaxBytes)
{
#if PLATFORM_WINDOWS
    DWORD BytesRead = 0;
    if (SerialHandle != INVALID_HANDLE_VALUE)
    {
        ReadFile(SerialHandle, Buffer, MaxBytes, &BytesRead, NULL);
    }
    return (int32)BytesRead;
#else
    if (SerialFileDescriptor < 0) return 0;
    ssize_t BytesRead = read(SerialFileDescriptor, Buffer, MaxBytes);
    return (BytesRead > 0) ? (int32)BytesRead : 0;
#endif
}
