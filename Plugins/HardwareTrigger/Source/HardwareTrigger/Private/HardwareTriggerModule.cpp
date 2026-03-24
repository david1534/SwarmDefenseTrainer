#include "HardwareTriggerModule.h"
#include "TriggerBlueprintLibrary.h"

#define LOCTEXT_NAMESPACE "FHardwareTriggerModule"

void FHardwareTriggerModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("HardwareTrigger: Plugin loaded"));
}

void FHardwareTriggerModule::ShutdownModule()
{
    UTriggerBlueprintLibrary::StopTrigger();
    UE_LOG(LogTemp, Log, TEXT("HardwareTrigger: Plugin unloaded"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FHardwareTriggerModule, HardwareTrigger)
