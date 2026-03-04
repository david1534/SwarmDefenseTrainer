#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SDTDroneBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDroneKilled, ASDTDroneBase*, Drone);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDroneReachedTarget, ASDTDroneBase*, Drone);

/**
 * Base drone enemy actor.
 * Spawned by WaveManager, flies toward the High Value Asset.
 * Can be killed by the player's hitscan weapon.
 *
 * Create a Blueprint child of this class to assign mesh, sounds, and VFX.
 */
UCLASS()
class SWARMDEFENSETRAINER_API ASDTDroneBase : public AActor
{
    GENERATED_BODY()

public:
    ASDTDroneBase();

    virtual void Tick(float DeltaTime) override;

    /** Called by WeaponComponent when a hitscan ray hits this drone */
    UFUNCTION(BlueprintCallable, Category="Combat")
    void ApplyDamage(float Damage);

    /** Set the actor this drone flies toward */
    void SetTarget(AActor* InTarget);

    /** Override the default move speed (called by WaveManager per wave) */
    void SetMoveSpeed(float Speed) { MoveSpeed = Speed; }

    /** Fired when this drone is killed by the player */
    UPROPERTY(BlueprintAssignable, Category="Events")
    FOnDroneKilled OnDroneKilled;

    /** Fired when this drone reaches its target without being killed */
    UPROPERTY(BlueprintAssignable, Category="Events")
    FOnDroneReachedTarget OnDroneReachedTarget;

protected:
    virtual void BeginPlay() override;
    virtual void OnDeath();
    virtual void OnReachedTarget();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UStaticMeshComponent* DroneMesh;

    /** How fast the drone moves toward the target (units/sec) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
    float MoveSpeed;

    /** Amplitude of side-to-side weaving motion */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
    float WeaveAmplitude;

    /** Frequency of the weaving oscillation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
    float WeaveFrequency;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
    float Health;

    /** How close the drone must get to the target to count as "arrived" */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
    float ArrivalDistance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
    USoundBase* DeathSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
    USoundBase* FlyingSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
    UParticleSystem* DeathEffect;

    UPROPERTY()
    AActor* TargetActor;

private:
    float WeaveTimeOffset;
    bool bIsDead;

    UPROPERTY()
    UAudioComponent* FlyingSoundComponent;
};
