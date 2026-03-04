#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SDTGameMode.generated.h"

class ASDTWaveManager;
class ASDTHighValueAsset;
class ASDTDroneBase;
class USDTScoreManager;

/** Game phase state machine */
UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    PreGame     UMETA(DisplayName = "Pre-Game"),
    Countdown   UMETA(DisplayName = "Countdown"),
    Playing     UMETA(DisplayName = "Playing"),
    GameOver    UMETA(DisplayName = "Game Over")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameOver, int32, FinalScore);

/**
 * Core game mode — manages the full game flow:
 *   PreGame (title screen) → Countdown (3-2-1) → Playing (waves) → GameOver
 *
 * Owns the WaveManager and ScoreManager.
 * Sets default pawn, controller, and HUD classes.
 * Auto-initializes hardware serial connections when InputMode == HardwareInput.
 */
UCLASS()
class SWARMDEFENSETRAINER_API ASDTGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ASDTGameMode();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

    /** Begin the 3-2-1 countdown, then auto-start gameplay */
    UFUNCTION(BlueprintCallable, Category="Game")
    void BeginCountdown();

    /** Begin gameplay — spawns WaveManager, starts drone waves */
    UFUNCTION(BlueprintCallable, Category="Game")
    void StartGame();

    /** End gameplay — stops waves, submits score, broadcasts final score */
    UFUNCTION(BlueprintCallable, Category="Game")
    void EndGame();

    /** Restart by reloading the current level */
    UFUNCTION(BlueprintCallable, Category="Game")
    void RestartGame();

    UFUNCTION(BlueprintPure, Category="Game")
    float GetElapsedGameTime() const { return ElapsedGameTime; }

    UFUNCTION(BlueprintPure, Category="Game")
    EGamePhase GetCurrentPhase() const { return CurrentPhase; }

    UFUNCTION(BlueprintPure, Category="Game")
    USDTScoreManager* GetScoreManager() const { return ScoreManager; }

    UFUNCTION(BlueprintPure, Category="Game")
    ASDTHighValueAsset* GetHighValueAsset() const { return HighValueAsset; }

    UFUNCTION(BlueprintPure, Category="Game")
    ASDTWaveManager* GetWaveManager() const { return WaveManager; }

    UFUNCTION(BlueprintPure, Category="Game")
    int32 GetTotalKills() const { return TotalDronesKilled; }

    UFUNCTION(BlueprintPure, Category="Game")
    float GetCountdownTimeRemaining() const { return CountdownTimer; }

    UPROPERTY(BlueprintAssignable, Category="Game")
    FOnGameStarted OnGameStarted;

    UPROPERTY(BlueprintAssignable, Category="Game")
    FOnGameOver OnGameOver;

protected:
    /** Optional: set a custom WaveManager Blueprint class in the editor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Gameplay")
    TSubclassOf<ASDTWaveManager> WaveManagerClass;

    UPROPERTY()
    ASDTWaveManager* WaveManager;

    UPROPERTY()
    ASDTHighValueAsset* HighValueAsset;

    UPROPERTY()
    USDTScoreManager* ScoreManager;

private:
    UFUNCTION()
    void OnAssetDestroyed();

    UFUNCTION()
    void OnAllWavesComplete();

    UFUNCTION()
    void OnDroneSpawned(ASDTDroneBase* Drone);

    UFUNCTION()
    void OnDroneKilled(ASDTDroneBase* Drone);

    void InitializeHardware();
    void ShutdownHardware();

    EGamePhase CurrentPhase;
    float ElapsedGameTime;
    float CountdownTimer;
    int32 TotalDronesKilled;
    bool bHardwareInitialized;

    /** Tracks which countdown numbers we've already beeped for (3, 2, 1) */
    int32 LastCountdownBeep;

    /** True when all waves are cleared but we're waiting for MinGameDuration */
    bool bWavesComplete;

    /**
     * Minimum gameplay duration in seconds.
     * The game will not end (except by HVA destruction) before this time.
     * Default wave config naturally exceeds 30s, but this is an explicit safety net
     * to guarantee the "gameplay >= 30 seconds" requirement.
     */
    static constexpr float MinGameDuration = 30.f;
};
