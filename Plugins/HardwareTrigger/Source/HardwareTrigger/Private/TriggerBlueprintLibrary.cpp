#include "TriggerBlueprintLibrary.h"
#include "TriggerSerialReader.h"

// Singleton reader instance
static TUniquePtr<FTriggerSerialReader> GTriggerReader;

bool UTriggerBlueprintLibrary::StartTrigger(const FString& PortName, int32 BaudRate)
{
    StopTrigger();

    GTriggerReader = MakeUnique<FTriggerSerialReader>();

    if (GTriggerReader->Start(PortName, BaudRate))
    {
        UE_LOG(LogTemp, Log, TEXT("Trigger: Started on %s"), *PortName);
        return true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Trigger: Failed to start on %s"), *PortName);
        GTriggerReader.Reset();
        return false;
    }
}

void UTriggerBlueprintLibrary::StopTrigger()
{
    if (GTriggerReader)
    {
        GTriggerReader->Stop();
        GTriggerReader.Reset();
    }
}

bool UTriggerBlueprintLibrary::IsTriggerPressed()
{
    return GTriggerReader && GTriggerReader->IsTriggerPressed();
}

bool UTriggerBlueprintLibrary::ConsumeTriggerPress()
{
    return GTriggerReader && GTriggerReader->ConsumeTriggerPress();
}

bool UTriggerBlueprintLibrary::IsTriggerConnected()
{
    return GTriggerReader && GTriggerReader->IsConnected();
}
