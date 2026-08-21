#pragma once

#include "Client_Defines.h"
#include "GameObject.h"
#include "BoostTrailInstancedUI.h"

NS_BEGIN(Client)

class CSpellMiniGame final : public E::CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CSpellMiniGame, E::CGameObject)

	enum class MODE : uint32_t
	{
		INCENDIO,
		FLIPENDO
	};

	struct DESC : public E::CGameObject::GAMEOBJECT_DESC
	{
		MODE Mode{ MODE::INCENDIO };
	};

	enum class STATE
	{
		INTRO,
		WAITING,
		RUNNING,
		COMPLETED
	};

	enum class COMPLETION_PHASE
	{
		NONE,
		SUCCESS_ANIMATION,
		CENTER_DELAY,
		CENTER_MOVE,
		CENTER_HOLD,
		EXITING
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

	struct BOOST_TRAIL_PARTICLE
	{
		_float RemainingTime{};
		_float Duration{};
		_float2 SpawnPosition{};
		_float2 CenterPosition{};
		_float2 LateralDirection{};
		_float2 RenderPosition{};
		_float2 RenderSize{};
		_float Rotation{};
		_float WavePhase{};
		_float InitialScale{ 1.f };
		uint32_t Frame{};
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
	void BuildFlipendoPath();
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
	_bool CreateBoostTrailPool(const std::string& currentLevel);
	void UpdateBoostTrailEmitter(
		_float fTimeDelta,
		const _float2& position,
		const _float2& facingDirection);
	void UpdateBoostTrailParticles(_float fTimeDelta);
	void EmitBoostTrailParticle(
		const _float2& position,
		const _float2& facingDirection,
		_float rotationOffset,
		_float initialScale,
		_float distanceScale);
	void ClearBoostTrailParticles();
	void UpdateChaserTrailEmitter(
		_float fTimeDelta,
		const _float2& position,
		const _float2& facingDirection);
	void UpdateChaserTrailParticles(_float fTimeDelta);
	void EmitChaserTrailParticle(
		const _float2& position,
		const _float2& facingDirection,
		_float rotationOffset);
	void ClearChaserTrailParticles();
	void UpdateIntro(_float fTimeDelta);
	void SetIntroAlpha(_float alpha);
	void UpdateIntroPadScales(_float elapsedTime);
	void PlayCreateButtonSound();
	void TryActivateBoostPad(_bool aPressed, _bool xPressed);
	void ActivateBoost();
	void PlayBoostCursorRipple();
	void UpdateBoostCursorRipple(_float fTimeDelta);
	void ResetBoostCursorRipple();
	void UpdateChaser(_float movementDelta, _float timerDelta);
	void ResetChaser();
	void SetChaserVisible(_bool visible);
	void ResetBoostPads();
	void SetStartPadVisible(_bool visible);
	void SetCursorVisible(_bool visible);
	_bool CreateDestinationSuccessFlame(
		const _float2& destinationPosition,
		const std::string& currentLevel);
	void PlayDestinationSuccessFlame();
	void ResetDestinationSuccessFlame();
	void PlayDestinationSuccessMeterScale();
	void ResetDestinationSuccessMeterScale();
	void PlayDestinationSuccessDiamondPulse();
	void ResetDestinationSuccessDiamondPulse();
	void UpdateCompletionSequence(_float fTimeDelta);
	void PlayCompletionCenterTransition();
	void FadeOutCompletionSecondaryVisuals();
	void PlayCompletionExitTransition();
	void ShowSuccessAlarm();
	void FadeOutUIHierarchy(CHandle rootHandle, _float duration);
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
	MODE m_eMode{ MODE::INCENDIO };
	STATE m_eState{ STATE::INTRO };

	_float2 m_vPathTopLeft{};
	_float m_fPathSize{};
	_float m_fPathDistance{};
	_float m_fTotalPathDistance{};
	_float m_fBoostTimeRemaining{};
	_float m_fChaserPathDistance{};
	_float m_fChaserStartDelayRemaining{};
	_float m_fIntroElapsed{};
	_float m_fCompletionPhaseElapsed{};
	_float2 m_vLastFacingDirection{ 0.f, -1.f };
	_float2 m_vChaserFacingDirection{ 0.f, -1.f };
	_bool m_bChaserActive{};
	_bool m_bStartPadIntroRevealed{};
	COMPLETION_PHASE m_eCompletionPhase{ COMPLETION_PHASE::NONE };
	std::vector<BOOST_PAD> m_BoostPads{};
	std::vector<TRANSIENT_EFFECT> m_TransientEffects{};
	std::vector<BOOST_TRAIL_PARTICLE> m_BoostTrailParticles{};
	E::UPtr<CBoostTrailInstancedUI> m_pBoostTrailRenderer{};
	std::vector<BOOST_TRAIL_PARTICLE> m_ChaserTrailParticles{};
	E::UPtr<CBoostTrailInstancedUI> m_pChaserTrailRenderer{};
	_float m_fBoostTrailSpawnAccumulator{};
	_float m_fBoostTrailWavePhase{};
	_float m_fChaserTrailSpawnAccumulator{};
	_float m_fBoostCursorRippleElapsed{};
	size_t m_iNextBoostTrailParticle{};
	size_t m_iNextChaserTrailParticle{};
	_bool m_bNextBoostTrailLeft{ true };
	_bool m_bBoostCursorRippleActive{};

	CHandle m_hPath{};
	CHandle m_hIntroPathProgress{};
	CHandle m_hPathProgress{};
	CHandle m_hChaserPathProgress{};
	CHandle m_hChaserCursor{};
	CHandle m_hDestinationSpellMeter{};
	CHandle m_hDestinationSpellMeterBorder{};
	CHandle m_hDestinationSuccessDiamond{};
	CHandle m_hDestinationSuccessFlame{};
	CHandle m_hArrow{};
	CHandle m_hCursor{};
	CHandle m_hBoostCursorRipple{};
	CHandle m_hStartPadBackdrop{};
	CHandle m_hStartPad{};
	SUCCESS_EFFECT m_StartPadSuccessEffect{};

	static constexpr _float MAX_MOVE_SPEED = 130.f;
	static constexpr _float MAX_MOVEMENT_DELTA = 1.f / 60.f;
	static constexpr _float INTRO_FADE_DURATION = 2.f;
	static constexpr _float INTRO_SPELL_METER_DURATION = 0.3f;
	static constexpr _float INTRO_PATH_DURATION = 3.f;
	static constexpr _float INTRO_PAD_SCALE_DURATION = 0.3f;
	static constexpr _float INTRO_START_PAD_FADE_DURATION = 0.3f;
	static constexpr _float INTRO_TOTAL_DURATION =
		INTRO_PATH_DURATION + INTRO_START_PAD_FADE_DURATION;
	static constexpr _float INCENDIO_PATH_SCREEN_RATIO = 0.82f;
	static constexpr _float FLIPENDO_PATH_SCREEN_RATIO = 0.96f;
	static constexpr _float ALIGNMENT_DEAD_ZONE = 0.12f;
	static constexpr _float MOUSE_DIRECTION_MIN_DISTANCE = 8.f;
	static constexpr _float CORNER_STEERING_SAMPLE_DISTANCE = 24.f;
	static constexpr _float CURSOR_SIZE = 24.f;
	static constexpr _float CURSOR_ARROW_SIZE = 72.f;
	static constexpr _float CURSOR_ARROW_ORBIT_RADIUS = 14.f;
	static constexpr _float BOOST_CURSOR_RIPPLE_MIN_SIZE = CURSOR_SIZE;
	static constexpr _float BOOST_CURSOR_RIPPLE_MAX_SIZE = 60.f;
	static constexpr _float BOOST_CURSOR_RIPPLE_HALF_DURATION = 0.22f;
	static constexpr uint32_t BOOST_CURSOR_RIPPLE_REPEAT_COUNT = 2;
	static constexpr _float BOOST_CURSOR_RIPPLE_BRIGHTNESS = 1.25f;
	static constexpr _float BOOST_CURSOR_RIPPLE_TOTAL_DURATION =
		BOOST_CURSOR_RIPPLE_HALF_DURATION * 2.f *
		static_cast<_float>(BOOST_CURSOR_RIPPLE_REPEAT_COUNT);
	static constexpr size_t BOOST_TRAIL_POOL_SIZE = 500;
	static constexpr size_t BOOST_TRAIL_BURST_COUNT = 3;
	static constexpr size_t BOOST_TRAIL_SINE_ROTATION_PERIOD = 3;
	static constexpr _bool BOOST_TRAIL_ENABLED = true;
	static constexpr int BOOST_TRAIL_WEIGHT = 912;
	static constexpr _float BOOST_TRAIL_WIDTH = 56.f;
	static constexpr _float BOOST_TRAIL_HEIGHT = 72.f;
	static constexpr _float BOOST_TRAIL_ALPHA = 0.2f;
	static constexpr _float BOOST_TRAIL_EMISSION_END_SCALE = 0.1f;
	// The visible smoke in each atlas cell is biased toward local -X.
	static constexpr _float BOOST_TRAIL_VISUAL_CENTER_CORRECTION = 9.f;
	static constexpr _float BOOST_TRAIL_BACKWARD_DISTANCE_MIN = 64.f;
	static constexpr _float BOOST_TRAIL_BACKWARD_DISTANCE_MAX = 100.f;
	static constexpr _float BOOST_TRAIL_RANDOM_DISTANCE_SCALE = 0.8f;
	static constexpr _float BOOST_TRAIL_SINE_DISTANCE_SCALE = 1.25f;
	static constexpr _float BOOST_TRAIL_ROTATION_OFFSET_MIN = -15.f;
	static constexpr _float BOOST_TRAIL_ROTATION_OFFSET_MAX = 15.f;
	static constexpr _float BOOST_TRAIL_ROTATION_JITTER = 3.f;
	static constexpr _float BOOST_TRAIL_ROTATION_WAVE_SPEED = 20.f;
	static constexpr _float BOOST_TRAIL_WAVE_AMPLITUDE = 2.5f;
	static constexpr _float BOOST_TRAIL_WAVE_TIME_SPEED = 1.2f;
	static constexpr _float BOOST_TRAIL_SPAWN_INTERVAL = 0.01f;
	static constexpr _float BOOST_TRAIL_PARTICLE_DURATION = 1.5f;
	static constexpr _float BOOST_TRAIL_FLIPBOOK_DURATION = 1.5f;
	static constexpr size_t CHASER_TRAIL_POOL_SIZE = 256;
	static constexpr size_t CHASER_TRAIL_BURST_COUNT = 2;
	static constexpr int CHASER_TRAIL_WEIGHT = 908;
	static constexpr _float CHASER_TRAIL_WIDTH = 52.f;
	static constexpr _float CHASER_TRAIL_HEIGHT = 52.f;
	static constexpr uint32_t CHASER_TRAIL_ATLAS_SIZE = 1024;
	static constexpr _float CHASER_TRAIL_ALPHA = 0.15f;
	static constexpr _float CHASER_TRAIL_MAX_BRIGHTNESS = 1.7f;
	static constexpr _float CHASER_TRAIL_SPAWN_INTERVAL = 0.02f;
	static constexpr _float CHASER_TRAIL_PARTICLE_DURATION = 1.2f;
	static constexpr _float CHASER_TRAIL_FLIPBOOK_DURATION = 1.2f;
	static constexpr _float CHASER_TRAIL_DISTANCE_MIN = 64.f;
	static constexpr _float CHASER_TRAIL_DISTANCE_MAX = 100.f;
	static constexpr _float CHASER_TRAIL_ROTATION_OFFSET_MIN = -10.f;
	static constexpr _float CHASER_TRAIL_ROTATION_OFFSET_MAX = 10.f;
	static constexpr _float CHASER_TURN_RESPONSE = 9.f;
	static constexpr _float DESTINATION_SPELL_METER_SIZE = 120.f;
	static constexpr _float DESTINATION_SPELL_METER_BORDER_SIZE = 132.f;
	static constexpr _float DESTINATION_SUCCESS_FLAME_WIDTH = 198.f;
	static constexpr _float DESTINATION_SUCCESS_FLAME_HEIGHT = 308.f;
	static constexpr _float DESTINATION_SUCCESS_FLAME_OFFSET_Y = -92.f;
	static constexpr _float DESTINATION_SUCCESS_FLAME_FADE_TIME = 0.28f;
	static constexpr _float DESTINATION_SUCCESS_ANIMATION_DURATION = 0.4f;
	static constexpr _float DESTINATION_SUCCESS_METER_SCALE = 1.2f;
	static constexpr _float DESTINATION_SUCCESS_FLAME_SCALE = 1.2f;
	static constexpr _float DESTINATION_SUCCESS_CENTER_DELAY = 0.5f;
	static constexpr _float DESTINATION_SUCCESS_CENTER_MOVE_DURATION = 1.f;
	static constexpr _float DESTINATION_SUCCESS_CENTER_HOLD_DURATION = 2.f;
	static constexpr _float DESTINATION_SUCCESS_EXIT_DURATION = 0.3f;
	static constexpr _float SUCCESS_ALARM_FADE_IN_DURATION = 0.3f;
	static constexpr _float SUCCESS_ALARM_DELAY = 0.f;
	static constexpr _float SUCCESS_ALARM_HOLD_DURATION = 3.f;
	static constexpr _float SUCCESS_ALARM_FADE_OUT_DURATION = 0.5f;
	static constexpr _float SUCCESS_ALARM_SMOKE_ALPHA = 0.7f;
	static constexpr _float SUCCESS_ALARM_SMOKE_DURATION = 1.5f;
	static constexpr _float SUCCESS_ALARM_SMOKE_FADE_IN_DURATION = 0.2f;
	static constexpr _float SUCCESS_ALARM_SMOKE_FADE_DURATION = 0.4f;
	static constexpr int SUCCESS_ALARM_SMOKE_WEIGHT = 898;
	static constexpr _float SUCCESS_ALARM_EFFECT_HEIGHT_SCALE = 1.35f;
	static constexpr _float SUCCESS_ALARM_EFFECT_CENTER_OFFSET_Y = 18.f;
	static constexpr _float SUCCESS_ALARM_SPLATTER_ALPHA = 0.72f;
	static constexpr _float SUCCESS_ALARM_SPLATTER_DURATION = 1.f;
	static constexpr _float SUCCESS_ALARM_SPLATTER_START_SCALE = 0.18f;
	static constexpr _float SUCCESS_ALARM_SPLATTER_END_SCALE = 1.25f;
	static constexpr _float SUCCESS_ALARM_SPLATTER_ROTATION = -30.f;
	static constexpr int SUCCESS_ALARM_SPLATTER_WEIGHT = 899;
	// Rotating the square texture by 45 degrees makes its bounding diamond
	// match the 120-pixel spell meter at this corrected starting size.
	static constexpr _float DESTINATION_SUCCESS_DIAMOND_START_SIZE =
		DESTINATION_SPELL_METER_SIZE * 0.70710678f;
	static constexpr _float DESTINATION_SUCCESS_DIAMOND_END_SIZE =
		DESTINATION_SUCCESS_DIAMOND_START_SIZE * 2.f;
	static constexpr _float DESTINATION_SUCCESS_DIAMOND_DURATION =
		DESTINATION_SUCCESS_ANIMATION_DURATION;
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
	static constexpr _float BOOST_DURATION = 1.5f;
	static constexpr _float BOOST_MAX_MULTIPLIER = 2.3f;
	static constexpr _float BOOST_SUCCESS_SMOKE_DURATION = 1.5f;
	static constexpr _float INTRO_SMOKE_DURATION =
		BOOST_SUCCESS_SMOKE_DURATION * (2.f / 3.f);
	static constexpr _float CHASER_MOVE_SPEED = 170.f;
	static constexpr _float CHASER_START_DELAY = 1.f;
	static constexpr _float CHASER_COLLISION_DISTANCE = 28.f;
	static constexpr _float CHASER_CURSOR_SIZE = 48.f;
	static constexpr _float CHASER_CURSOR_BRIGHTNESS = 1.7f;
};

NS_END
