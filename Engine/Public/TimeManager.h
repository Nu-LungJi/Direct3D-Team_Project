#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)

enum class TIME_DOMAIN : uint8_t
{
	SCALED,
	UNSCALED
};

using TIME_SCALE_HANDLE = uint64_t;
inline constexpr TIME_SCALE_HANDLE INVALID_TIME_SCALE_HANDLE = 0;

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
		TIME_SCALE_HANDLE hHandle{ INVALID_TIME_SCALE_HANDLE };
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

	TIME_SCALE_HANDLE BeginTimeScale(
		const TIME_SCALE_REQUEST_DESC& Desc);
	_bool EndTimeScale(
		TIME_SCALE_HANDLE hHandle,
		_float fBlendOut);
	_bool CancelTimeScale(TIME_SCALE_HANDLE hHandle);
	void ClearTimeScaleRequests();

	_bool IsTimeScaleActive(TIME_SCALE_HANDLE hHandle) const;

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
	TIME_SCALE_HANDLE GenerateHandle();

private:
	std::unordered_map<TIME_SCALE_HANDLE, TIME_SCALE_REQUEST> m_Requests{};
	TIME_SCALE_HANDLE m_hNextHandle{ 1 };
	_float m_fUnscaledDelta{};
	_float m_fGameDelta{};
	_float m_fCurrentScale{ 1.f };
};

NS_END
