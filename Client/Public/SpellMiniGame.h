#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Client)

class CSpellMiniGame final : public E::CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CSpellMiniGame, E::CGameObject)

	enum class STATE
	{
		INTRO,
		WAITING,
		RUNNING,
		COMPLETED
	};

private:
	struct SUCCESS_EFFECT
	{
		CHandle WispyHandle{};
		CHandle FireHandle{};
		CHandle CoreHandle{};
		CHandle SmokeHandle{};
	};

	struct PATH_SAMPLE
	{
		_float2 Position{};
		_float AccumulatedDistance{};
	};

	struct BOOST_PAD
	{
		_float PathDistance{};
		_ubyte KeyCode{};
		CHandle BackdropHandle{};
		CHandle Handle{};
		SUCCESS_EFFECT SuccessEffect{};
		_bool Consumed{};
		_bool IntroRevealed{};
	};

	struct TRANSIENT_EFFECT
	{
		CHandle Handle{};
		_float RemainingTime{};
		_float Duration{};
	};

private:
	CSpellMiniGame();
	CSpellMiniGame(const CSpellMiniGame& rhs);
	~CSpellMiniGame() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(_float fTimeDelta) override;
	void Update(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(
		ID3D11DeviceContext* pContext,
		const E::RENDER_CTX& ctx) override;

	STATE GetState() const { return m_eState; }
	_float GetProgress() const;
	void ResetToStart();

public:
	static E::UPtr<CSpellMiniGame> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
	_bool CreateVisuals();
	void DestroyVisuals();
	void BuildIncendioPath();
	void AppendLine(
		const _float2& start,
		const _float2& end,
		uint32_t sampleCount);
	void AppendQuadraticBezier(
		const _float2& start,
		const _float2& control,
		const _float2& end,
		uint32_t sampleCount);
	_float2 EvaluatePosition(_float distance) const;
	_float2 EvaluateForward(_float distance) const;
	void UpdateArrowVisual(
		const _float2& position,
		const _float2& facingDirection);
	void UpdateIntro(_float fTimeDelta);
	void SetIntroAlpha(_float alpha);
	void UpdateIntroPadScales(_float elapsedTime);
	void TryActivateBoostPad(_bool aPressed, _bool xPressed);
	void ActivateBoost();
	void UpdateChaser(_float movementDelta, _float timerDelta);
	void ResetChaser();
	void SetChaserVisible(_bool visible);
	void ResetBoostPads();
	void SetStartPadVisible(_bool visible);
	void SetCursorVisible(_bool visible);
	SUCCESS_EFFECT CreateMagicBurst(const _float2& position);
	void PlayMagicBurst(
		const SUCCESS_EFFECT& effect,
		CHandle padHandle);
	void SetBoostPadHighlight(CHandle padHandle, _bool highlighted);
	void CreateBoostSuccessSmoke(
		CHandle smokeHandle,
		CHandle padHandle,
		int weight = 916,
		_float sizeScale = 1.f,
		const _float3& color = {},
		_float duration = BOOST_SUCCESS_SMOKE_DURATION);
	void UpdateTransientEffects(_float fTimeDelta);
	void ClearTransientEffects();
	void SetMagicBurstVisible(
		const SUCCESS_EFFECT& effect,
		_bool visible);
	void DestroyMagicBurst(SUCCESS_EFFECT& effect);
	void SetUIHierarchyVisible(CHandle rootHandle, _bool visible);
	void SetUIHierarchyInputLocked(CHandle rootHandle);
	void DestroyUIHandle(CHandle& handle);

private:
	std::vector<PATH_SAMPLE> m_PathSamples{};
	STATE m_eState{ STATE::INTRO };

	_float2 m_vPathTopLeft{};
	_float m_fPathSize{};
	_float m_fPathDistance{};
	_float m_fTotalPathDistance{};
	_float m_fBoostTimeRemaining{};
	_float m_fChaserPathDistance{};
	_float m_fChaserStartDelayRemaining{};
	_float m_fIntroElapsed{};
	_float2 m_vLastFacingDirection{ 0.f, -1.f };
	_bool m_bChaserActive{};
	_bool m_bStartPadIntroRevealed{};
	std::vector<BOOST_PAD> m_BoostPads{};
	std::vector<TRANSIENT_EFFECT> m_TransientEffects{};

	CHandle m_hPath{};
	CHandle m_hIntroPathProgress{};
	CHandle m_hPathProgress{};
	CHandle m_hChaserPathProgress{};
	CHandle m_hChaserCursor{};
	CHandle m_hDestinationSpellMeter{};
	CHandle m_hDestinationSpellMeterBorder{};
	CHandle m_hArrow{};
	CHandle m_hCursor{};
	CHandle m_hStartPadBackdrop{};
	CHandle m_hStartPad{};
	SUCCESS_EFFECT m_StartPadSuccessEffect{};

	static constexpr _float MAX_MOVE_SPEED = 120.f;
	static constexpr _float MAX_MOVEMENT_DELTA = 1.f / 60.f;
	static constexpr _float INTRO_FADE_DURATION = 2.f;
	static constexpr _float INTRO_SPELL_METER_DURATION = 0.3f;
	static constexpr _float INTRO_PATH_DURATION = 3.f;
	static constexpr _float INTRO_PAD_SCALE_DURATION = 0.3f;
	static constexpr _float INTRO_START_PAD_FADE_DURATION = 0.3f;
	static constexpr _float INTRO_TOTAL_DURATION =
		INTRO_PATH_DURATION + INTRO_START_PAD_FADE_DURATION;
	static constexpr _float ALIGNMENT_DEAD_ZONE = 0.12f;
	static constexpr _float MOUSE_DIRECTION_MIN_DISTANCE = 8.f;
	static constexpr _float CORNER_STEERING_SAMPLE_DISTANCE = 24.f;
	static constexpr _float CURSOR_SIZE = 24.f;
	static constexpr _float CURSOR_ARROW_SIZE = 72.f;
	static constexpr _float CURSOR_ARROW_ORBIT_RADIUS = 14.f;
	static constexpr _float DESTINATION_SPELL_METER_SIZE = 120.f;
	static constexpr _float DESTINATION_SPELL_METER_BORDER_SIZE = 136.f;
	// Leave a subtle gap inside the callout ring's black center.
	static constexpr _float BOOST_PAD_ICON_SIZE = 28.f;
	static constexpr _float BOOST_PAD_BACKDROP_SIZE = 50.f;
	static constexpr _float BOOST_SUCCESS_SMOKE_SIZE = 70.f;
	static constexpr _float INTRO_SMOKE_SIZE_SCALE = 1.2f;
	static constexpr _float MAGIC_BURST_CORE_SIZE = 50.f;
	static constexpr _float MAGIC_BURST_CORE_PULSE_SIZE = 70.f;
	static constexpr _float MAGIC_BURST_CORE_GROW_TIME = 0.35f;
	static constexpr _float MAGIC_BURST_CORE_RETURN_TIME = 0.65f;
	static constexpr _float BOOST_PAD_TRIGGER_RANGE = 36.f;
	static constexpr _float BOOST_DURATION = 0.85f;
	static constexpr _float BOOST_MAX_MULTIPLIER = 2.5f;
	static constexpr _float BOOST_SUCCESS_SMOKE_DURATION = 1.5f;
	static constexpr _float INTRO_SMOKE_DURATION =
		BOOST_SUCCESS_SMOKE_DURATION * (2.f / 3.f);
	static constexpr _float CHASER_MOVE_SPEED = 150.f;
	static constexpr _float CHASER_START_DELAY = 1.f;
	static constexpr _float CHASER_COLLISION_DISTANCE = 28.f;
	static constexpr _float CHASER_CURSOR_SIZE = 36.f;
};

NS_END
