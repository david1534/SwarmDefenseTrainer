#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SDTSettings.h"
#include "SDTPlayerController.generated.h"

class USDTWeaponComponent;

/**
 * Player controller that bridges hardware input to gameplay.
 *
 * In MockInput mode: standard mouse look + left-click to fire.
 * In HardwareInput mode: VN-100 orientation controls aim, mouse left-click fires.
 *
 * Handles calibration, sensitivity, smoothing, dead zones, and debug commands.
 */
UCLASS()
class SWARMDEFENSETRAINER_API ASDTPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ASDTPlayerController();

    virtual void Tick(float DeltaTime) override;

    /** Store current VN-100 orientation as the "center" reference point */
    UFUNCTION(BlueprintCallable, Category="Input")
    void CalibrateOrientation();

    /** Switch between mock (mouse) and hardware (VN-100) input at runtime */
    UFUNCTION(BlueprintCallable, Category="Input")
    void SetSDTInputMode(ESDTInputMode NewMode);

    UFUNCTION(BlueprintPure, Category="Input")
    ESDTInputMode GetSDTInputMode() const { return CurrentInputMode; }

    // --- Debug console commands (type ~ in UE5 to open console) ---

    /** Debug: skip to game over */
    UFUNCTION(Exec)
    void SDTEndGame();

    /** Debug: toggle between mock and hardware input */
    UFUNCTION(Exec)
    void SDTToggleInput();

    /** Debug: add score points */
    UFUNCTION(Exec)
    void SDTAddScore(int32 Amount);

    /** Debug: set player name for scoreboard */
    UFUNCTION(Exec)
    void SDTSetName(const FString& Name);

    /** Debug: force start the game (skip pre-game screen) */
    UFUNCTION(Exec)
    void SDTForceStart();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

private:
    void HandleMockInput(float DeltaTime);
    void HandleHardwareInput(float DeltaTime);
    void HandleFireInput();

    // Axis callbacks for mock mouse input
    void OnTurnAxis(float Value);
    void OnLookUpAxis(float Value);

    // Action callbacks
    void OnFirePressed();
    void OnFireReleased();
    void OnCalibratePressed();
    void OnRestartPressed();
    void OnStartGamePressed();
    void OnToggleInputModePressed();

    ESDTInputMode CurrentInputMode;

    // Smoothing state for hardware input
    FRotator SmoothedOrientation;
    FRotator CalibrationOffset;
    bool bCalibrated;

    // Fire state
    bool bFireHeld;

    // Cached axis input from mouse
    float TurnInput;
    float LookUpInput;

    // Tuning parameters (loaded from SDTSettings)
    float YawSensitivity;
    float PitchSensitivity;
    float SmoothingAlpha;
    float DeadZoneDegrees;
};
