using UnrealBuildTool;

public class RTPlanTools : ModuleRules
{
	public RTPlanTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"RTPlanCore",
				"RTPlanInput",
				"RTPlanSpatial",
				"RTPlanMath", // Added dependency for geometry utils
				"RTPlanCatalog",
				"RTPlanObjects"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}