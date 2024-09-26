// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiModuleSettings.h"

#include "ImGuiModuleCommands.h"
#include "ImGuiModuleProperties.h"

#include <Engine/Engine.h>
#include <GameFramework/GameUserSettings.h>
#include <Misc/ConfigCacheIni.h>


//====================================================================================================
// FImGuiDPIScaleInfo
//====================================================================================================

FImGuiDPIScaleInfo::FImGuiDPIScaleInfo()
{
	if (FRichCurve* Curve = DPICurve.GetRichCurve())
	{
		Curve->AddKey(   0.0f, 1.f);

		Curve->AddKey(2159.5f, 1.f);
		Curve->AddKey(2160.0f, 2.f);

		Curve->AddKey(4319.5f, 2.f);
		Curve->AddKey(4320.0f, 4.f);
	}
}

float FImGuiDPIScaleInfo::CalculateResolutionBasedScale() const
{
	float ResolutionBasedScale = 1.f;
	if (bScaleWithCurve && GEngine && GEngine->GameUserSettings)
	{
		if (const FRichCurve* Curve = DPICurve.GetRichCurveConst())
		{
			ResolutionBasedScale *= Curve->Eval((float)GEngine->GameUserSettings->GetDesktopResolution().Y, 1.f);
		}
	}
	return ResolutionBasedScale;
}

//====================================================================================================
// UImGuiSettings
//====================================================================================================

UImGuiSettings* UImGuiSettings::DefaultInstance = nullptr;

FSimpleMulticastDelegate UImGuiSettings::OnSettingsLoaded;

void UImGuiSettings::PostInitProperties()
{
	Super::PostInitProperties();

	if (IsTemplate())
	{
		DefaultInstance = this;
		OnSettingsLoaded.Broadcast();
	}
}

void UImGuiSettings::BeginDestroy()
{
	Super::BeginDestroy();

	if (DefaultInstance == this)
	{
		DefaultInstance = nullptr;
	}
}

//====================================================================================================
// FImGuiModuleSettings
//====================================================================================================

FImGuiModuleSettings::FImGuiModuleSettings(FImGuiModuleProperties& InProperties, FImGuiModuleCommands& InCommands)
	: Properties(InProperties)
	, Commands(InCommands)
{
	// Delegate initializer to support settings loaded after this object creation (in stand-alone builds) and potential
	// reloading of settings.
	UImGuiSettings::OnSettingsLoaded.AddRaw(this, &FImGuiModuleSettings::UpdateSettings);

	// Call initializer to support settings already loaded (editor).
	UpdateSettings();
}

FImGuiModuleSettings::~FImGuiModuleSettings()
{
	UImGuiSettings::OnSettingsLoaded.RemoveAll(this);
}

void FImGuiModuleSettings::UpdateSettings()
{
	if (UImGuiSettings* SettingsObject = UImGuiSettings::Get())
	{
		SetImGuiInputHandlerClass(SettingsObject->ImGuiInputHandlerClass);
		SetShareKeyboardInput(SettingsObject->bShareKeyboardInput);
		SetShareGamepadInput(SettingsObject->bShareGamepadInput);
		SetShareMouseInput(SettingsObject->bShareMouseInput);
		SetUseSoftwareCursor(SettingsObject->bUseSoftwareCursor);
		SetToggleInputKey(SettingsObject->ToggleInput);
		SetCanvasSizeInfo(SettingsObject->CanvasSize);
	}
}

void FImGuiModuleSettings::SetImGuiInputHandlerClass(const FSoftClassPath& ClassReference)
{
	if (ImGuiInputHandlerClass != ClassReference)
	{
		ImGuiInputHandlerClass = ClassReference;
		OnImGuiInputHandlerClassChanged.Broadcast(ClassReference);
	}
}

void FImGuiModuleSettings::SetShareKeyboardInput(bool bShare)
{
	if (bShareKeyboardInput != bShare)
	{
		bShareKeyboardInput = bShare;
		Properties.SetKeyboardInputShared(bShare);
	}
}

void FImGuiModuleSettings::SetShareGamepadInput(bool bShare)
{
	if (bShareGamepadInput != bShare)
	{
		bShareGamepadInput = bShare;
		Properties.SetGamepadInputShared(bShare);
	}
}

void FImGuiModuleSettings::SetShareMouseInput(bool bShare)
{
	if (bShareMouseInput != bShare)
	{
		bShareMouseInput = bShare;
		Properties.SetMouseInputShared(bShare);
	}
}

void FImGuiModuleSettings::SetUseSoftwareCursor(bool bUse)
{
	if (bUseSoftwareCursor != bUse)
	{
		bUseSoftwareCursor = bUse;
		OnUseSoftwareCursorChanged.Broadcast(bUse);
	}
}

void FImGuiModuleSettings::SetToggleInputKey(const FImGuiKeyInfo& KeyInfo)
{
	if (ToggleInputKey != KeyInfo)
	{
		ToggleInputKey = KeyInfo;
		Commands.SetKeyBinding(FImGuiModuleCommands::ToggleInput, ToggleInputKey);
	}
}

void FImGuiModuleSettings::SetCanvasSizeInfo(const FImGuiCanvasSizeInfo& CanvasSizeInfo)
{
	if (CanvasSize != CanvasSizeInfo)
	{
		CanvasSize = CanvasSizeInfo;
		OnCanvasSizeChangedDelegate.Broadcast(CanvasSize);
	}
}

void FImGuiModuleSettings::SetDPIScaleInfo(const FImGuiDPIScaleInfo& ScaleInfo)
{
	DPIScale = ScaleInfo;
	OnDPIScaleChangedDelegate.Broadcast(DPIScale);
}
