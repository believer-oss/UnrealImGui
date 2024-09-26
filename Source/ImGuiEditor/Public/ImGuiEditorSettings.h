#include "Engine/DeveloperSettings.h"
#include "ImGuiModuleSettingsInterface.h"
#include "ImGuiEditorSettings.generated.h"

#pragma once

UCLASS(config=EditorPerProjectUserSettings)
class IMGUIEDITOR_API UImGuiEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// UObject
	virtual void PostInitProperties() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	// UDeveloperSettings
	virtual FName GetCategoryName() const override { return FName("Plugins"); }
	virtual FName GetSectionName() const override { return FName("ImGui"); }
	virtual FText GetSectionText() const override { return NSLOCTEXT("ImGuiEditorSettings", "ImGuiEditorSettingsName", "ImGui"); }
	virtual FText GetSectionDescription() const override { return NSLOCTEXT("ImGuiEditorSettings", "ImGuiEditorSettingsSettingsDescription", "Configure the ImGui plugin"); }

	// Setup DPI Scale.
	UPROPERTY(EditAnywhere, config, Category = "DPI Scale", Meta = (ShowOnlyInnerProperties))
	FImGuiDPIScaleInfo DPIScale;
};
