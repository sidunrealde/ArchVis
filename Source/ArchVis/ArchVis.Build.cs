// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ArchVis : ModuleRules
{
	public ArchVis(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
			// Plugin Dependencies
			"RTPlanCore",
			"RTPlanTools",
			"RTPlanShell",
			"RTPlanInput",
			"RTPlanNet"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });
	}
}