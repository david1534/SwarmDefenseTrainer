#include "VN100OrientationReader.h"
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

FVN100OrientationReader::FVN100OrientationReader()
    : Thread(nullptr)
    , CurrentOrientation(FRotator::ZeroRotator)
#if PLATFORM_WINDOWS
    , SerialHandle(INVALID_HANDLE_VALUE)
#else
    , SerialFileDescriptor(-1)
#endif
{
    bRunning = false;
    bConnected = false;
}

FVN100OrientationReader::~FVN100OrientationReader()
{
    Stop();
}

bool FVN100OrientationReader::Start(const FString& PortName, int32 BaudRate)
{
    if (bRunning) return false;

    if (!OpenSerialPort(PortName, BaudRate))
    {
        UE_LOG(LogTemp, Error, TEXT("VN100: Failed to open serial port %s"), *PortName);
        return false;
    }

    bConnected = true;
    bRunning = true;

    Thread = FRunnableThread::Create(this, TEXT("VN100ReaderThread"), 0, TPri_AboveNormal);

    UE_LOG(LogTemp, Log, TEXT("VN100: Reader started on %s @ %d baud"), *PortName, BaudRate);
    return true;
}

void FVN100OrientationReader::Stop()
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

FRotator FVN100OrientationReader::GetOrientation() const
{
    FScopeLock Lock(&OrientationMutex);
    return CurrentOrientation;
}

bool FVN100OrientationReader::IsConnected() const
{
    return bConnected;
}

bool FVN100OrientationReader::Init()
{
    return true;
}

uint32 FVN100OrientationReader::Run()
{
    uint8 Buffer[256];

    while (bRunning)
    {
        int32 BytesRead = ReadSerial(Buffer, sizeof(Buffer) - 1);

        if (BytesRead > 0)
        {
            Buffer[BytesRead] = 0;
            LineBuffer += UTF8_TO_TCHAR((char*)Buffer);

            // Guard against unbounded growth from malformed data without newlines
            if (LineBuffer.Len() > 4096)
            {
                UE_LOG(LogTemp, Warning, TEXT("VN100: LineBuffer exceeded 4KB without newline, flushing"));
                LineBuffer.Empty();
                continue;
            }

            // Process all complete lines in the buffer
            int32 NewlineIdx;
            while (LineBuffer.FindChar('\n', NewlineIdx))
            {
                FString Line = LineBuffer.Left(NewlineIdx).TrimEnd();
                LineBuffer.RightChopInline(NewlineIdx + 1);

                // Parse $VNYPR sentence
                float Yaw, Pitch, Roll;
                if (ParseVNYPR(Line, Yaw, Pitch, Roll))
                {
                    FScopeLock Lock(&OrientationMutex);
                    CurrentOrientation = FRotator(Pitch, Yaw, Roll);
                }
            }
        }
        else
        {
            // No data available — sleep briefly to avoid busy-waiting
            FPlatformProcess::Sleep(0.001f);
        }
    }

    return 0;
}

void FVN100OrientationReader::Exit()
{
    // Cleanup is handled in Stop()
}

bool FVN100OrientationReader::ParseVNYPR(const FString& Line, float& OutYaw, float& OutPitch, float& OutRoll)
{
    // Expected format: $VNYPR,yaw,pitch,roll*checksum
    if (!Line.StartsWith(TEXT("$VNYPR,")))
    {
        return false;
    }

    // Validate checksum if present: XOR of all bytes between '$' and '*'
    FString Data = Line.Mid(7);
    int32 ChecksumIdx;
    if (Data.FindChar('*', ChecksumIdx))
    {
        // Compute expected checksum (XOR of bytes between '$' and '*')
        FString BodyForChecksum = Line.Mid(1, Line.Find(TEXT("*")) - 1);
        uint8 Computed = 0;
        for (int32 i = 0; i < BodyForChecksum.Len(); i++)
        {
            Computed ^= (uint8)BodyForChecksum[i];
        }

        // Parse the two-hex-digit checksum after '*'
        FString HexStr = Data.Mid(ChecksumIdx + 1, 2);
        uint8 Received = (uint8)FCString::Strtoi(*HexStr, nullptr, 16);

        if (Computed != Received)
        {
            return false; // Corrupt sentence
        }

        Data.LeftInline(ChecksumIdx);
    }

    // Split by comma: yaw, pitch, roll
    TArray<FString> Parts;
    Data.ParseIntoArray(Parts, TEXT(","), false);

    if (Parts.Num() >= 3)
    {
        OutYaw = FCString::Atof(*Parts[0]);
        OutPitch = FCString::Atof(*Parts[1]);
        OutRoll = FCString::Atof(*Parts[2]);
        return true;
    }

    return false;
}

// ============================================================
// Platform-specific serial port implementation
// ============================================================

bool FVN100OrientationReader::OpenSerialPort(const FString& PortName, int32 BaudRate)
{
#if PLATFORM_WINDOWS
    // Windows: open COM port using Win32 API
    FString WinPort = FString::Printf(TEXT("\\\\.\\%s"), *PortName);

    SerialHandle = CreateFileW(
        *WinPort, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (SerialHandle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    DCB dcb = {};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(SerialHandle, &dcb))
    {
        UE_LOG(LogTemp, Error, TEXT("VN100: GetCommState failed"));
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
        UE_LOG(LogTemp, Error, TEXT("VN100: SetCommState failed"));
        CloseHandle(SerialHandle);
        SerialHandle = INVALID_HANDLE_VALUE;
        return false;
    }

    COMMTIMEOUTS timeouts = {};
    timeouts.ReadIntervalTimeout = 10;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 50;
    if (!SetCommTimeouts(SerialHandle, &timeouts))
    {
        UE_LOG(LogTemp, Warning, TEXT("VN100: SetCommTimeouts failed, continuing anyway"));
    }

    return true;

#else
    // macOS / Linux: open serial port using POSIX API
    FString PosixPort = PortName;

    SerialFileDescriptor = open(TCHAR_TO_UTF8(*PosixPort), O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (SerialFileDescriptor < 0)
    {
        UE_LOG(LogTemp, Error, TEXT("VN100: POSIX open failed: %d"), errno);
        return false;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(SerialFileDescriptor, &tty) != 0)
    {
        UE_LOG(LogTemp, Error, TEXT("VN100: tcgetattr failed: %d"), errno);
        close(SerialFileDescriptor);
        SerialFileDescriptor = -1;
        return false;
    }

    // Map baud rate
    speed_t speed = B115200;
    if (BaudRate == 9600) speed = B9600;
    else if (BaudRate == 19200) speed = B19200;
    else if (BaudRate == 38400) speed = B38400;
    else if (BaudRate == 57600) speed = B57600;
    else if (BaudRate == 115200) speed = B115200;

    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit
    tty.c_cflag &= ~PARENB;                      // no parity
    tty.c_cflag &= ~CSTOPB;                      // 1 stop bit
    tty.c_cflag |= CLOCAL | CREAD;               // enable read
    tty.c_iflag = IGNPAR;                         // ignore parity errors
    tty.c_oflag = 0;
    tty.c_lflag = 0;                              // raw mode

    tty.c_cc[VMIN] = 0;                           // non-blocking
    tty.c_cc[VTIME] = 1;                          // 100ms timeout

    if (tcsetattr(SerialFileDescriptor, TCSANOW, &tty) != 0)
    {
        UE_LOG(LogTemp, Error, TEXT("VN100: tcsetattr failed: %d"), errno);
        close(SerialFileDescriptor);
        SerialFileDescriptor = -1;
        return false;
    }

    return true;
#endif
}

void FVN100OrientationReader::CloseSerialPort()
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

int32 FVN100OrientationReader::ReadSerial(uint8* Buffer, int32 MaxBytes)
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
