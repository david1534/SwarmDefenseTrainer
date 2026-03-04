#include "SDTDefaultSounds.h"
#include "Sound/SoundWaveProcedural.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

// Static member initialization
FSDTDefaultSounds::FSoundDef FSDTDefaultSounds::Defs[SND_COUNT];
bool FSDTDefaultSounds::bInitialized = false;

// ============================================================
//  Deterministic noise — hash-based, no FMath::Rand()
// ============================================================

float FSDTDefaultSounds::HashNoise(int32 Index)
{
    uint32 x = (uint32)(Index + 1) * 1103515245u + 12345u;
    x = ((x >> 16) ^ x) * 45679u;
    x = ((x >> 16) ^ x);
    return ((float)(x & 0xFFFF) / 32767.5f) - 1.f;
}

// ============================================================
//  Core PCM generator
// ============================================================

TArray<uint8> FSDTDefaultSounds::GeneratePCM(float DurationSec, TFunctionRef<float(float, int32)> Generator)
{
    int32 NumSamples = (int32)(SampleRate * DurationSec);
    TArray<uint8> Data;
    Data.SetNumZeroed(NumSamples * sizeof(int16));

    int16* Samples = reinterpret_cast<int16*>(Data.GetData());
    for (int32 i = 0; i < NumSamples; i++)
    {
        float t = (float)i / (float)SampleRate;
        float Value = FMath::Clamp(Generator(t, i), -1.f, 1.f);
        Samples[i] = static_cast<int16>(Value * 32600.f); // slightly below max to avoid clipping
    }

    return Data;
}

// ============================================================
//  Sound generation — called once, cached forever
// ============================================================

void FSDTDefaultSounds::EnsureInitialized()
{
    if (bInitialized) return;
    bInitialized = true;

    // --- FIRE (0.1s): Noise burst + low tone, aggressive decay ---
    Defs[SND_Fire].Duration = 0.1f;
    Defs[SND_Fire].PCMData = GeneratePCM(0.1f, [](float t, int32 i) -> float
    {
        float Decay = FMath::Exp(-t * 40.f);
        float Noise = HashNoise(i);
        float LowTone = FMath::Sin(2.f * PI * 120.f * t);
        return (Noise * 0.6f + LowTone * 0.4f) * Decay * 0.8f;
    });

    // --- HIT CONFIRM (0.12s): High metallic ping ---
    Defs[SND_Hit].Duration = 0.12f;
    Defs[SND_Hit].PCMData = GeneratePCM(0.12f, [](float t, int32 i) -> float
    {
        float Env = FMath::Exp(-t * 25.f);
        float Tone = FMath::Sin(2.f * PI * 1500.f * t);
        float Overtone = FMath::Sin(2.f * PI * 3000.f * t) * 0.3f;
        return (Tone + Overtone) * Env * 0.5f;
    });

    // --- DRONE DEATH (0.3s): Low explosion rumble ---
    Defs[SND_Explosion].Duration = 0.3f;
    Defs[SND_Explosion].PCMData = GeneratePCM(0.3f, [](float t, int32 i) -> float
    {
        float Decay = FMath::Exp(-t * 8.f);
        float Noise = HashNoise(i) * 0.5f;
        float Low1 = FMath::Sin(2.f * PI * 80.f * t) * 0.3f;
        float Low2 = FMath::Sin(2.f * PI * 160.f * t) * 0.2f;
        return (Noise + Low1 + Low2) * Decay;
    });

    // --- ASSET DAMAGE (0.3s): Quick alarm warble ---
    Defs[SND_Damage].Duration = 0.3f;
    Defs[SND_Damage].PCMData = GeneratePCM(0.3f, [](float t, int32 i) -> float
    {
        float Env = FMath::Min(t * 20.f, 1.f) * FMath::Max(1.f - (t / 0.3f), 0.f);
        float Freq = (t < 0.15f) ? 800.f : 600.f;
        return FMath::Sin(2.f * PI * Freq * t) * Env * 0.6f;
    });

    // --- COUNTDOWN BEEP (0.1s): Clean 880Hz tick ---
    Defs[SND_Beep].Duration = 0.1f;
    Defs[SND_Beep].PCMData = GeneratePCM(0.1f, [](float t, int32 i) -> float
    {
        float Env = FMath::Min(t * 50.f, 1.f) * FMath::Clamp(1.f - (t - 0.06f) * 25.f, 0.f, 1.f);
        return FMath::Sin(2.f * PI * 880.f * t) * Env * 0.5f;
    });

    // --- GAME START (0.25s): Rising chirp 400→1600Hz ---
    Defs[SND_Start].Duration = 0.25f;
    Defs[SND_Start].PCMData = GeneratePCM(0.25f, [](float t, int32 i) -> float
    {
        float Freq = FMath::Lerp(400.f, 1600.f, t / 0.25f);
        float Env = FMath::Min(t * 20.f, 1.f) * FMath::Clamp(1.f - (t - 0.18f) * 14.f, 0.f, 1.f);
        return FMath::Sin(2.f * PI * Freq * t) * Env * 0.5f;
    });

    // --- GAME OVER (0.4s): Descending chirp 800→200Hz ---
    Defs[SND_Over].Duration = 0.4f;
    Defs[SND_Over].PCMData = GeneratePCM(0.4f, [](float t, int32 i) -> float
    {
        float Freq = FMath::Lerp(800.f, 200.f, t / 0.4f);
        float Env = FMath::Min(t * 20.f, 1.f) * FMath::Clamp(1.f - (t - 0.3f) * 10.f, 0.f, 1.f);
        return FMath::Sin(2.f * PI * Freq * t) * Env * 0.6f;
    });

    // --- DRONE BUZZ (1.0s, designed for looping): Mid-frequency hum ---
    Defs[SND_Buzz].Duration = 1.0f;
    Defs[SND_Buzz].PCMData = GeneratePCM(1.0f, [](float t, int32 i) -> float
    {
        float Fundamental = FMath::Sin(2.f * PI * 180.f * t);
        float H2 = FMath::Sin(2.f * PI * 360.f * t) * 0.5f;
        float H3 = FMath::Sin(2.f * PI * 540.f * t) * 0.25f;
        float Noise = HashNoise(i) * 0.08f;
        return (Fundamental + H2 + H3 + Noise) * 0.25f;
    });

    UE_LOG(LogTemp, Log, TEXT("SDT: Default procedural sounds initialized (%d types)"), (int32)SND_COUNT);
}

// ============================================================
//  Playback — creates a temporary USoundWaveProcedural per call
// ============================================================

void FSDTDefaultSounds::PlayOneShot(UWorld* World, ESoundID ID, FVector Location, bool b2D, float VolumeScale)
{
    EnsureInitialized();

    const FSoundDef& Def = Defs[ID];
    if (!World || Def.PCMData.Num() == 0) return;

    // Create a temporary procedural wave — the AudioComponent holds a ref
    // during playback, preventing GC. After playback it gets collected.
    USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>();
    Wave->SetSampleRate(SampleRate);
    Wave->NumChannels = 1;
    Wave->Duration = Def.Duration;
    Wave->SoundGroup = SOUNDGROUP_Effects;
    Wave->bLooping = false;
    Wave->QueueAudio(Def.PCMData.GetData(), Def.PCMData.Num());

    if (b2D)
    {
        UGameplayStatics::PlaySound2D(World, Wave, VolumeScale);
    }
    else
    {
        UGameplayStatics::PlaySoundAtLocation(World, Wave, Location, VolumeScale);
    }
}

// ============================================================
//  Public play functions
// ============================================================

void FSDTDefaultSounds::PlayFire(UWorld* World)
{
    PlayOneShot(World, SND_Fire, FVector::ZeroVector, true, 0.7f);
}

void FSDTDefaultSounds::PlayHitConfirm(UWorld* World)
{
    PlayOneShot(World, SND_Hit, FVector::ZeroVector, true, 0.6f);
}

void FSDTDefaultSounds::PlayDroneDeath(UWorld* World, FVector Location)
{
    PlayOneShot(World, SND_Explosion, Location, false, 0.8f);
}

void FSDTDefaultSounds::PlayAssetDamage(UWorld* World, FVector Location)
{
    PlayOneShot(World, SND_Damage, Location, false, 0.7f);
}

void FSDTDefaultSounds::PlayCountdownBeep(UWorld* World)
{
    PlayOneShot(World, SND_Beep, FVector::ZeroVector, true, 0.6f);
}

void FSDTDefaultSounds::PlayGameStart(UWorld* World)
{
    PlayOneShot(World, SND_Start, FVector::ZeroVector, true, 0.7f);
}

void FSDTDefaultSounds::PlayGameOver(UWorld* World)
{
    PlayOneShot(World, SND_Over, FVector::ZeroVector, true, 0.8f);
}

// ============================================================
//  Looping drone buzz — one instance per drone
// ============================================================

USoundWaveProcedural* FSDTDefaultSounds::CreateDroneBuzzSound()
{
    EnsureInitialized();

    const FSoundDef& Def = Defs[SND_Buzz];
    if (Def.PCMData.Num() == 0) return nullptr;

    USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>();
    Wave->SetSampleRate(SampleRate);
    Wave->NumChannels = 1;
    Wave->Duration = 10000.f; // Large value — sound loops until stopped
    Wave->SoundGroup = SOUNDGROUP_Effects;
    Wave->bLooping = true;
    Wave->QueueAudio(Def.PCMData.GetData(), Def.PCMData.Num());

    // Re-queue buzz data when buffer empties so the sound loops seamlessly
    Wave->OnSoundWaveProceduralUnderflow.BindLambda(
        [BuzzData = Def.PCMData](USoundWaveProcedural* InWave, int32 SamplesNeeded)
        {
            if (BuzzData.Num() > 0)
            {
                InWave->QueueAudio(BuzzData.GetData(), BuzzData.Num());
            }
        });

    return Wave;
}
