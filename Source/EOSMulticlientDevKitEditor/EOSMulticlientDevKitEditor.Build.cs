// Copyright (c) 2026 Ivan "official-rimacc" Rimac. Source-available; see LICENSE.

using UnrealBuildTool;

public class EOSMulticlientDevKitEditor : ModuleRules
{
	public EOSMulticlientDevKitEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Slate",
			"SlateCore",
			"ToolMenus",
			"Projects",
			"InputCore",
			"LevelEditor",
			"WorkspaceMenuStructure",
			"DesktopPlatform"
		});
	}
}
