#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"

/**
 * Background thread that reads trigger button signals from an Arduino over serial.
 *
 * Protocol: Arduino sends 'F' byte when trigger is pulled.
 * This class reads serial data and maintains a thread-safe "fire pressed" flag.
 *
 * The flag is set when 'F' is received and must be consumed (cleared) by the game thread.
 */
class HARDWARETRIGGER_API FTriggerSerialReader : public FRunnable
{
public:
    FTriggerSerialReader();
    virtual ~FTriggerSerialReader();

    bool Start(const FString& PortName, int32 BaudRate);
    void Stop();

    /** Returns true if trigger was pressed since last check. Consumes the event. */
    bool ConsumeTriggerPress();

    /** Returns true if trigger is currently being held (non-consuming check) */
    bool IsTriggerPressed() const;

    bool IsConnected() const;

    // FRunnable interface
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Exit() override;

private:
    bool OpenSerialPort(const FString& PortName, int32 BaudRate);
    void CloseSerialPort();
    int32 ReadSerial(uint8* Buffer, int32 MaxBytes);

    FRunnableThread* Thread;
    FThreadSafeBool bRunning;
    FThreadSafeBool bConnected;

    /** Mutex protecting bTriggerPressedFlag for atomic consume operations */
    mutable FCriticalSection TriggerMutex;
    bool bTriggerPressedFlag;

#if PLATFORM_WINDOWS
    void* SerialHandle;
#else
    int SerialFileDescriptor;
#endif
};
