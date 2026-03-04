#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SDTGameInstance.generated.h"

/**
 * Game instance that persists across level loads.
 * Stores player name for the scoreboard.
 */
UCLASS()
class SWARMDEFENSETRAINER_API USDTGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void Init() override;

    UFUNCTION(BlueprintPure, Category="Player")
    FString GetPlayerName() const { return PlayerName; }

    UFUNCTION(BlueprintCallable, Category="Player")
    void SetPlayerName(const FString& Name) { PlayerName = Name; }

private:
    FString PlayerName;
};
