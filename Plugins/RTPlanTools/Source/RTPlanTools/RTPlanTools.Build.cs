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
				"RTPlanCatalog", // Added dependency
				"RTPlanObjects"  // Added dependency
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}