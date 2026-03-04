#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SDTHighValueAsset.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAssetDestroyed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAssetDamaged, float, CurrentHealth, float, MaxHealth);

/**
 * The thing the player is defending.
 * Place one of these in the level — drones fly toward it.
 * When its health reaches 0, the game ends.
 */
UCLASS()
class SWARMDEFENSETRAINER_API ASDTHighValueAsset : public AActor
{
    GENERATED_BODY()

public:
    ASDTHighValueAsset();

    /** Called by drones when they reach this asset */
    UFUNCTION(BlueprintCallable, Category="Damage")
    void TakeDamageFromDrone(float Damage);

    UFUNCTION(BlueprintPure, Category="Health")
    float GetHealthPercent() const { return MaxHealth > 0.f ? CurrentHealth / MaxHealth : 0.f; }

    UFUNCTION(BlueprintPure, Category="Health")
    float GetCurrentHealth() const { return CurrentHealth; }

    UFUNCTION(BlueprintPure, Category="Health")
    bool IsDestroyed() const { return CurrentHealth <= 0.f; }

    UFUNCTION(BlueprintPure, Category="Health")
    float GetDamagePerDrone() const { return DamagePerDrone; }

    /** Fired when health reaches 0 */
    UPROPERTY(BlueprintAssignable, Category="Events")
    FOnAssetDestroyed OnAssetDestroyed;

    /** Fired every time the asset takes damage */
    UPROPERTY(BlueprintAssignable, Category="Events")
    FOnAssetDamaged OnAssetDamaged;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health")
    float MaxHealth;

    UPROPERTY(BlueprintReadOnly, Category="Health")
    float CurrentHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health")
    float DamagePerDrone;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
    USoundBase* DamageSound;
};
