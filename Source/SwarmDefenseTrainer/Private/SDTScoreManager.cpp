#include "SDTScoreManager.h"
#include "Kismet/GameplayStatics.h"

void USDTScoreManager::Initialize()
{
    CurrentScore = 0;
    ComboMultiplier = 1;
    LastKillTime = 0.f;
    LoadScores();
}

void USDTScoreManager::AddKillScore()
{
    UpdateCombo();

    int32 Points = BaseKillScore * ComboMultiplier;
    CurrentScore += Points;

    OnScoreChanged.Broadcast(CurrentScore);

    UWorld* World = GetWorld();
    if (World)
    {
        LastKillTime = World->GetTimeSeconds();
    }
}

void USDTScoreManager::ResetScore()
{
    CurrentScore = 0;
    ComboMultiplier = 1;
    LastKillTime = 0.f;
    OnScoreChanged.Broadcast(CurrentScore);
    OnComboChanged.Broadcast(ComboMultiplier);
}

void USDTScoreManager::UpdateCombo()
{
    UWorld* World = GetWorld();
    if (!World) return;

    float CurrentTime = World->GetTimeSeconds();

    if (CurrentTime - LastKillTime <= ComboWindowSeconds && LastKillTime > 0.f)
    {
        ComboMultiplier = FMath::Min(ComboMultiplier + 1, MaxCombo);
    }
    else
    {
        ComboMultiplier = 1;
    }

    OnComboChanged.Broadcast(ComboMultiplier);
}

TArray<FSDTScoreEntry> USDTScoreManager::GetTopScores() const
{
    if (SaveGameInstance)
    {
        return SaveGameInstance->TopScores;
    }
    return TArray<FSDTScoreEntry>();
}

bool USDTScoreManager::IsHighScore() const
{
    if (!SaveGameInstance) return true;
    if (SaveGameInstance->TopScores.Num() < USDTSaveGame::MaxScores) return true;

    for (const FSDTScoreEntry& Entry : SaveGameInstance->TopScores)
    {
        if (CurrentScore > Entry.Score) return true;
    }
    return false;
}

bool USDTScoreManager::TrySubmitScore(const FString& PlayerName)
{
    if (!IsHighScore()) return false;

    if (!SaveGameInstance)
    {
        SaveGameInstance = Cast<USDTSaveGame>(
            UGameplayStatics::CreateSaveGameObject(USDTSaveGame::StaticClass()));
    }

    FSDTScoreEntry NewEntry(PlayerName, CurrentScore);
    SaveGameInstance->TopScores.Add(NewEntry);

    // Sort descending by score
    SaveGameInstance->TopScores.Sort([](const FSDTScoreEntry& A, const FSDTScoreEntry& B) {
        return A.Score > B.Score;
    });

    // Keep only top 10
    while (SaveGameInstance->TopScores.Num() > USDTSaveGame::MaxScores)
    {
        SaveGameInstance->TopScores.RemoveAt(SaveGameInstance->TopScores.Num() - 1);
    }

    SaveScores();
    return true;
}

void USDTScoreManager::LoadScores()
{
    if (UGameplayStatics::DoesSaveGameExist(USDTSaveGame::SaveSlotName, 0))
    {
        SaveGameInstance = Cast<USDTSaveGame>(
            UGameplayStatics::LoadGameFromSlot(USDTSaveGame::SaveSlotName, 0));
    }

    if (!SaveGameInstance)
    {
        SaveGameInstance = Cast<USDTSaveGame>(
            UGameplayStatics::CreateSaveGameObject(USDTSaveGame::StaticClass()));
    }
}

void USDTScoreManager::SaveScores()
{
    if (SaveGameInstance)
    {
        UGameplayStatics::SaveGameToSlot(SaveGameInstance, USDTSaveGame::SaveSlotName, 0);
    }
}
