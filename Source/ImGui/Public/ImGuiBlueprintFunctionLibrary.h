// Copyright The Believer Company. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ImGuiBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class IMGUI_API UImGuiBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "ImGui")
	static void SetNetImGuiPort(const FString& NetImGuiPort);
	
};
