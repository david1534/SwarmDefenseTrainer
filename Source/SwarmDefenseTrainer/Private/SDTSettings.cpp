#include "SDTSettings.h"

USDTSettings::USDTSettings()
{
    InputMode = ESDTInputMode::MockInput;
    OrientationSerialPort = TEXT("COM3");
    BaudRate = 115200;
    YawSensitivity = 1.0f;
    PitchSensitivity = 1.0f;
    SmoothingAlpha = 0.3f;
    DeadZoneDegrees = 1.0f;
    FireCooldown = 0.15f;
}

const USDTSettings* USDTSettings::Get()
{
    return GetDefault<USDTSettings>();
}
