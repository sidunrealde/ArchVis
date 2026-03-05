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
				"GeometryScriptingCore",
				"GeometryCore",        // For FDynamicMesh3, FDynamicMeshAttributeSet
				"DynamicMesh",         // Additional dynamic mesh support
				"RHI",                 // For GMaxRHIShaderPlatform (Nanite check)
				"RenderCore"           // For shader platform enums
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}