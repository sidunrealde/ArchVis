using UnrealBuildTool;

public class RTPlanUI : ModuleRules
{
	public RTPlanUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UMG",
				"CommonUI",
				"RTPlanCore",
				"RTPlanTools",
				"RTPlanCatalog" // Added dependency
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}