#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SDTSaveGame.generated.h"

/** A single entry in the Top-10 scoreboard */
USTRUCT(BlueprintType)
struct FSDTScoreEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString PlayerName;

    UPROPERTY(BlueprintReadOnly)
    int32 Score;

    UPROPERTY(BlueprintReadOnly)
    FDateTime Timestamp;

    FSDTScoreEntry() : Score(0) {}
    FSDTScoreEntry(const FString& Name, int32 InScore)
        : PlayerName(Name), Score(InScore), Timestamp(FDateTime::Now()) {}
};

/**
 * Persistent save game for the Top-10 scoreboard.
 * Uses UE5's built-in SaveGame system — auto-serialized to disk.
 */
UCLASS()
class SWARMDEFENSETRAINER_API USDTSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TArray<FSDTScoreEntry> TopScores;

    static constexpr int32 MaxScores = 10;
    static const FString SaveSlotName;
};
