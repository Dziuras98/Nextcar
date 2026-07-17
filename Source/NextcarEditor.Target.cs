using UnrealBuildTool;
using System.Collections.Generic;

public class NextcarEditorTarget : TargetRules
{
    public NextcarEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        ExtraModuleNames.Add("Nextcar");
    }
}
