using UnrealBuildTool;

public class NextcarEngineSimRuntime : ModuleRules
{
    public NextcarEngineSimRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.Add("Core");
        PrivateDependencyModuleNames.Add("NextcarEngineSimCore");
    }
}
