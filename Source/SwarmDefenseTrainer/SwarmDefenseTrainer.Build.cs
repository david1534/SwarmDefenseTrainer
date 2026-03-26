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
            "DeveloperSettings",
            "VN100Input"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });
    }
}
