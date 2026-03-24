#include "VN100InputModule.h"
#include "VN100BlueprintLibrary.h"

#define LOCTEXT_NAMESPACE "FVN100InputModule"

void FVN100InputModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("VN100Input: Plugin loaded"));
}

void FVN100InputModule::ShutdownModule()
{
    UVN100BlueprintLibrary::StopVN100();
    UE_LOG(LogTemp, Log, TEXT("VN100Input: Plugin unloaded"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FVN100InputModule, VN100Input)
