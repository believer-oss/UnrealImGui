#include "ImGuiEditorSettings.h"
#include "ImGuiModule.h"

void UImGuiEditorSettings::PostInitProperties()
{
	Super::PostInitProperties();

	// First instance was loaded, so we can update imgui module settings now
	if (IsTemplate())
	{
		IImGuiModuleSettings* Settings = FImGuiModule::Get().GetSettings();
		if (ensure(Settings))
		{
			Settings->SetDPIScaleInfo(DPIScale);
		}
	}
}

void UImGuiEditorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UImGuiEditorSettings, DPIScale))
	{
		IImGuiModuleSettings* Settings = FImGuiModule::Get().GetSettings();
		if (ensure(Settings))
		{
			Settings->SetDPIScaleInfo(DPIScale);
		}
	}
}
