// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ImGuiEditor : ModuleRules
{
	public ImGuiEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
        {
        	"Core",
            "CoreUObject",
			"DeveloperSettings",
            "ImGui",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            // "EditorSubsystem",
            "Engine",
            // "GMCEditor",
            // "InputCore",
            // "SettingsEditor",
            // "Slate",
            // "SlateCore",
            // "ToolMenus",

			// "EditorStyle",
			"Settings",
			"UnrealEd",
        });
    }
}
