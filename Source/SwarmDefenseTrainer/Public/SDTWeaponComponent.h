#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SDTWeaponComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponFired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetHit, AActor*, HitActor);

/**
 * Hitscan weapon component — handles firing, cooldowns, hit detection, and effects.
 * Attached to SDTCharacter. Fires an instant line trace from the camera center.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SWARMDEFENSETRAINER_API USDTWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USDTWeaponComponent();

    /** Fire the weapon — performs hitscan, plays effects, respects cooldown */
    UFUNCTION(BlueprintCallable, Category="Weapon")
    void Fire();

    /** Check if enough time has passed since last shot */
    UFUNCTION(BlueprintPure, Category="Weapon")
    bool CanFire() const;

    /** Fired every time the weapon shoots */
    UPROPERTY(BlueprintAssignable, Category="Weapon")
    FOnWeaponFired OnWeaponFired;

    /** Fired when a hitscan ray hits any actor */
    UPROPERTY(BlueprintAssignable, Category="Weapon")
    FOnTargetHit OnTargetHit;

    /** Minimum time between shots (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
    float FireCooldown;

    /** Maximum range of the hitscan ray (units) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
    float TraceRange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon|Effects")
    USoundBase* FireSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon|Effects")
    USoundBase* HitSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon|Effects")
    UParticleSystem* MuzzleFlashEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon|Effects")
    UParticleSystem* HitEffect;

protected:
    virtual void BeginPlay() override;

private:
    void PerformHitscan();

    float LastFireTime;
};
