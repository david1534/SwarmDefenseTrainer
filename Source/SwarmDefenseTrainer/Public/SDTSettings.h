#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SDTSettings.generated.h"

/**
 * Input mode enum — toggles between mouse (dev) and real hardware (VN-100 + trigger).
 * Set in DefaultGame.ini or Project Settings > SDT Settings.
 */
UENUM(BlueprintType)
enum class ESDTInputMode : uint8
{
    MockInput       UMETA(DisplayName = "Mock (Mouse)"),
    HardwareInput   UMETA(DisplayName = "Hardware (VN-100 + Trigger)")
};

/**
 * Developer settings for the SDT project.
 * Editable in Project Settings > Game > SDT Settings or DefaultGame.ini.
 */
UCLASS(Config=Game, defaultconfig, meta=(DisplayName="SDT Settings"))
class SWARMDEFENSETRAINER_API USDTSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    USDTSettings();

    /** Toggle between mock (mouse) and real hardware input */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Input")
    ESDTInputMode InputMode;

    /** Serial port name for the VN-100 orientation sensor (e.g. COM3, /dev/tty.usbserial) */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Input|Hardware")
    FString OrientationSerialPort;

    /** Serial port name for the Arduino trigger button */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Input|Hardware")
    FString TriggerSerialPort;

    /** Baud rate for serial communication */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Input|Hardware")
    int32 BaudRate;

    /** Horizontal aim sensitivity multiplier */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Input|Sensitivity", meta=(ClampMin="0.1", ClampMax="10.0"))
    float YawSensitivity;

    /** Vertical aim sensitivity multiplier */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Input|Sensitivity", meta=(ClampMin="0.1", ClampMax="10.0"))
    float PitchSensitivity;

    /** Smoothing alpha for hardware input (lower = smoother but laggier, 0.1-1.0) */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Input|Sensitivity", meta=(ClampMin="0.05", ClampMax="1.0"))
    float SmoothingAlpha;

    /** Ignore orientation changes smaller than this (degrees) */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Input|Sensitivity", meta=(ClampMin="0.0", ClampMax="5.0"))
    float DeadZoneDegrees;

    /** Minimum seconds between shots */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Gameplay", meta=(ClampMin="0.05", ClampMax="2.0"))
    float FireCooldown;

    /** Get the singleton settings instance */
    static const USDTSettings* Get();
};
