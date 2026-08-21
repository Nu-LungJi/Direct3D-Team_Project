#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)

enum class TIME_DOMAIN : uint8_t
{
	SCALED,
	UNSCALED
};

struct TIME_SCALE_REQUEST_DESC
{
	_float fTargetScale{ 1.f };
	_float fBlendIn{};
	_float fMaxUnscaledDuration{};
	_float fSafetyBlendOut{ 0.15f };
	StringID sTag{};
};

class ENGINE_DLL CTimeManager final : public CEngineBase
{
private:
	enum class REQUEST_PHASE : uint8_t
	{
		BLEND_IN,
		HOLD,
		BLEND_OUT
	};

	struct TIME_SCALE_REQUEST
	{
		TIME_SCALE_REQUEST_DESC Desc{};
		REQUEST_PHASE ePhase{ REQUEST_PHASE::BLEND_IN };
		_float fCurrentScale{ 1.f };
		_float fPhaseStartScale{ 1.f };
		_float fPhaseElapsed{};
		_float fTotalUnscaledElapsed{};
		_float fBlendOut{};
	};

private:
	CTimeManager() = default;
	~CTimeManager() override = default;

public:
	void BeginFrame(_float fUnscaledDelta);

	_bool BeginTimeScale(
		const TIME_SCALE_REQUEST_DESC& Desc);
	_bool EndTimeScale(
		const StringID& sTag,
		_float fBlendOut);
	_bool CancelTimeScale(const StringID& sTag);
	void ClearTimeScaleRequests();

	_bool IsTimeScaleActive(const StringID& sTag) const;

	_float GetUnscaledDelta() const { return m_fUnscaledDelta; }
	_float GetGameDelta() const { return m_fGameDelta; }
	_float GetTimeScale() const { return m_fCurrentScale; }
	_float ScaleFixedDelta(_float fUnscaledFixedDelta) const;

public:
	static UPtr<CTimeManager> Create();

private:
	static _float ApplySmoothStep(_float fRatio);
	void UpdateRequest(TIME_SCALE_REQUEST& Request, _float fUnscaledDelta);
	void RecalculateCurrentScale();

private:
	std::unordered_map<StringID, TIME_SCALE_REQUEST> m_Requests{};
	_float m_fUnscaledDelta{};
	_float m_fGameDelta{};
	_float m_fCurrentScale{ 1.f };
};

NS_END
