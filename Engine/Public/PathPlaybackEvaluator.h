#pragma once

#include "PathPlaybackDefines.h"

NS_BEGIN(Engine)

class ENGINE_DLL CPathPlaybackEvaluator final
{
public:
	struct CONTEXT
	{
		PATH_PLAYBACK_POSE tStartAnchorPose{};
		PATH_PLAYBACK_POSE tCurrentObjectPose{};
		_bool bHasCurrentObjectPose{};
	};

public:
	static _bool EvaluatePose(
		const PATH_PLAYBACK_CLIP& Clip,
		_float fElapsedTime,
		const CONTEXT& Context,
		PATH_PLAYBACK_POSE& OutPose,
		size_t* pOutSegmentIndex = nullptr);

private:
	static _float ApplyEasing(
		_float fRatio,
		PATH_PLAYBACK_EASING eEasing);
	static _vector NormalizeQuaternionOrIdentity(
		const _float4& Rotation);
	static _float4 MakeFacingQuaternion(
		const _float3& Direction,
		const _float4& Fallback);
};

NS_END
