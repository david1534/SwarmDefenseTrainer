#include "SDTPlayerController.h"
#include "SDTCharacter.h"
#include "SDTWeaponComponent.h"
#include "SDTSettings.h"
#include "SDTGameMode.h"
#include "SDTGameInstance.h"
#include "SDTScoreManager.h"
#include "VN100BlueprintLibrary.h"
#include "TriggerBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/InputSettings.h"

ASDTPlayerController::ASDTPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;

    CurrentInputMode = ESDTInputMode::MockInput;
    SmoothedOrientation = FRotator::ZeroRotator;
    CalibrationOffset = FRotator::ZeroRotator;
    bCalibrated = false;
    bFireHeld = false;
    TurnInput = 0.f;
    LookUpInput = 0.f;
    YawSensitivity = 1.f;
    PitchSensitivity = 1.f;
    SmoothingAlpha = 0.3f;
    DeadZoneDegrees = 1.f;
}

void ASDTPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Load tuning parameters from project settings
    const USDTSettings* Settings = USDTSettings::Get();
    if (Settings)
    {
        CurrentInputMode = Settings->InputMode;
        YawSensitivity = Settings->YawSensitivity;
        PitchSensitivity = Settings->PitchSensitivity;
        SmoothingAlpha = Settings->SmoothingAlpha;
        DeadZoneDegrees = Settings->DeadZoneDegrees;
    }

    // Hide mouse cursor and lock input to game viewport
    bShowMouseCursor = false;
    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);

    // Mouse capture/lock is configured in DefaultInput.ini via:
    //   DefaultViewportMouseCaptureMode=CapturePermanently_IncludingInitialMouseDown
    //   DefaultViewportMouseLockMode=LockAlways
    // UE 5.7.4 removed SetCaptureMouseOnClick from UGameViewportClient,
    // so config-based settings are the correct approach.

    UE_LOG(LogTemp, Warning, TEXT("SDT: PlayerController BeginPlay — using GetInputAnalogKeyState for mouse input (v4)"));
}

void ASDTPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (!InputComponent) return;

    // Axis bindings — used in mock (mouse) mode
    InputComponent->BindAxis("Turn", this, &ASDTPlayerController::OnTurnAxis);
    InputComponent->BindAxis("LookUp", this, &ASDTPlayerController::OnLookUpAxis);

    // Action bindings — fire works in both modes (mouse click in mock, also hardware)
    InputComponent->BindAction("Fire", IE_Pressed, this, &ASDTPlayerController::OnFirePressed);
    InputComponent->BindAction("Fire", IE_Released, this, &ASDTPlayerController::OnFireReleased);
    InputComponent->BindAction("Calibrate", IE_Pressed, this, &ASDTPlayerController::OnCalibratePressed);

    // Game flow actions
    InputComponent->BindAction("Restart", IE_Pressed, this, &ASDTPlayerController::OnRestartPressed);
    InputComponent->BindAction("StartGame", IE_Pressed, this, &ASDTPlayerController::OnStartGamePressed);
    InputComponent->BindAction("ToggleInputMode", IE_Pressed, this, &ASDTPlayerController::OnToggleInputModePressed);
}

void ASDTPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Only process input during gameplay
    ASDTGameMode* GM = Cast<ASDTGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (!GM || GM->GetCurrentPhase() != EGamePhase::Playing) return;

    switch (CurrentInputMode)
    {
    case ESDTInputMode::MockInput:
        HandleMockInput(DeltaTime);
        break;
    case ESDTInputMode::HardwareInput:
        HandleHardwareInput(DeltaTime);
        break;
    }

    HandleFireInput();
}

void ASDTPlayerController::HandleMockInput(float DeltaTime)
{
    // Read mouse axes using GetInputAnalogKeyState — the most low-level
    // input API in UE5. This bypasses both Enhanced Input and legacy
    // AxisMapping systems, reading directly from the hardware input state.
    float MouseX = GetInputAnalogKeyState(EKeys::MouseX);
    float MouseY = GetInputAnalogKeyState(EKeys::MouseY);

    if (FMath::Abs(MouseX) > 0.01f)
    {
        AddYawInput(MouseX * YawSensitivity);
    }
    if (FMath::Abs(MouseY) > 0.01f)
    {
        AddPitchInput(-MouseY * PitchSensitivity);
    }
}

void ASDTPlayerController::HandleHardwareInput(float DeltaTime)
{
    // Read raw orientation from VN-100 plugin (yaw, pitch, roll)
    FRotator RawOrientation = UVN100BlueprintLibrary::GetVN100Orientation();

    // Subtract calibration offset so "center" aligns with where the user calibrated
    if (bCalibrated)
    {
        RawOrientation -= CalibrationOffset;
    }

    // Dead zone — ignore tiny movements to prevent jitter
    if (FMath::Abs(RawOrientation.Yaw) < DeadZoneDegrees) RawOrientation.Yaw = 0.f;
    if (FMath::Abs(RawOrientation.Pitch) < DeadZoneDegrees) RawOrientation.Pitch = 0.f;

    // Exponential moving average smoothing to reduce jitter
    SmoothedOrientation.Yaw = FMath::Lerp(
        SmoothedOrientation.Yaw,
        RawOrientation.Yaw * YawSensitivity,
        SmoothingAlpha);
    SmoothedOrientation.Pitch = FMath::Lerp(
        SmoothedOrientation.Pitch,
        RawOrientation.Pitch * PitchSensitivity,
        SmoothingAlpha);
    SmoothedOrientation.Roll = 0.f;

    // Set control rotation directly from hardware orientation
    SetControlRotation(SmoothedOrientation);
}

void ASDTPlayerController::HandleFireInput()
{
    bool bShouldFire = false;

    switch (CurrentInputMode)
    {
    case ESDTInputMode::MockInput:
        bShouldFire = bFireHeld;
        break;
    case ESDTInputMode::HardwareInput:
        // Check both hardware trigger AND mouse click (allow mouse as fallback)
        // Use ConsumeTriggerPress() so the flag clears after each read,
        // preventing a single press from firing indefinitely.
        bShouldFire = UTriggerBlueprintLibrary::ConsumeTriggerPress() || bFireHeld;
        break;
    }

    if (bShouldFire)
    {
        ASDTCharacter* SDTChar = Cast<ASDTCharacter>(GetPawn());
        if (SDTChar && SDTChar->WeaponComponent)
        {
            SDTChar->WeaponComponent->Fire();
        }
    }
}

// --- Input axis/action callbacks ---

void ASDTPlayerController::OnTurnAxis(float Value)
{
    TurnInput = Value;
}

void ASDTPlayerController::OnLookUpAxis(float Value)
{
    LookUpInput = Value;
}

void ASDTPlayerController::OnFirePressed()
{
    bFireHeld = true;
}

void ASDTPlayerController::OnFireReleased()
{
    bFireHeld = false;
}

void ASDTPlayerController::OnCalibratePressed()
{
    CalibrateOrientation();
}

void ASDTPlayerController::OnRestartPressed()
{
    ASDTGameMode* GM = Cast<ASDTGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GM && GM->GetCurrentPhase() == EGamePhase::GameOver)
    {
        GM->RestartGame();
    }
}

void ASDTPlayerController::OnStartGamePressed()
{
    ASDTGameMode* GM = Cast<ASDTGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GM && GM->GetCurrentPhase() == EGamePhase::PreGame)
    {
        GM->BeginCountdown();
    }
}

void ASDTPlayerController::OnToggleInputModePressed()
{
    if (CurrentInputMode == ESDTInputMode::MockInput)
    {
        SetSDTInputMode(ESDTInputMode::HardwareInput);
    }
    else
    {
        SetSDTInputMode(ESDTInputMode::MockInput);
    }
}

// --- Public methods ---

void ASDTPlayerController::CalibrateOrientation()
{
    if (CurrentInputMode == ESDTInputMode::HardwareInput)
    {
        CalibrationOffset = UVN100BlueprintLibrary::GetVN100Orientation();
        bCalibrated = true;
        SmoothedOrientation = FRotator::ZeroRotator;
        UE_LOG(LogTemp, Log, TEXT("SDT: VN-100 calibrated. Offset: %s"), *CalibrationOffset.ToString());
    }
}

void ASDTPlayerController::SetSDTInputMode(ESDTInputMode NewMode)
{
    CurrentInputMode = NewMode;
    bCalibrated = false;
    SmoothedOrientation = FRotator::ZeroRotator;
    CalibrationOffset = FRotator::ZeroRotator;
    UE_LOG(LogTemp, Log, TEXT("SDT: Input mode changed to %s"),
        NewMode == ESDTInputMode::MockInput ? TEXT("Mock (Mouse)") : TEXT("Hardware (VN-100)"));
}

// --- Debug console commands (type ~ in UE5 editor to open console) ---

void ASDTPlayerController::SDTEndGame()
{
    ASDTGameMode* GM = Cast<ASDTGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GM)
    {
        GM->EndGame();
        UE_LOG(LogTemp, Log, TEXT("SDT Debug: Forced game over."));
    }
}

void ASDTPlayerController::SDTToggleInput()
{
    OnToggleInputModePressed();
}

void ASDTPlayerController::SDTAddScore(int32 Amount)
{
    ASDTGameMode* GM = Cast<ASDTGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GM && GM->GetScoreManager())
    {
        for (int32 i = 0; i < Amount / 100; i++)
        {
            GM->GetScoreManager()->AddKillScore();
        }
        UE_LOG(LogTemp, Log, TEXT("SDT Debug: Added ~%d score points."), Amount);
    }
}

void ASDTPlayerController::SDTSetName(const FString& Name)
{
    USDTGameInstance* GI = Cast<USDTGameInstance>(GetGameInstance());
    if (GI)
    {
        GI->SetPlayerName(Name);
        UE_LOG(LogTemp, Log, TEXT("SDT Debug: Player name set to '%s'."), *Name);
    }
}

void ASDTPlayerController::SDTForceStart()
{
    ASDTGameMode* GM = Cast<ASDTGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GM)
    {
        if (GM->GetCurrentPhase() == EGamePhase::PreGame)
        {
            GM->BeginCountdown();
            UE_LOG(LogTemp, Log, TEXT("SDT Debug: Force-started countdown."));
        }
    }
}
