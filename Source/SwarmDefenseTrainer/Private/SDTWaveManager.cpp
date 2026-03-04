#include "SDTWaveManager.h"
#include "SDTDroneBase.h"
#include "SDTHighValueAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ASDTWaveManager::ASDTWaveManager()
{
    PrimaryActorTick.bCanEverTick = true;

    SpawnRadius = 4000.f;
    SpawnMinHeight = 500.f;
    SpawnMaxHeight = 1500.f;
    CurrentWaveIndex = -1;
    DronesRemainingToSpawn = 0;
    SpawnTimer = 0.f;
    WaveDelayTimer = 0.f;
    bIsActive = false;
    bWaitingForWaveDelay = false;
}

void ASDTWaveManager::BeginPlay()
{
    Super::BeginPlay();

    // Find the HVA in the level (drones fly toward this)
    AActor* FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), ASDTHighValueAsset::StaticClass());
    TargetAsset = Cast<ASDTHighValueAsset>(FoundActor);

    if (!TargetAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("SDT WaveManager: No ASDTHighValueAsset found in level!"));
    }

    // Use default wave config if none set in Blueprint
    if (WaveConfigs.Num() == 0)
    {
        InitializeDefaultWaves();
    }
}

void ASDTWaveManager::InitializeDefaultWaves()
{
    // 6 escalating waves — total gameplay ~45-60 seconds minimum
    //                        Count  Speed  Interval  Delay
    WaveConfigs.Add(FWaveConfig(3,   400.f,   2.0f,   3.f));   // Wave 1: easy intro
    WaveConfigs.Add(FWaveConfig(5,   500.f,   1.5f,   5.f));   // Wave 2: ramp up
    WaveConfigs.Add(FWaveConfig(7,   550.f,   1.2f,   5.f));   // Wave 3: moderate
    WaveConfigs.Add(FWaveConfig(8,   600.f,   1.0f,   5.f));   // Wave 4: challenging
    WaveConfigs.Add(FWaveConfig(10,  650.f,   0.8f,   5.f));   // Wave 5: intense
    WaveConfigs.Add(FWaveConfig(12,  700.f,   0.7f,   5.f));   // Wave 6: final push
}

void ASDTWaveManager::StartWaves()
{
    bIsActive = true;
    CurrentWaveIndex = -1;
    StartNextWave();
}

void ASDTWaveManager::StopWaves()
{
    bIsActive = false;

    // Destroy all active drones
    for (ASDTDroneBase* Drone : ActiveDrones)
    {
        if (Drone && IsValid(Drone))
        {
            Drone->Destroy();
        }
    }
    ActiveDrones.Empty();
}

void ASDTWaveManager::StartNextWave()
{
    CurrentWaveIndex++;

    if (CurrentWaveIndex >= WaveConfigs.Num())
    {
        // All waves complete — player survived!
        OnAllWavesComplete.Broadcast();
        bIsActive = false;
        return;
    }

    const FWaveConfig& Config = WaveConfigs[CurrentWaveIndex];
    DronesRemainingToSpawn = Config.DroneCount;
    SpawnTimer = 0.f;
    WaveDelayTimer = Config.DelayBeforeWave;
    bWaitingForWaveDelay = true;

    OnWaveStarted.Broadcast(CurrentWaveIndex + 1, WaveConfigs.Num());
    UE_LOG(LogTemp, Log, TEXT("SDT: Wave %d/%d starting (Count: %d, Speed: %.0f)"),
        CurrentWaveIndex + 1, WaveConfigs.Num(), Config.DroneCount, Config.DroneSpeed);
}

void ASDTWaveManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bIsActive) return;

    // Wait for inter-wave delay
    if (bWaitingForWaveDelay)
    {
        WaveDelayTimer -= DeltaTime;
        if (WaveDelayTimer <= 0.f)
        {
            bWaitingForWaveDelay = false;
        }
        return;
    }

    // Spawn drones at the configured interval
    if (DronesRemainingToSpawn > 0)
    {
        SpawnTimer -= DeltaTime;
        if (SpawnTimer <= 0.f)
        {
            SpawnDrone();
            DronesRemainingToSpawn--;

            if (CurrentWaveIndex < WaveConfigs.Num())
            {
                SpawnTimer = WaveConfigs[CurrentWaveIndex].SpawnInterval;
            }
        }
    }
    else if (ActiveDrones.Num() == 0)
    {
        // All drones in this wave are gone — advance to next wave
        StartNextWave();
    }
}

void ASDTWaveManager::SpawnDrone()
{
    if (!DroneClass)
    {
        // If no drone class is set, use the base class (won't have a mesh, but won't crash)
        DroneClass = ASDTDroneBase::StaticClass();
    }

    if (!TargetAsset) return;

    FVector SpawnLoc = GetRandomSpawnLocation();
    FRotator SpawnRot = FRotator::ZeroRotator;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ASDTDroneBase* Drone = GetWorld()->SpawnActor<ASDTDroneBase>(
        DroneClass, SpawnLoc, SpawnRot, SpawnParams);

    if (Drone)
    {
        Drone->SetTarget(TargetAsset);

        if (CurrentWaveIndex < WaveConfigs.Num())
        {
            Drone->SetMoveSpeed(WaveConfigs[CurrentWaveIndex].DroneSpeed);
        }

        // Bind to drone events for tracking
        Drone->OnDroneKilled.AddDynamic(this, &ASDTWaveManager::OnDroneKilledCallback);
        Drone->OnDroneReachedTarget.AddDynamic(this, &ASDTWaveManager::OnDroneReachedTargetCallback);

        ActiveDrones.Add(Drone);
        OnDroneSpawned.Broadcast(Drone);
    }
}

FVector ASDTWaveManager::GetRandomSpawnLocation() const
{
    FVector Center = TargetAsset ? TargetAsset->GetActorLocation() : FVector::ZeroVector;

    // Random point on a circle around the center, at a random height
    float Angle = FMath::RandRange(0.f, 360.f);
    float Height = FMath::RandRange(SpawnMinHeight, SpawnMaxHeight);

    FVector SpawnLoc;
    SpawnLoc.X = Center.X + SpawnRadius * FMath::Cos(FMath::DegreesToRadians(Angle));
    SpawnLoc.Y = Center.Y + SpawnRadius * FMath::Sin(FMath::DegreesToRadians(Angle));
    SpawnLoc.Z = Center.Z + Height;

    return SpawnLoc;
}

void ASDTWaveManager::OnDroneKilledCallback(ASDTDroneBase* Drone)
{
    RemoveDrone(Drone);
}

void ASDTWaveManager::OnDroneReachedTargetCallback(ASDTDroneBase* Drone)
{
    RemoveDrone(Drone);
}

void ASDTWaveManager::RemoveDrone(ASDTDroneBase* Drone)
{
    ActiveDrones.Remove(Drone);
}
