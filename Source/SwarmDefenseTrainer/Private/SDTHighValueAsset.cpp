#include "SDTHighValueAsset.h"
#include "SDTDefaultSounds.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

ASDTHighValueAsset::ASDTHighValueAsset()
{
    PrimaryActorTick.bCanEverTick = false;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    // Scale this up in the editor or assign a mesh via Blueprint
    MeshComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    MaxHealth = 100.f;
    CurrentHealth = MaxHealth;
    DamagePerDrone = 10.f;
}

void ASDTHighValueAsset::BeginPlay()
{
    Super::BeginPlay();
    CurrentHealth = MaxHealth;
}

void ASDTHighValueAsset::TakeDamageFromDrone(float Damage)
{
    if (CurrentHealth <= 0.f) return;

    CurrentHealth = FMath::Max(0.f, CurrentHealth - Damage);

    // Play damage sound — custom or default alarm
    if (DamageSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, DamageSound, GetActorLocation());
    }
    else
    {
        FSDTDefaultSounds::PlayAssetDamage(GetWorld(), GetActorLocation());
    }

    OnAssetDamaged.Broadcast(CurrentHealth, MaxHealth);

    if (CurrentHealth <= 0.f)
    {
        OnAssetDestroyed.Broadcast();
        UE_LOG(LogTemp, Log, TEXT("SDT: High Value Asset has been destroyed!"));
    }
}
