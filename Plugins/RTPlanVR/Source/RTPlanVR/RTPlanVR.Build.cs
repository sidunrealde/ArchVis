using UnrealBuildTool;

public class RTPlanVR : ModuleRules
{
	public RTPlanVR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"HeadMountedDisplay",
				"XRBase",
				"UMG", // Added for WidgetInteractionComponent
				"RTPlanCore",
				"RTPlanInput",
				"RTPlanUI"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}