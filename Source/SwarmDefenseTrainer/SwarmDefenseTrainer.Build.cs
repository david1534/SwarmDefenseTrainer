using UnrealBuildTool;

public class SwarmDefenseTrainer : ModuleRules
{
    public SwarmDefenseTrainer(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "UMG",
            "Slate",
            "SlateCore",
            "VN100Input",
            "HardwareTrigger"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });
    }
}
