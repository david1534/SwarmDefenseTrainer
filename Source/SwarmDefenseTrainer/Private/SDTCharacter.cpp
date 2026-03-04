#include "SDTCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SDTWeaponComponent.h"

ASDTCharacter::ASDTCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    // Capsule size
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

    // First-person camera — attached directly to capsule, follows controller rotation
    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
    FirstPersonCamera->bUsePawnControlRotation = true;

    // Weapon mesh — attached to camera so it moves with the player's view
    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(FirstPersonCamera);
    WeaponMesh->SetRelativeLocation(FVector(50.f, 25.f, -15.f));
    WeaponMesh->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
    WeaponMesh->CastShadow = false;

    // Weapon component — handles shooting logic (hitscan, cooldown, effects)
    WeaponComponent = CreateDefaultSubobject<USDTWeaponComponent>(TEXT("WeaponComponent"));

    // Player is STATIONARY — only the camera/aim rotates
    bUseControllerRotationYaw = true;
    bUseControllerRotationPitch = true;
    bUseControllerRotationRoll = false;

    // Disable all character movement
    UCharacterMovementComponent* Movement = GetCharacterMovement();
    if (Movement)
    {
        Movement->GravityScale = 0.f;
        Movement->MaxWalkSpeed = 0.f;
        Movement->bOrientRotationToMovement = false;
    }
}

void ASDTCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void ASDTCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    // All input is handled by SDTPlayerController, not the character directly
}
