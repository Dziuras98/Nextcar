using UnrealBuildTool;
using System.Collections.Generic;

public class NextcarTarget : TargetRules
{
    public NextcarTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        ExtraModuleNames.Add("Nextcar");
    }
}
