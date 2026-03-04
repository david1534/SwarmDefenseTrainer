#pragma once

#include "CoreMinimal.h"

class USoundWaveProcedural;

/**
 * Generates and plays procedural default sound effects at runtime.
 * Provides audio out-of-the-box even without imported .wav assets.
 *
 * When custom sounds are assigned to components via Blueprint/Editor,
 * those take priority — these defaults only play when no custom sound is set.
 *
 * Sound design:
 *   Fire:      Sharp noise burst with low-frequency body (0.1s)
 *   HitConfirm: High-pitched metallic ping (0.12s)
 *   Explosion: Low rumble with noise (0.3s)
 *   Buzz:      Mid-frequency hum with harmonics (looping)
 *   Damage:    Alternating alarm tones (0.3s)
 *   Beep:      Clean 880Hz countdown tick (0.1s)
 *   GameStart: Rising frequency chirp (0.25s)
 *   GameOver:  Descending frequency chirp (0.4s)
 */
class SWARMDEFENSETRAINER_API FSDTDefaultSounds
{
public:
    // --- One-shot 2D sounds (HUD / weapon feedback) ---
    static void PlayFire(UWorld* World);
    static void PlayHitConfirm(UWorld* World);
    static void PlayCountdownBeep(UWorld* World);
    static void PlayGameStart(UWorld* World);
    static void PlayGameOver(UWorld* World);

    // --- Positional 3D sounds (world-space) ---
    static void PlayDroneDeath(UWorld* World, FVector Location);
    static void PlayAssetDamage(UWorld* World, FVector Location);

    /**
     * Create a fresh looping buzz sound for a drone.
     * Returns a new USoundWaveProcedural that loops indefinitely.
     * Use with UGameplayStatics::SpawnSoundAttached().
     * Each drone should get its own instance.
     */
    static USoundWaveProcedural* CreateDroneBuzzSound();

private:
    /** Sound definition — cached PCM data + duration */
    struct FSoundDef
    {
        TArray<uint8> PCMData;
        float Duration = 0.f;
    };

    /** Sound type keys */
    enum ESoundID
    {
        SND_Fire,
        SND_Hit,
        SND_Explosion,
        SND_Damage,
        SND_Beep,
        SND_Start,
        SND_Over,
        SND_Buzz,
        SND_COUNT
    };

    static void EnsureInitialized();
    static void PlayOneShot(UWorld* World, ESoundID ID, FVector Location, bool b2D, float VolumeScale = 1.f);

    /**
     * Core PCM generator — calls the provided lambda for each sample.
     * Lambda signature: float(float timeSec, int32 sampleIndex)
     * Should return a value in [-1.0, 1.0].
     */
    static TArray<uint8> GeneratePCM(float DurationSec, TFunctionRef<float(float, int32)> Generator);

    /** Deterministic noise function (hash-based, no rand()) */
    static float HashNoise(int32 Index);

    static FSoundDef Defs[SND_COUNT];
    static bool bInitialized;
    static constexpr int32 SampleRate = 44100;
};
