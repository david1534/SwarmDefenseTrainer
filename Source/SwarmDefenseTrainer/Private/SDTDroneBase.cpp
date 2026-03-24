#include "SDTDroneBase.h"
#include "SDTHighValueAsset.h"
#include "SDTDefaultSounds.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Kismet/GameplayStatics.h"

ASDTDroneBase::ASDTDroneBase()
{
    PrimaryActorTick.bCanEverTick = true;

    DroneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DroneMesh"));
    RootComponent = DroneMesh;

    // Must block line traces for hitscan detection
    DroneMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    DroneMesh->SetGenerateOverlapEvents(true);

    MoveSpeed = 500.f;
    WeaveAmplitude = 50.f;
    WeaveFrequency = 2.f;
    Health = 1.f;
    ArrivalDistance = 200.f;
    bIsDead = false;
    WeaveTimeOffset = 0.f;
    FlyingSoundComponent = nullptr;
}

void ASDTDroneBase::BeginPlay()
{
    Super::BeginPlay();

    // Random phase offset so drones don't all weave in sync
    WeaveTimeOffset = FMath::RandRange(0.f, 6.28f);

    // Attach looping flight sound — use custom if assigned, otherwise default buzz
    USoundBase* BuzzToPlay = FlyingSound;
    if (!BuzzToPlay)
    {
        BuzzToPlay = FSDTDefaultSounds::CreateDroneBuzzSound();
    }

    if (BuzzToPlay)
    {
        FlyingSoundComponent = UGameplayStatics::SpawnSoundAttached(
            BuzzToPlay, RootComponent, NAME_None,
            FVector::ZeroVector, EAttachLocation::KeepRelativeOffset,
            false, 1.f, 1.f, 0.f, nullptr, nullptr, true);
    }
}

void ASDTDroneBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsDead || !TargetActor) return;

    FVector CurrentLocation = GetActorLocation();
    FVector TargetLocation = TargetActor->GetActorLocation();
    FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal();

    // Move toward target
    FVector Movement = Direction * MoveSpeed * DeltaTime;

    // Add perpendicular weaving motion for unpredictability
    float Time = GetWorld()->GetTimeSeconds() + WeaveTimeOffset;
    FVector Right = FVector::CrossProduct(Direction, FVector::UpVector).GetSafeNormal();
    FVector Weave = Right * FMath::Sin(Time * WeaveFrequency) * WeaveAmplitude * DeltaTime;

    SetActorLocation(CurrentLocation + Movement + Weave);

    // Face direction of travel
    SetActorRotation(Direction.Rotation());

    // Check if reached target
    float DistToTarget = FVector::Dist(GetActorLocation(), TargetLocation);
    if (DistToTarget <= ArrivalDistance)
    {
        OnReachedTarget();
    }
}

void ASDTDroneBase::SetTarget(AActor* InTarget)
{
    TargetActor = InTarget;
}

void ASDTDroneBase::ApplyDamage(float Damage)
{
    if (bIsDead) return;

    Health -= Damage;
    if (Health <= 0.f)
    {
        OnDeath();
    }
}

void ASDTDroneBase::OnDeath()
{
    bIsDead = true;

    // Play death sound — custom or default procedural explosion
    if (DeathSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
    }
    else
    {
        FSDTDefaultSounds::PlayDroneDeath(GetWorld(), GetActorLocation());
    }

    if (DeathEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(), DeathEffect, GetActorLocation(), FRotator::ZeroRotator, true);
    }

    // Stop flight sound
    if (FlyingSoundComponent)
    {
        FlyingSoundComponent->Stop();
    }

    OnDroneKilled.Broadcast(this);
    Destroy();
}

void ASDTDroneBase::OnReachedTarget()
{
    bIsDead = true;

    // Deal damage to the HVA using its configured DamagePerDrone value
    ASDTHighValueAsset* HVA = Cast<ASDTHighValueAsset>(TargetActor);
    if (HVA)
    {
        HVA->TakeDamageFromDrone(HVA->GetDamagePerDrone());
    }

    if (FlyingSoundComponent)
    {
        FlyingSoundComponent->Stop();
    }

    OnDroneReachedTarget.Broadcast(this);
    Destroy();
}
