// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiModuleSettingsInterface.generated.h"


UENUM(BlueprintType)
enum class EImGuiDPIScaleMethod : uint8
{
	ImGui UMETA(DisplayName = "ImGui", ToolTip = "Scale ImGui fonts and styles."),
	Slate UMETA(ToolTip = "Scale in Slate. ImGui canvas size will be adjusted to get the screen size that is the same as defined in the Canvas Size property.")
};

/**
 * Struct with DPI scale data.
 */
USTRUCT()
struct IMGUI_API FImGuiDPIScaleInfo
{
	GENERATED_BODY()

protected:

	// Whether to scale in ImGui or in Slate. Scaling in ImGui gives better looking results but Slate might be a better
	// option when layouts do not account for different fonts and styles. When scaling in Slate, ImGui canvas size will
	// be adjusted to get the screen size that is the same as defined in the Canvas Size property.
	UPROPERTY(EditAnywhere, Category = "DPI Scale")
	EImGuiDPIScaleMethod ScalingMethod = EImGuiDPIScaleMethod::ImGui;

	// An optional scale to apply on top or instead of the curve-based scale.
	UPROPERTY(EditAnywhere, Category = "DPI Scale", meta = (ClampMin = 0, UIMin = 0))
	float Scale = 1.f;

	// Curve mapping resolution height to scale.
	UPROPERTY(config, EditAnywhere, Category = "DPI Scale", meta = (XAxisName = "Resolution Height", YAxisName = "Scale", EditCondition = "bScaleWithCurve"))
	FRuntimeFloatCurve DPICurve;

	// Whether to use curve-based scaling. If enabled, Scale will be multiplied by a value read from the DPICurve.
	// If disabled, only the Scale property will be used.
	UPROPERTY(config, EditAnywhere, Category = "DPI Scale")
	bool bScaleWithCurve = true;

public:

	FImGuiDPIScaleInfo();

	float GetImGuiScale() const { return ShouldScaleInSlate() ? 1.f : CalculateScale(); }

	float GetSlateScale() const { return ShouldScaleInSlate() ? CalculateScale() : 1.f; }

	bool ShouldScaleInSlate() const { return ScalingMethod == EImGuiDPIScaleMethod::Slate; }

private:

	float CalculateScale() const { return Scale * CalculateResolutionBasedScale(); }

	float CalculateResolutionBasedScale() const;
};

class IMGUI_API IImGuiModuleSettings
{
public:
	virtual ~IImGuiModuleSettings() = default;
	virtual void SetDPIScaleInfo(const FImGuiDPIScaleInfo& InDPIScale) = 0;
};
