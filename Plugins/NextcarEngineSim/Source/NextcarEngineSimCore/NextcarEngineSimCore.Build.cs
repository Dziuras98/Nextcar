using UnrealBuildTool;

public class NextcarEngineSimCore : ModuleRules
{
    public NextcarEngineSimCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.Add("Core");
    }
}
