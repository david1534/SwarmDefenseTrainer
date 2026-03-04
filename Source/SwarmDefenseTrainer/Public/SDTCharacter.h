#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SDTCharacter.generated.h"

class UCameraComponent;
class USDTWeaponComponent;

/**
 * First-person stationary player character.
 * The player does NOT move — only the camera/aim rotates.
 * Camera rotation is driven by SDTPlayerController (mouse or VN-100).
 */
UCLASS()
class SWARMDEFENSETRAINER_API ASDTCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ASDTCharacter();

    /** First-person camera attached to the capsule, follows controller rotation */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    UCameraComponent* FirstPersonCamera;

    /** Weapon mesh visible in first-person view, attached to the camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    UStaticMeshComponent* WeaponMesh;

    /** Weapon logic component — handles hitscan firing, cooldowns, effects */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon")
    USDTWeaponComponent* WeaponComponent;

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
};
