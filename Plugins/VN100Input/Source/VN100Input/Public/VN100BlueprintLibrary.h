#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VN100BlueprintLibrary.generated.h"

/**
 * Blueprint-accessible functions for the VN-100 orientation sensor.
 *
 * Usage from Blueprints or C++:
 *   UVN100BlueprintLibrary::StartVN100("COM3", 115200);
 *   FRotator Ori = UVN100BlueprintLibrary::GetVN100Orientation();
 *   UVN100BlueprintLibrary::StopVN100();
 */
UCLASS()
class VN100INPUT_API UVN100BlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Start reading from the VN-100 on the specified serial port */
    UFUNCTION(BlueprintCallable, Category="VN100")
    static bool StartVN100(const FString& PortName, int32 BaudRate = 115200);

    /** Stop reading and close the serial port */
    UFUNCTION(BlueprintCallable, Category="VN100")
    static void StopVN100();

    /** Get the latest orientation reading (Pitch, Yaw, Roll) */
    UFUNCTION(BlueprintPure, Category="VN100")
    static FRotator GetVN100Orientation();

    /** Check if the VN-100 is connected and actively reading */
    UFUNCTION(BlueprintPure, Category="VN100")
    static bool IsVN100Connected();
};
