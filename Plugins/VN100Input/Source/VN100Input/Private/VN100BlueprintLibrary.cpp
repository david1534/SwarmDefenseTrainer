#include "VN100BlueprintLibrary.h"
#include "VN100OrientationReader.h"

// Singleton reader instance — lives for the duration of the application
static TUniquePtr<FVN100OrientationReader> GVN100Reader;

bool UVN100BlueprintLibrary::StartVN100(const FString& PortName, int32 BaudRate)
{
    // Stop any existing reader first
    StopVN100();

    GVN100Reader = MakeUnique<FVN100OrientationReader>();

    if (GVN100Reader->Start(PortName, BaudRate))
    {
        UE_LOG(LogTemp, Log, TEXT("VN100: Started on %s"), *PortName);
        return true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("VN100: Failed to start on %s"), *PortName);
        GVN100Reader.Reset();
        return false;
    }
}

void UVN100BlueprintLibrary::StopVN100()
{
    if (GVN100Reader)
    {
        GVN100Reader->Stop();
        GVN100Reader.Reset();
        UE_LOG(LogTemp, Log, TEXT("VN100: Stopped"));
    }
}

FRotator UVN100BlueprintLibrary::GetVN100Orientation()
{
    if (GVN100Reader && GVN100Reader->IsConnected())
    {
        return GVN100Reader->GetOrientation();
    }
    return FRotator::ZeroRotator;
}

bool UVN100BlueprintLibrary::IsVN100Connected()
{
    return GVN100Reader && GVN100Reader->IsConnected();
}
