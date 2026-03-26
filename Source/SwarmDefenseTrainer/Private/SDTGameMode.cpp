#include "SDTGameMode.h"
#include "SDTCharacter.h"
#include "SDTPlayerController.h"
#include "SDTHUD.h"
#include "SDTWaveManager.h"
#include "SDTHighValueAsset.h"
#include "SDTScoreManager.h"
#include "SDTDroneBase.h"
#include "SDTSettings.h"
#include "SDTGameInstance.h"
#include "SDTDefaultSounds.h"
#include "VN100BlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ASDTGameMode::ASDTGameMode()
{
    PrimaryActorTick.bCanEverTick = true;

    // Set default classes — the core of the SDT framework
    DefaultPawnClass = ASDTCharacter::StaticClass();
    PlayerControllerClass = ASDTPlayerController::StaticClass();
    HUDClass = ASDTHUD::StaticClass();

    CurrentPhase = EGamePhase::PreGame;
    ElapsedGameTime = 0.f;
    CountdownTimer = 0.f;
    TotalDronesKilled = 0;
    bHardwareInitialized = false;
    LastCountdownBeep = 0;
    bWavesComplete = false;
}

void ASDTGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Create the score manager (not an actor, just a UObject)
    ScoreManager = NewObject<USDTScoreManager>(this);
    ScoreManager->Initialize();

    // Find the High Value Asset placed in the level
    AActor* FoundHVA = UGameplayStatics::GetActorOfClass(GetWorld(), ASDTHighValueAsset::StaticClass());
    HighValueAsset = Cast<ASDTHighValueAsset>(FoundHVA);

    if (HighValueAsset)
    {
        HighValueAsset->OnAssetDestroyed.AddDynamic(this, &ASDTGameMode::OnAssetDestroyed);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SDT: No ASDTHighValueAsset found in level! Place one in the map."));
    }

    // Initialize hardware serial connections if in HardwareInput mode
    InitializeHardware();

    // Start in PreGame phase — player sees title screen, presses SPACE to begin
    CurrentPhase = EGamePhase::PreGame;
    UE_LOG(LogTemp, Log, TEXT("SDT: Game loaded. Press SPACE to begin."));
}

void ASDTGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ShutdownHardware();
    Super::EndPlay(EndPlayReason);
}

void ASDTGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (CurrentPhase == EGamePhase::Countdown)
    {
        // Play countdown beep sounds (3, 2, 1)
        int32 CurrentCount = FMath::CeilToInt(CountdownTimer);
        if (CurrentCount != LastCountdownBeep && CurrentCount >= 1 && CurrentCount <= 3)
        {
            FSDTDefaultSounds::PlayCountdownBeep(GetWorld());
            LastCountdownBeep = CurrentCount;
        }

        CountdownTimer -= DeltaTime;
        if (CountdownTimer <= 0.f)
        {
            CountdownTimer = 0.f;
            StartGame();
        }
    }
    else if (CurrentPhase == EGamePhase::Playing)
    {
        ElapsedGameTime += DeltaTime;

        // If waves finished early, end game once MinGameDuration is met
        if (bWavesComplete && ElapsedGameTime >= MinGameDuration)
        {
            UE_LOG(LogTemp, Log, TEXT("SDT: Minimum duration reached (%.1fs). All waves cleared — Victory!"),
                ElapsedGameTime);
            EndGame();
        }
    }
}

void ASDTGameMode::BeginCountdown()
{
    if (CurrentPhase != EGamePhase::PreGame) return;

    CurrentPhase = EGamePhase::Countdown;
    CountdownTimer = 3.0f;
    ElapsedGameTime = 0.f;
    TotalDronesKilled = 0;
    LastCountdownBeep = 0;
    bWavesComplete = false;

    if (ScoreManager)
    {
        ScoreManager->ResetScore();
    }

    UE_LOG(LogTemp, Log, TEXT("SDT: Countdown started — 3... 2... 1..."));
}

void ASDTGameMode::StartGame()
{
    if (CurrentPhase != EGamePhase::Countdown) return;

    CurrentPhase = EGamePhase::Playing;
    ElapsedGameTime = 0.f;

    // Spawn the wave manager
    FActorSpawnParameters SpawnParams;
    if (WaveManagerClass)
    {
        WaveManager = GetWorld()->SpawnActor<ASDTWaveManager>(
            WaveManagerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    }
    else
    {
        WaveManager = GetWorld()->SpawnActor<ASDTWaveManager>(
            ASDTWaveManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    }

    if (WaveManager)
    {
        // Listen for drone spawns so we can track kills for scoring
        WaveManager->OnDroneSpawned.AddDynamic(this, &ASDTGameMode::OnDroneSpawned);
        WaveManager->OnAllWavesComplete.AddDynamic(this, &ASDTGameMode::OnAllWavesComplete);
        WaveManager->StartWaves();
    }

    // Play game start sound
    FSDTDefaultSounds::PlayGameStart(GetWorld());

    OnGameStarted.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("SDT: Game Started!"));
}

void ASDTGameMode::EndGame()
{
    if (CurrentPhase == EGamePhase::GameOver) return;

    CurrentPhase = EGamePhase::GameOver;

    if (WaveManager)
    {
        WaveManager->StopWaves();
    }

    int32 FinalScore = ScoreManager ? ScoreManager->GetCurrentScore() : 0;

    // Auto-submit score with player name from GameInstance
    if (ScoreManager)
    {
        FString PlayerName = TEXT("Player");
        USDTGameInstance* GI = Cast<USDTGameInstance>(GetGameInstance());
        if (GI && !GI->GetPlayerName().IsEmpty())
        {
            PlayerName = GI->GetPlayerName();
        }
        ScoreManager->TrySubmitScore(PlayerName);
    }

    // Play game over sound
    FSDTDefaultSounds::PlayGameOver(GetWorld());

    OnGameOver.Broadcast(FinalScore);

    UE_LOG(LogTemp, Log, TEXT("SDT: Game Over! Final Score: %d | Kills: %d | Time: %.1fs"),
        FinalScore, TotalDronesKilled, ElapsedGameTime);
}

void ASDTGameMode::RestartGame()
{
    UE_LOG(LogTemp, Log, TEXT("SDT: Restarting game..."));

    // Reload the current level — cleanly resets everything
    UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()), true);
}

// --- Hardware serial initialization / shutdown ---

void ASDTGameMode::InitializeHardware()
{
    const USDTSettings* Settings = USDTSettings::Get();
    if (!Settings || Settings->InputMode != ESDTInputMode::HardwareInput) return;

    UE_LOG(LogTemp, Log, TEXT("SDT: Initializing hardware connections..."));

    // Start VN-100 orientation sensor
    bool bVN100 = UVN100BlueprintLibrary::StartVN100(
        Settings->OrientationSerialPort, Settings->BaudRate);
    if (bVN100)
    {
        UE_LOG(LogTemp, Log, TEXT("SDT: VN-100 connected on %s @ %d baud"),
            *Settings->OrientationSerialPort, Settings->BaudRate);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SDT: Failed to connect VN-100 on %s — is the device plugged in?"),
            *Settings->OrientationSerialPort);
    }

    bHardwareInitialized = bVN100;
}

void ASDTGameMode::ShutdownHardware()
{
    if (!bHardwareInitialized) return;

    UE_LOG(LogTemp, Log, TEXT("SDT: Shutting down hardware connections..."));
    UVN100BlueprintLibrary::StopVN100();
    bHardwareInitialized = false;
}

// --- Delegate callbacks ---

void ASDTGameMode::OnAssetDestroyed()
{
    UE_LOG(LogTemp, Log, TEXT("SDT: High Value Asset destroyed — Game Over!"));
    EndGame();
}

void ASDTGameMode::OnAllWavesComplete()
{
    // Safety net: ensure gameplay lasted at least MinGameDuration (30s requirement)
    // The wave design naturally exceeds this, but this is an explicit guarantee.
    if (ElapsedGameTime < MinGameDuration)
    {
        bWavesComplete = true;
        UE_LOG(LogTemp, Warning,
            TEXT("SDT: All waves cleared in %.1fs (< %.0fs minimum). "
                 "Waiting for minimum duration before ending."),
            ElapsedGameTime, MinGameDuration);
        // Tick() will call EndGame() once ElapsedGameTime >= MinGameDuration
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("SDT: All waves cleared — Victory!"));
    EndGame();
}

void ASDTGameMode::OnDroneSpawned(ASDTDroneBase* Drone)
{
    // When a new drone spawns, bind to its kill event so we can award score
    if (Drone)
    {
        Drone->OnDroneKilled.AddDynamic(this, &ASDTGameMode::OnDroneKilled);
    }
}

void ASDTGameMode::OnDroneKilled(ASDTDroneBase* Drone)
{
    TotalDronesKilled++;

    // Award score when a drone is killed by the player
    if (ScoreManager)
    {
        ScoreManager->AddKillScore();
    }
}
