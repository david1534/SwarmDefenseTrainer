#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SDTSaveGame.h"
#include "SDTScoreManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoreChanged, int32, NewScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComboChanged, int32, NewCombo);

/**
 * Manages in-game scoring, combo multiplier, and persistent Top-10 scoreboard.
 * Created and owned by SDTGameMode.
 *
 * Scoring:
 *   - Base: 100 pts per drone kill
 *   - Combo: kills within 2s of each other increase multiplier (up to x10)
 *   - Score = BaseKillScore * ComboMultiplier
 */
UCLASS(BlueprintType)
class SWARMDEFENSETRAINER_API USDTScoreManager : public UObject
{
    GENERATED_BODY()

public:
    /** Call once at game start to reset state and load saved scores */
    void Initialize();

    /** Award points for a drone kill (applies combo multiplier) */
    UFUNCTION(BlueprintCallable, Category="Score")
    void AddKillScore();

    /** Reset current score and combo to zero */
    UFUNCTION(BlueprintCallable, Category="Score")
    void ResetScore();

    UFUNCTION(BlueprintPure, Category="Score")
    int32 GetCurrentScore() const { return CurrentScore; }

    UFUNCTION(BlueprintPure, Category="Score")
    int32 GetComboMultiplier() const { return ComboMultiplier; }

    /** Get the saved Top-10 scoreboard */
    UFUNCTION(BlueprintPure, Category="Score")
    TArray<FSDTScoreEntry> GetTopScores() const;

    /** Submit current score under a player name. Returns true if it made the Top-10. */
    UFUNCTION(BlueprintCallable, Category="Score")
    bool TrySubmitScore(const FString& PlayerName);

    /** Check if current score would qualify for the Top-10 */
    UFUNCTION(BlueprintPure, Category="Score")
    bool IsHighScore() const;

    UPROPERTY(BlueprintAssignable, Category="Score")
    FOnScoreChanged OnScoreChanged;

    UPROPERTY(BlueprintAssignable, Category="Score")
    FOnComboChanged OnComboChanged;

private:
    void LoadScores();
    void SaveScores();
    void UpdateCombo();

    int32 CurrentScore = 0;
    int32 ComboMultiplier = 1;
    float LastKillTime = 0.f;

    static constexpr float ComboWindowSeconds = 2.0f;
    static constexpr int32 BaseKillScore = 100;
    static constexpr int32 MaxCombo = 10;

    UPROPERTY()
    USDTSaveGame* SaveGameInstance;
};
