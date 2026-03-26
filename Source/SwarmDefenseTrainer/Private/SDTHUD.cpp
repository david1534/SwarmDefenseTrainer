#include "SDTHUD.h"
#include "SDTGameMode.h"
#include "SDTScoreManager.h"
#include "SDTHighValueAsset.h"
#include "SDTWaveManager.h"
#include "SDTSettings.h"
#include "SDTSaveGame.h"
#include "VN100BlueprintLibrary.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"

ASDTHUD::ASDTHUD()
{
    CrosshairColor = FLinearColor::Green;
    CrosshairSize = 20.f;
    CrosshairThickness = 2.f;
}

void ASDTHUD::BeginPlay()
{
    Super::BeginPlay();

    CachedGameMode = Cast<ASDTGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    HUDFont = GEngine ? GEngine->GetLargeFont() : nullptr;
}

void ASDTHUD::DrawHUD()
{
    Super::DrawHUD();

    if (!Canvas || !CachedGameMode) return;

    EGamePhase Phase = CachedGameMode->GetCurrentPhase();

    switch (Phase)
    {
    case EGamePhase::PreGame:
        DrawPreGameScreen();
        break;
    case EGamePhase::Countdown:
        DrawCountdownScreen();
        break;
    case EGamePhase::Playing:
        DrawCrosshair();
        DrawScoreDisplay();
        DrawComboDisplay();
        DrawWaveInfo();
        DrawKillCounter();
        DrawHealthBar();
        DrawGameTimer();
        DrawHardwareStatus();
        break;
    case EGamePhase::GameOver:
        DrawGameOverScreen();
        break;
    }
}

// ============================================================
//  PRE-GAME TITLE SCREEN
// ============================================================

void ASDTHUD::DrawPreGameScreen()
{
    if (!HUDFont) return;

    float CenterX = Canvas->SizeX * 0.5f;
    float CenterY = Canvas->SizeY * 0.5f;

    // Dark background
    DrawRect(FLinearColor(0.05f, 0.05f, 0.1f, 0.95f), 0, 0, Canvas->SizeX, Canvas->SizeY);

    // Title
    DrawText(TEXT("SWARM DEFENSE TRAINER"),
        FLinearColor::White, CenterX - 260.f, CenterY - 200.f, HUDFont, 3.f);

    // Subtitle
    DrawText(TEXT("Counter-UAS Training System"),
        FLinearColor(0.5f, 0.7f, 1.f, 1.f), CenterX - 180.f, CenterY - 140.f, HUDFont, 1.2f);

    // Objective
    DrawText(TEXT("Defend the High Value Asset against incoming drone swarms."),
        FLinearColor(0.8f, 0.8f, 0.8f, 1.f), CenterX - 280.f, CenterY - 70.f, HUDFont, 1.f);

    // Input mode indicator
    const USDTSettings* Settings = USDTSettings::Get();
    if (Settings)
    {
        FString ModeText = Settings->InputMode == ESDTInputMode::MockInput
            ? TEXT("Input Mode: Mouse Aim + Click (Mock)")
            : TEXT("Input Mode: VN-100 Aim + Mouse Click (Hardware)");
        DrawText(ModeText, FLinearColor(0.6f, 0.8f, 0.6f, 1.f),
            CenterX - 200.f, CenterY - 40.f, HUDFont, 0.9f);
    }

    // Controls
    DrawText(TEXT("Controls:"), FLinearColor::Yellow,
        CenterX - 130.f, CenterY + 10.f, HUDFont, 1.f);
    DrawText(TEXT("Mouse / VN-100  -  Aim"), FLinearColor(0.7f, 0.7f, 0.7f, 1.f),
        CenterX - 130.f, CenterY + 35.f, HUDFont, 0.8f);
    DrawText(TEXT("Left Click  -  Fire"), FLinearColor(0.7f, 0.7f, 0.7f, 1.f),
        CenterX - 130.f, CenterY + 55.f, HUDFont, 0.8f);
    DrawText(TEXT("C  -  Calibrate (Hardware Mode)"), FLinearColor(0.7f, 0.7f, 0.7f, 1.f),
        CenterX - 130.f, CenterY + 75.f, HUDFont, 0.8f);
    DrawText(TEXT("T  -  Toggle Input Mode"), FLinearColor(0.7f, 0.7f, 0.7f, 1.f),
        CenterX - 130.f, CenterY + 95.f, HUDFont, 0.8f);

    // Start prompt — pulsing green
    float Pulse = (FMath::Sin(GetWorld()->GetTimeSeconds() * 3.f) + 1.f) * 0.5f;
    FLinearColor PromptColor = FMath::Lerp(
        FLinearColor(0.3f, 0.3f, 0.3f, 1.f), FLinearColor::Green, Pulse);
    DrawText(TEXT("[ Press SPACE to Start ]"), PromptColor,
        CenterX - 150.f, CenterY + 150.f, HUDFont, 1.5f);

    // Scoreboard at bottom
    DrawText(TEXT("--- TOP SCORES ---"), FLinearColor::Yellow,
        CenterX - 90.f, CenterY + 210.f, HUDFont, 0.9f);
    DrawScoreboard(CenterX - 150.f, CenterY + 235.f);
}

// ============================================================
//  COUNTDOWN SCREEN (3... 2... 1... GO!)
// ============================================================

void ASDTHUD::DrawCountdownScreen()
{
    if (!HUDFont) return;

    float CenterX = Canvas->SizeX * 0.5f;
    float CenterY = Canvas->SizeY * 0.5f;

    float TimeLeft = CachedGameMode->GetCountdownTimeRemaining();
    int32 CountNumber = FMath::CeilToInt(TimeLeft);

    FString CountText;
    FLinearColor CountColor;

    if (CountNumber > 0)
    {
        CountText = FString::Printf(TEXT("%d"), CountNumber);
        CountColor = FLinearColor::Yellow;
    }
    else
    {
        CountText = TEXT("GO!");
        CountColor = FLinearColor::Green;
    }

    // Scale animation — number pulses bigger as it counts down
    float Frac = FMath::Frac(TimeLeft);
    float Scale = FMath::Lerp(5.f, 3.5f, Frac);

    DrawText(CountText, CountColor, CenterX - 50.f, CenterY - 60.f, HUDFont, Scale);

    // Sub-label
    DrawText(TEXT("Get Ready!"), FLinearColor(0.7f, 0.7f, 0.7f, 1.f),
        CenterX - 65.f, CenterY + 50.f, HUDFont, 1.2f);
}

// ============================================================
//  GAMEPLAY HUD — CROSSHAIR
// ============================================================

void ASDTHUD::DrawCrosshair()
{
    float CenterX = Canvas->SizeX * 0.5f;
    float CenterY = Canvas->SizeY * 0.5f;
    float Half = CrosshairSize * 0.5f;
    float Gap = 4.f;

    // Horizontal lines
    DrawLine(CenterX - Half, CenterY, CenterX - Gap, CenterY, CrosshairColor, CrosshairThickness);
    DrawLine(CenterX + Gap, CenterY, CenterX + Half, CenterY, CrosshairColor, CrosshairThickness);

    // Vertical lines
    DrawLine(CenterX, CenterY - Half, CenterX, CenterY - Gap, CrosshairColor, CrosshairThickness);
    DrawLine(CenterX, CenterY + Gap, CenterX, CenterY + Half, CrosshairColor, CrosshairThickness);

    // Center dot
    DrawRect(CrosshairColor, CenterX - 1.f, CenterY - 1.f, 2.f, 2.f);
}

// ============================================================
//  GAMEPLAY HUD — SCORE (top-left)
// ============================================================

void ASDTHUD::DrawScoreDisplay()
{
    USDTScoreManager* ScoreManager = CachedGameMode->GetScoreManager();
    if (!ScoreManager || !HUDFont) return;

    FString ScoreText = FString::Printf(TEXT("SCORE: %d"), ScoreManager->GetCurrentScore());
    DrawText(ScoreText, FLinearColor::White, 20.f, 20.f, HUDFont, 1.5f);
}

// ============================================================
//  GAMEPLAY HUD — COMBO MULTIPLIER (below score)
// ============================================================

void ASDTHUD::DrawComboDisplay()
{
    USDTScoreManager* ScoreManager = CachedGameMode->GetScoreManager();
    if (!ScoreManager || !HUDFont) return;

    int32 Combo = ScoreManager->GetComboMultiplier();
    if (Combo > 1)
    {
        FString ComboText = FString::Printf(TEXT("x%d COMBO!"), Combo);
        DrawText(ComboText, FLinearColor::Yellow, 20.f, 55.f, HUDFont, 1.8f);
    }
}

// ============================================================
//  GAMEPLAY HUD — WAVE INFO (top-right) — FIXED
// ============================================================

void ASDTHUD::DrawWaveInfo()
{
    if (!HUDFont) return;

    ASDTWaveManager* WaveMgr = CachedGameMode->GetWaveManager();

    FString WaveText;
    if (WaveMgr)
    {
        WaveText = FString::Printf(TEXT("WAVE: %d/%d"),
            WaveMgr->GetCurrentWave(), WaveMgr->GetTotalWaves());
    }
    else
    {
        WaveText = TEXT("WAVE: --/--");
    }

    DrawText(WaveText, FLinearColor::White, Canvas->SizeX - 220.f, 20.f, HUDFont, 1.2f);
}

// ============================================================
//  GAMEPLAY HUD — KILL COUNTER (below wave info)
// ============================================================

void ASDTHUD::DrawKillCounter()
{
    if (!HUDFont) return;

    int32 Kills = CachedGameMode->GetTotalKills();
    FString KillText = FString::Printf(TEXT("KILLS: %d"), Kills);
    DrawText(KillText, FLinearColor(1.f, 0.6f, 0.2f, 1.f),
        Canvas->SizeX - 220.f, 50.f, HUDFont, 1.0f);
}

// ============================================================
//  GAMEPLAY HUD — ASSET HEALTH BAR (bottom-center)
// ============================================================

void ASDTHUD::DrawHealthBar()
{
    ASDTHighValueAsset* HVA = CachedGameMode->GetHighValueAsset();
    if (!HVA || !HUDFont) return;

    float BarWidth = 300.f;
    float BarHeight = 20.f;
    float BarX = (Canvas->SizeX - BarWidth) * 0.5f;
    float BarY = Canvas->SizeY - 60.f;

    // Background
    FLinearColor BgColor(0.2f, 0.2f, 0.2f, 0.8f);
    DrawRect(BgColor, BarX, BarY, BarWidth, BarHeight);

    // Health fill — green when healthy, transitions to red
    float HealthPct = HVA->GetHealthPercent();
    FLinearColor HealthColor = FMath::Lerp(FLinearColor::Red, FLinearColor::Green, HealthPct);
    DrawRect(HealthColor, BarX, BarY, BarWidth * HealthPct, BarHeight);

    // Border
    DrawLine(BarX, BarY, BarX + BarWidth, BarY, FLinearColor::White, 1.f);
    DrawLine(BarX, BarY + BarHeight, BarX + BarWidth, BarY + BarHeight, FLinearColor::White, 1.f);
    DrawLine(BarX, BarY, BarX, BarY + BarHeight, FLinearColor::White, 1.f);
    DrawLine(BarX + BarWidth, BarY, BarX + BarWidth, BarY + BarHeight, FLinearColor::White, 1.f);

    // Label
    FString Label = FString::Printf(TEXT("ASSET HEALTH: %d%%"), FMath::RoundToInt(HealthPct * 100.f));
    DrawText(Label, FLinearColor::White, BarX, BarY - 22.f, HUDFont, 0.8f);
}

// ============================================================
//  GAMEPLAY HUD — GAME TIMER (top-center)
// ============================================================

void ASDTHUD::DrawGameTimer()
{
    if (!HUDFont) return;

    float Time = CachedGameMode->GetElapsedGameTime();
    int32 Minutes = FMath::FloorToInt(Time / 60.f);
    int32 Seconds = FMath::FloorToInt(FMath::Fmod(Time, 60.f));

    FString TimeText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
    DrawText(TimeText, FLinearColor::White, Canvas->SizeX * 0.5f - 40.f, 20.f, HUDFont, 1.5f);
}

// ============================================================
//  GAMEPLAY HUD — HARDWARE STATUS (bottom-right, hardware mode only)
// ============================================================

void ASDTHUD::DrawHardwareStatus()
{
    if (!HUDFont) return;

    const USDTSettings* Settings = USDTSettings::Get();
    if (!Settings || Settings->InputMode != ESDTInputMode::HardwareInput) return;

    float X = Canvas->SizeX - 280.f;
    float Y = Canvas->SizeY - 60.f;

    bool bVN100 = UVN100BlueprintLibrary::IsVN100Connected();

    FLinearColor VNColor = bVN100 ? FLinearColor::Green : FLinearColor::Red;
    FString VNStatus = bVN100 ? TEXT("VN-100: CONNECTED") : TEXT("VN-100: DISCONNECTED");

    DrawText(VNStatus, VNColor, X, Y, HUDFont, 0.7f);
    DrawText(TEXT("FIRE: Mouse Click"), FLinearColor(0.6f, 0.8f, 0.6f, 1.f), X, Y + 18.f, HUDFont, 0.7f);
}

// ============================================================
//  GAME OVER SCREEN
// ============================================================

void ASDTHUD::DrawGameOverScreen()
{
    if (!HUDFont) return;

    float CenterX = Canvas->SizeX * 0.5f;
    float CenterY = Canvas->SizeY * 0.5f;

    // Semi-transparent black overlay
    DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.85f), 0, 0, Canvas->SizeX, Canvas->SizeY);

    // Check if player survived all waves or HVA was destroyed
    ASDTHighValueAsset* HVA = CachedGameMode->GetHighValueAsset();
    bool bVictory = HVA && !HVA->IsDestroyed();

    if (bVictory)
    {
        DrawText(TEXT("VICTORY!"), FLinearColor::Green,
            CenterX - 110.f, CenterY - 200.f, HUDFont, 3.f);
        DrawText(TEXT("All waves cleared!"), FLinearColor(0.5f, 1.f, 0.5f, 1.f),
            CenterX - 110.f, CenterY - 140.f, HUDFont, 1.2f);
    }
    else
    {
        DrawText(TEXT("GAME OVER"), FLinearColor::Red,
            CenterX - 130.f, CenterY - 200.f, HUDFont, 3.f);
        DrawText(TEXT("Asset Destroyed!"), FLinearColor(1.f, 0.5f, 0.5f, 1.f),
            CenterX - 100.f, CenterY - 140.f, HUDFont, 1.2f);
    }

    // --- Final stats ---
    float Y = CenterY - 90.f;

    USDTScoreManager* ScoreManager = CachedGameMode->GetScoreManager();
    if (ScoreManager)
    {
        FString FinalScore = FString::Printf(TEXT("FINAL SCORE: %d"), ScoreManager->GetCurrentScore());
        DrawText(FinalScore, FLinearColor::White, CenterX - 120.f, Y, HUDFont, 1.5f);

        if (ScoreManager->IsHighScore())
        {
            DrawText(TEXT("NEW HIGH SCORE!"), FLinearColor::Yellow,
                CenterX - 105.f, Y + 35.f, HUDFont, 1.3f);
        }
        Y += 60.f;
    }

    FString KillsText = FString::Printf(TEXT("DRONES DESTROYED: %d"), CachedGameMode->GetTotalKills());
    DrawText(KillsText, FLinearColor::White, CenterX - 130.f, Y, HUDFont, 1.2f);

    float TimeElapsed = CachedGameMode->GetElapsedGameTime();
    FString TimeText = FString::Printf(TEXT("TIME SURVIVED: %.1f seconds"), TimeElapsed);
    DrawText(TimeText, FLinearColor::White, CenterX - 140.f, Y + 30.f, HUDFont, 1.2f);

    // --- Scoreboard ---
    DrawText(TEXT("--- TOP SCORES ---"), FLinearColor::Yellow,
        CenterX - 90.f, Y + 80.f, HUDFont, 1.f);
    DrawScoreboard(CenterX - 150.f, Y + 108.f);

    // --- Restart prompt (pulsing) ---
    float Pulse = (FMath::Sin(GetWorld()->GetTimeSeconds() * 3.f) + 1.f) * 0.5f;
    FLinearColor RestartColor = FMath::Lerp(
        FLinearColor(0.3f, 0.3f, 0.3f, 1.f), FLinearColor::White, Pulse);
    DrawText(TEXT("[ Press R to Restart ]"), RestartColor,
        CenterX - 135.f, Canvas->SizeY - 80.f, HUDFont, 1.3f);
}

// ============================================================
//  SCOREBOARD (reusable — drawn on PreGame and GameOver screens)
// ============================================================

void ASDTHUD::DrawScoreboard(float StartX, float StartY)
{
    USDTScoreManager* ScoreManager = CachedGameMode->GetScoreManager();
    if (!ScoreManager || !HUDFont) return;

    TArray<FSDTScoreEntry> Scores = ScoreManager->GetTopScores();

    if (Scores.Num() == 0)
    {
        DrawText(TEXT("No scores yet — be the first!"),
            FLinearColor(0.5f, 0.5f, 0.5f, 1.f), StartX + 20.f, StartY, HUDFont, 0.8f);
        return;
    }

    for (int32 i = 0; i < Scores.Num() && i < 10; i++)
    {
        float Y = StartY + (i * 22.f);

        // Gold for #1, silver for #2, bronze for #3, grey for rest
        FLinearColor EntryColor;
        if (i == 0) EntryColor = FLinearColor(1.f, 0.84f, 0.f, 1.f);       // Gold
        else if (i == 1) EntryColor = FLinearColor(0.75f, 0.75f, 0.75f, 1.f); // Silver
        else if (i == 2) EntryColor = FLinearColor(0.8f, 0.5f, 0.2f, 1.f);   // Bronze
        else EntryColor = FLinearColor(0.6f, 0.6f, 0.6f, 1.f);

        FString EntryText = FString::Printf(TEXT("%2d.  %-12s  %d"),
            i + 1, *Scores[i].PlayerName, Scores[i].Score);
        DrawText(EntryText, EntryColor, StartX, Y, HUDFont, 0.8f);
    }
}
