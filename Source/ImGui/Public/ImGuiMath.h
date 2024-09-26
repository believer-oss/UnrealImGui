// Copyright The Believer Company. All Rights Reserved.

#include "imgui.h"

// Used to implement FMath::Lerp for imgui vectors. See the comment above TCustomLerp in UnrealMathUtility.h
template<> struct TCustomLerp<ImVec2>
{
	enum { Value = true };

	static inline ImVec2 Lerp(const ImVec2& A, const ImVec2& B, const float& Alpha)
	{
		return ImVec2(A.x + (B.x - A.x) * Alpha, A.y + (B.y - A.y) * Alpha);
	}
};

template<> struct TCustomLerp<ImVec4>
{
	enum { Value = true };

	static inline ImVec4 Lerp(const ImVec4& A, const ImVec4& B, const float& Alpha)
	{
		return ImVec4(A.x + (B.x - A.x) * Alpha, A.y + (B.y - A.y) * Alpha, A.z + (B.z - A.z) * Alpha, A.w + (B.w - A.w) * Alpha);
	}
};
