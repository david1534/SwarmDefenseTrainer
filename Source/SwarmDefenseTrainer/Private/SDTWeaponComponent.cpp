#include "SDTWeaponComponent.h"
#include "SDTDroneBase.h"
#include "SDTSettings.h"
#include "SDTDefaultSounds.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

USDTWeaponComponent::USDTWeaponComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    FireCooldown = 0.15f;
    TraceRange = 50000.f;
    LastFireTime = -999.f;
}

void USDTWeaponComponent::BeginPlay()
{
    Super::BeginPlay();

    // Apply fire cooldown from project settings (if configured)
    const USDTSettings* Settings = USDTSettings::Get();
    if (Settings)
    {
        FireCooldown = Settings->FireCooldown;
    }

    LastFireTime = -FireCooldown; // Allow immediate first shot
}

bool USDTWeaponComponent::CanFire() const
{
    UWorld* World = GetWorld();
    if (!World) return false;

    return (World->GetTimeSeconds() - LastFireTime) >= FireCooldown;
}

void USDTWeaponComponent::Fire()
{
    if (!CanFire()) return;

    UWorld* World = GetWorld();
    if (!World) return;

    LastFireTime = World->GetTimeSeconds();

    // Play fire sound — use custom if assigned, otherwise default procedural
    if (FireSound)
    {
        UGameplayStatics::PlaySound2D(World, FireSound);
    }
    else
    {
        FSDTDefaultSounds::PlayFire(World);
    }

    OnWeaponFired.Broadcast();

    // Perform the hitscan line trace
    PerformHitscan();
}

void USDTWeaponComponent::PerformHitscan()
{
    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (!PC) return;

    // Trace from camera position in camera forward direction
    FVector CameraLocation;
    FRotator CameraRotation;
    PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

    FVector TraceStart = CameraLocation;
    FVector TraceEnd = CameraLocation + (CameraRotation.Vector() * TraceRange);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(PC->GetPawn());
    QueryParams.bTraceComplex = false;

    bool bHit = World->LineTraceSingleByChannel(
        HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

    if (bHit && HitResult.GetActor())
    {
        // Check if we hit a drone
        ASDTDroneBase* Drone = Cast<ASDTDroneBase>(HitResult.GetActor());
        if (Drone)
        {
            Drone->ApplyDamage(1.f);

            // Play hit confirmation sound — custom or default
            if (HitSound)
            {
                UGameplayStatics::PlaySound2D(World, HitSound);
            }
            else
            {
                FSDTDefaultSounds::PlayHitConfirm(World);
            }
        }

        OnTargetHit.Broadcast(HitResult.GetActor());

        // Spawn hit impact effect
        if (HitEffect)
        {
            UGameplayStatics::SpawnEmitterAtLocation(
                World, HitEffect, HitResult.ImpactPoint,
                HitResult.ImpactNormal.Rotation());
        }
    }
}
