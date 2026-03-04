#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SDTHUD.generated.h"

class ASDTGameMode;
class ASDTHighValueAsset;

/**
 * Canvas-based HUD — draws all game screens directly on the canvas.
 *
 * Screens by phase:
 *   PreGame:  Title screen with controls, input mode, and Top-10 scoreboard
 *   Countdown: 3... 2... 1... GO! with scale animation
 *   Playing:  Crosshair, score, combo, wave info, kills, health bar, timer, hardware status
 *   GameOver: Victory/defeat, final stats, scoreboard, restart prompt
 */
UCLASS()
class SWARMDEFENSETRAINER_API ASDTHUD : public AHUD
{
    GENERATED_BODY()

public:
    ASDTHUD();

    virtual void DrawHUD() override;
    virtual void BeginPlay() override;

protected:
    // --- Gameplay HUD elements ---
    void DrawCrosshair();
    void DrawScoreDisplay();
    void DrawWaveInfo();
    void DrawHealthBar();
    void DrawGameTimer();
    void DrawComboDisplay();
    void DrawKillCounter();
    void DrawHardwareStatus();

    // --- Phase screens ---
    void DrawPreGameScreen();
    void DrawCountdownScreen();
    void DrawGameOverScreen();
    void DrawScoreboard(float StartX, float StartY);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD")
    FLinearColor CrosshairColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD")
    float CrosshairSize;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD")
    float CrosshairThickness;

private:
    UPROPERTY()
    ASDTGameMode* CachedGameMode;

    UPROPERTY()
    UFont* HUDFont;
};
