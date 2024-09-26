// Copyright The Believer Company. All Rights Reserved.


#include "ImGuiBlueprintFunctionLibrary.h"

#include "ImGuiModule.h"

void UImGuiBlueprintFunctionLibrary::SetNetImGuiPort(const FString& NetImGuiPort)
{
	FImGuiNetControl& ImGuiNetControl = FImGuiModule::Get().GetNetControl();
	ImGuiNetControl.Port = FCString::Atoi(*NetImGuiPort);
}