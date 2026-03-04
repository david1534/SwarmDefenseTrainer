#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"

/**
 * Background thread that reads VN-100 orientation data over serial.
 *
 * Protocol: VN-100 outputs ASCII sentences in the format:
 *   $VNYPR,yaw,pitch,roll*checksum
 *
 * This class:
 *   1. Opens a serial port (platform-specific: Win32 or POSIX)
 *   2. Spawns a background thread to continuously read data
 *   3. Parses $VNYPR sentences into FRotator
 *   4. Provides thread-safe access to the latest orientation
 *
 * Usage:
 *   Reader->Start("COM3", 115200);  // or "/dev/tty.usbserial-XXX"
 *   FRotator Ori = Reader->GetOrientation();
 *   Reader->Stop();
 */
class VN100INPUT_API FVN100OrientationReader : public FRunnable
{
public:
    FVN100OrientationReader();
    virtual ~FVN100OrientationReader();

    /** Open serial port and start the reader thread */
    bool Start(const FString& PortName, int32 BaudRate);

    /** Stop the reader thread and close the serial port */
    void Stop();

    /** Thread-safe: get the latest orientation (pitch, yaw, roll) */
    FRotator GetOrientation() const;

    /** Thread-safe: check if serial port is open and reading */
    bool IsConnected() const;

    // FRunnable interface
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Exit() override;

private:
    bool OpenSerialPort(const FString& PortName, int32 BaudRate);
    void CloseSerialPort();
    int32 ReadSerial(uint8* Buffer, int32 MaxBytes);
    bool ParseVNYPR(const FString& Line, float& OutYaw, float& OutPitch, float& OutRoll);

    // Thread management
    FRunnableThread* Thread;
    FThreadSafeBool bRunning;
    FThreadSafeBool bConnected;

    // Thread-safe orientation data
    mutable FCriticalSection OrientationMutex;
    FRotator CurrentOrientation;

    // Platform-specific serial handle
#if PLATFORM_WINDOWS
    void* SerialHandle; // HANDLE (Windows)
#else
    int SerialFileDescriptor; // file descriptor (POSIX / Mac)
#endif

    // Buffer for accumulating partial serial reads into lines
    FString LineBuffer;
};
