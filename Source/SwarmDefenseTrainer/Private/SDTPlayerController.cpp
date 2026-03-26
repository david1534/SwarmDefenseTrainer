#include "SDTPlayerController.h"
#include "SDTCharacter.h"
#include "SDTWeaponComponent.h"
#include "SDTSettings.h"
#include "SDTGameMode.h"
#include "SDTGameInstance.h"
#include "SDTScoreManager.h"
#include "VN100BlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/InputSettings.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"

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

    // FInputModeGameOnly overrides our DefaultInput.ini settings, changing
    // LockAlways -> LockOnCapture. Re-apply LockAlways so the mouse is
    // captured immediately in PIE without needing a click first.
    // Note: SetCaptureMouseOnClick was removed in UE 5.7.4, but
    // SetMouseLockMode still exists on UGameViewportClient.
    ULocalPlayer* LP = GetLocalPlayer();
    if (LP && LP->ViewportClient)
    {
        LP->ViewportClient->SetMouseLockMode(EMouseLockMode::LockAlways);
    }

    UE_LOG(LogTemp, Warning, TEXT("SDT: PlayerController BeginPlay — mouse LockAlways applied, using GetInputMouseDelta (v7)"));
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
    // Use APlayerController::GetInputMouseDelta — reads mouse delta
    // directly from the viewport, bypassing the PlayerInput class.
    // Works regardless of whether Enhanced or Legacy input is active.
    float MouseDeltaX = 0.f;
    float MouseDeltaY = 0.f;
    GetInputMouseDelta(MouseDeltaX, MouseDeltaY);

    if (FMath::Abs(MouseDeltaX) > 0.01f)
    {
        AddYawInput(MouseDeltaX * YawSensitivity);
    }
    if (FMath::Abs(MouseDeltaY) > 0.01f)
    {
        AddPitchInput(-MouseDeltaY * PitchSensitivity);
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
    // Mouse left-click fires in both Mock and Hardware modes
    if (bFireHeld)
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
