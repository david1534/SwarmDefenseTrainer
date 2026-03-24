#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TriggerBlueprintLibrary.generated.h"

/**
 * Blueprint-accessible functions for the hardware trigger button.
 *
 * Usage:
 *   UTriggerBlueprintLibrary::StartTrigger("COM4", 115200);
 *   bool bFired = UTriggerBlueprintLibrary::IsTriggerPressed();
 *   UTriggerBlueprintLibrary::StopTrigger();
 */
UCLASS()
class HARDWARETRIGGER_API UTriggerBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Start reading trigger signals from the Arduino on the specified serial port */
    UFUNCTION(BlueprintCallable, Category="Trigger")
    static bool StartTrigger(const FString& PortName, int32 BaudRate = 115200);

    /** Stop reading and close the serial port */
    UFUNCTION(BlueprintCallable, Category="Trigger")
    static void StopTrigger();

    /** Check if the trigger is currently pressed (non-consuming) */
    UFUNCTION(BlueprintCallable, Category="Trigger")
    static bool IsTriggerPressed();

    /** Check if trigger was pressed since last check, then clear the flag */
    UFUNCTION(BlueprintCallable, Category="Trigger")
    static bool ConsumeTriggerPress();

    /** Check if the Arduino trigger is connected */
    UFUNCTION(BlueprintPure, Category="Trigger")
    static bool IsTriggerConnected();
};
