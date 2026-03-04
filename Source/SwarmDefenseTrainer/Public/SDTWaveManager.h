#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SDTWaveManager.generated.h"

class ASDTDroneBase;
class ASDTHighValueAsset;

/** Configuration for a single wave of drones */
USTRUCT(BlueprintType)
struct FWaveConfig
{
    GENERATED_BODY()

    /** Number of drones in this wave */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 DroneCount;

    /** Movement speed of drones in this wave */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DroneSpeed;

    /** Seconds between each drone spawn */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpawnInterval;

    /** Seconds to wait before this wave begins */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DelayBeforeWave;

    FWaveConfig()
        : DroneCount(3), DroneSpeed(400.f), SpawnInterval(2.f), DelayBeforeWave(3.f) {}

    FWaveConfig(int32 Count, float Speed, float Interval, float Delay)
        : DroneCount(Count), DroneSpeed(Speed), SpawnInterval(Interval), DelayBeforeWave(Delay) {}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWaveStarted, int32, WaveNumber, int32, TotalWaves);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllWavesComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDroneSpawned, ASDTDroneBase*, Drone);

/**
 * Manages the spawning of drones across multiple escalating waves.
 * Spawned by SDTGameMode at game start.
 *
 * Default waves: 6 waves from 3 drones @ 400 speed to 12 drones @ 700 speed.
 * Customize by setting WaveConfigs in a Blueprint child class.
 */
UCLASS()
class SWARMDEFENSETRAINER_API ASDTWaveManager : public AActor
{
    GENERATED_BODY()

public:
    ASDTWaveManager();

    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category="Waves")
    void StartWaves();

    UFUNCTION(BlueprintCallable, Category="Waves")
    void StopWaves();

    UFUNCTION(BlueprintPure, Category="Waves")
    int32 GetCurrentWave() const { return CurrentWaveIndex + 1; }

    UFUNCTION(BlueprintPure, Category="Waves")
    int32 GetTotalWaves() const { return WaveConfigs.Num(); }

    UFUNCTION(BlueprintPure, Category="Waves")
    int32 GetDronesRemainingInWave() const { return DronesRemainingToSpawn + ActiveDrones.Num(); }

    UPROPERTY(BlueprintAssignable, Category="Events")
    FOnWaveStarted OnWaveStarted;

    UPROPERTY(BlueprintAssignable, Category="Events")
    FOnAllWavesComplete OnAllWavesComplete;

    UPROPERTY(BlueprintAssignable, Category="Events")
    FOnDroneSpawned OnDroneSpawned;

protected:
    virtual void BeginPlay() override;

    /** Wave definitions — override in Blueprint for custom wave progressions */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Waves")
    TArray<FWaveConfig> WaveConfigs;

    /** The drone Blueprint class to spawn (must be a child of ASDTDroneBase) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning")
    TSubclassOf<ASDTDroneBase> DroneClass;

    /** Spawn radius around the HVA (units) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning")
    float SpawnRadius;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning")
    float SpawnMinHeight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawning")
    float SpawnMaxHeight;

private:
    void InitializeDefaultWaves();
    void StartNextWave();
    void SpawnDrone();
    FVector GetRandomSpawnLocation() const;

    UFUNCTION()
    void OnDroneKilledCallback(ASDTDroneBase* Drone);

    UFUNCTION()
    void OnDroneReachedTargetCallback(ASDTDroneBase* Drone);

    void RemoveDrone(ASDTDroneBase* Drone);

    UPROPERTY()
    ASDTHighValueAsset* TargetAsset;

    UPROPERTY()
    TArray<ASDTDroneBase*> ActiveDrones;

    int32 CurrentWaveIndex;
    int32 DronesRemainingToSpawn;
    float SpawnTimer;
    float WaveDelayTimer;
    bool bIsActive;
    bool bWaitingForWaveDelay;
};
