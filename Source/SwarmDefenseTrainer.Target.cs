using UnrealBuildTool;
using System.Collections.Generic;

public class SwarmDefenseTrainerTarget : TargetRules
{
    public SwarmDefenseTrainerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.Add("SwarmDefenseTrainer");
    }
}
