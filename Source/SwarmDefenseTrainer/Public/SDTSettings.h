#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SDTSettings.generated.h"

/**
 * Input mode enum — toggles between mouse-only (dev) and VN-100 hardware aiming.
 * In both modes the mouse left-click fires the weapon.
 * Set in DefaultGame.ini or Project Settings > SDT Settings.
 */
UENUM(BlueprintType)
enum class ESDTInputMode : uint8
{
    MockInput       UMETA(DisplayName = "Mock (Mouse Aim + Click)"),
    HardwareInput   UMETA(DisplayName = "Hardware (VN-100 Aim + Mouse Click)")
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

    /** Baud rate for VN-100 serial communication */
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
