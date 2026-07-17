using UnrealBuildTool;

public class Nextcar : ModuleRules
{
    public Nextcar(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore"
            });

        if (Target.bBuildDeveloperTools)
        {
            PrivateDefinitions.Add("WITH_NEXTCAR_AUTOMATION_TESTS=1");
        }
        else
        {
            PrivateDefinitions.Add("WITH_NEXTCAR_AUTOMATION_TESTS=0");
        }
    }
}
