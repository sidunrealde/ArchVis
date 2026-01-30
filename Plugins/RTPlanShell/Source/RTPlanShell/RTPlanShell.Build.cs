using UnrealBuildTool;

public class RTPlanShell : ModuleRules
{
	public RTPlanShell(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"RTPlanCore",
				"RTPlanMeshing",
				"RTPlanMath",
				"RTPlanOpenings",
				"RTPlanCatalog", // For finish catalog
				"GeometryFramework",
				"GeometryScriptingCore"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}