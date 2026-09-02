using UnrealBuildTool;

public class GravityShiftCore : ModuleRules
{
    public GravityShiftCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "PhysicsCore"
            }
        );
    }
}
