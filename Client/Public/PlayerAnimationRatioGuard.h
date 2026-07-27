#pragma once

#include "Client_Defines.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Client::PlayerAnimationRatioGuard
{
	inline _float Sanitize(_float fRatio)
	{
		return std::isfinite(fRatio)
			? std::clamp(fRatio, 0.f, 1.f)
			: 0.f;
	}

	inline _bool Crossed(
		_float fPreviousRatio,
		_float fCurrentRatio,
		_float fThreshold)
	{
		const _float fPrevious = Sanitize(fPreviousRatio);
		const _float fCurrent = Sanitize(fCurrentRatio);
		const _float fEventRatio = Sanitize(fThreshold);

		if (fCurrent < fPrevious)
			return fCurrent >= fEventRatio;

		return fPrevious < fEventRatio &&
			fCurrent >= fEventRatio;
	}

	inline _bool Intersects(
		_float fPreviousRatio,
		_float fCurrentRatio,
		_float fStartRatio,
		_float fEndRatio)
	{
		_float fPrevious = Sanitize(fPreviousRatio);
		const _float fCurrent = Sanitize(fCurrentRatio);
		const _float fStart = Sanitize(std::min(fStartRatio, fEndRatio));
		const _float fEnd = Sanitize(std::max(fStartRatio, fEndRatio));

		if (fCurrent < fPrevious)
			fPrevious = 0.f;

		return std::max(fPrevious, fStart) <=
			std::min(fCurrent, fEnd);
	}

	inline _float CalculateActiveDeltaTime(
		_float fPreviousRatio,
		_float fCurrentRatio,
		_float fStartRatio,
		_float fEndRatio,
		_float fDeltaTime)
	{
		if (!std::isfinite(fDeltaTime) || fDeltaTime <= 0.f)
			return 0.f;

		_float fPrevious = Sanitize(fPreviousRatio);
		const _float fCurrent = Sanitize(fCurrentRatio);
		const _float fStart = Sanitize(std::min(fStartRatio, fEndRatio));
		const _float fEnd = Sanitize(std::max(fStartRatio, fEndRatio));

		if (fCurrent < fPrevious)
			fPrevious = 0.f;

		const _float fRatioDelta = fCurrent - fPrevious;
		if (fRatioDelta <= std::numeric_limits<_float>::epsilon())
			return 0.f;

		const _float fOverlapStart = std::max(fPrevious, fStart);
		const _float fOverlapEnd = std::min(fCurrent, fEnd);
		if (fOverlapEnd <= fOverlapStart)
			return 0.f;

		return fDeltaTime *
			((fOverlapEnd - fOverlapStart) / fRatioDelta);
	}
}
