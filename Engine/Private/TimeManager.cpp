#include "pch.h"
#include "TimeManager.h"

#include <cmath>

NS_USING(Engine)

namespace
{
	constexpr _float MIN_TIME_SCALE = 0.f;
	constexpr _float MAX_TIME_SCALE = 1.f;
}

void CTimeManager::BeginFrame(_float fUnscaledDelta)
{
	m_fUnscaledDelta = std::isfinite(fUnscaledDelta)
		? std::max(fUnscaledDelta, 0.f)
		: 0.f;

	for (auto& [_, Request] : m_Requests)
		UpdateRequest(Request, m_fUnscaledDelta);

	std::erase_if(
		m_Requests,
		[](const auto& Pair)
		{
			const TIME_SCALE_REQUEST& Request = Pair.second;
			return Request.ePhase == REQUEST_PHASE::BLEND_OUT &&
				Request.fCurrentScale >= 1.f;
		});

	RecalculateCurrentScale();
	m_fScaledDelta = m_fUnscaledDelta * m_fCurrentScale;
}

_bool CTimeManager::BeginTimeScale(
	const TIME_SCALE_REQUEST_DESC& Desc)
{
	if (Desc.sTag.hash == 0 ||
		!std::isfinite(Desc.fTargetScale) ||
		!std::isfinite(Desc.fBlendIn) ||
		!std::isfinite(Desc.fMaxUnscaledDuration) ||
		!std::isfinite(Desc.fSafetyBlendOut))
	{
		return false;
	}

	TIME_SCALE_REQUEST Request{};
	Request.Desc = Desc;
	Request.Desc.fTargetScale = std::clamp(
		Desc.fTargetScale,
		MIN_TIME_SCALE,
		MAX_TIME_SCALE);
	Request.Desc.fBlendIn = std::max(Desc.fBlendIn, 0.f);
	Request.Desc.fMaxUnscaledDuration =
		std::max(Desc.fMaxUnscaledDuration, 0.f);
	Request.Desc.fSafetyBlendOut =
		std::max(Desc.fSafetyBlendOut, 0.f);

	if (Request.Desc.fBlendIn <= 0.f)
	{
		Request.ePhase = REQUEST_PHASE::HOLD;
		Request.fCurrentScale = Request.Desc.fTargetScale;
	}

	m_Requests.insert_or_assign(Desc.sTag, std::move(Request));
	RecalculateCurrentScale();
	m_fScaledDelta = m_fUnscaledDelta * m_fCurrentScale;
	return true;
}

_bool CTimeManager::EndTimeScale(
	const StringID& sTag,
	_float fBlendOut)
{
	const auto Iter = m_Requests.find(sTag);
	if (Iter == m_Requests.end() || !std::isfinite(fBlendOut))
		return false;

	TIME_SCALE_REQUEST& Request = Iter->second;
	Request.ePhase = REQUEST_PHASE::BLEND_OUT;
	Request.fPhaseStartScale = Request.fCurrentScale;
	Request.fPhaseElapsed = 0.f;
	Request.fBlendOut = std::max(fBlendOut, 0.f);

	if (Request.fBlendOut <= 0.f)
	{
		m_Requests.erase(Iter);
		RecalculateCurrentScale();
		m_fScaledDelta = m_fUnscaledDelta * m_fCurrentScale;
	}

	return true;
}

_bool CTimeManager::CancelTimeScale(const StringID& sTag)
{
	if (m_Requests.erase(sTag) == 0)
		return false;

	RecalculateCurrentScale();
	m_fScaledDelta = m_fUnscaledDelta * m_fCurrentScale;
	return true;
}

void CTimeManager::ClearTimeScaleRequests()
{
	m_Requests.clear();
	m_fCurrentScale = 1.f;
	m_fScaledDelta = m_fUnscaledDelta;
}

_bool CTimeManager::IsTimeScaleActive(const StringID& sTag) const
{
	return sTag.hash != 0 && m_Requests.contains(sTag);
}

_float CTimeManager::GetScaledFixedDelta(_float fUnscaledFixedDelta) const
{
	if (!std::isfinite(fUnscaledFixedDelta) || fUnscaledFixedDelta <= 0.f)
		return 0.f;

	return fUnscaledFixedDelta * m_fCurrentScale;
}

UPtr<CTimeManager> CTimeManager::Create()
{
	return ToUPtr(new CTimeManager{});
}

_float CTimeManager::ApplySmoothStep(_float fRatio)
{
	fRatio = std::clamp(fRatio, 0.f, 1.f);
	return fRatio * fRatio * (3.f - 2.f * fRatio);
}

void CTimeManager::UpdateRequest(
	TIME_SCALE_REQUEST& Request,
	_float fUnscaledDelta)
{
	Request.fTotalUnscaledElapsed += fUnscaledDelta;

	if (Request.ePhase != REQUEST_PHASE::BLEND_OUT &&
		Request.Desc.fMaxUnscaledDuration > 0.f &&
		Request.fTotalUnscaledElapsed >= Request.Desc.fMaxUnscaledDuration)
	{
		Request.ePhase = REQUEST_PHASE::BLEND_OUT;
		Request.fPhaseStartScale = Request.fCurrentScale;
		Request.fPhaseElapsed = 0.f;
		Request.fBlendOut = Request.Desc.fSafetyBlendOut;
	}

	switch (Request.ePhase)
	{
	case REQUEST_PHASE::BLEND_IN:
	{
		Request.fPhaseElapsed += fUnscaledDelta;
		const _float fRatio = Request.Desc.fBlendIn > 0.f
			? ApplySmoothStep(Request.fPhaseElapsed / Request.Desc.fBlendIn)
			: 1.f;

		Request.fCurrentScale = std::lerp(
			1.f,
			Request.Desc.fTargetScale,
			fRatio);

		if (fRatio >= 1.f)
		{
			Request.ePhase = REQUEST_PHASE::HOLD;
			Request.fCurrentScale = Request.Desc.fTargetScale;
			Request.fPhaseElapsed = 0.f;
		}
		break;
	}
	case REQUEST_PHASE::HOLD:
		Request.fCurrentScale = Request.Desc.fTargetScale;
		break;
	case REQUEST_PHASE::BLEND_OUT:
	{
		Request.fPhaseElapsed += fUnscaledDelta;
		const _float fRatio = Request.fBlendOut > 0.f
			? ApplySmoothStep(Request.fPhaseElapsed / Request.fBlendOut)
			: 1.f;

		Request.fCurrentScale = std::lerp(
			Request.fPhaseStartScale,
			1.f,
			fRatio);
		break;
	}
	}
}

void CTimeManager::RecalculateCurrentScale()
{
	m_fCurrentScale = 1.f;
	for (const auto& [_, Request] : m_Requests)
		m_fCurrentScale = std::min(
			m_fCurrentScale,
			Request.fCurrentScale);

	m_fCurrentScale = std::clamp(
		m_fCurrentScale,
		MIN_TIME_SCALE,
		MAX_TIME_SCALE);
}
