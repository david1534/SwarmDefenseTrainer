#include "HardwareTriggerModule.h"

#define LOCTEXT_NAMESPACE "FHardwareTriggerModule"

void FHardwareTriggerModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("HardwareTrigger: Plugin loaded"));
}

void FHardwareTriggerModule::ShutdownModule()
{
    UE_LOG(LogTemp, Log, TEXT("HardwareTrigger: Plugin unloaded"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FHardwareTriggerModule, HardwareTrigger)
