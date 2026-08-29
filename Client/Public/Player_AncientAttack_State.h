
#pragma once

#include "Player_SkillStateBase.h"

NS_BEGIN(Client)

class CPlayer_AncientAttack_State final : public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_AncientAttack_State, CPlayer_SkillStateBase)

private:
	CPlayer_AncientAttack_State() = default;
	~CPlayer_AncientAttack_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Exit(CStateMachine* pStateMachine) override;
	static SPtr<CPlayer_AncientAttack_State> Create();

private:
	void CacheAnimationIndices(const CPlayer& player);
	_bool UpdateThrowPull(CPlayer& player, _float fPullRatio);
	_bool LaunchThrow(CPlayer& player);
	void EmitThrowWandTrail(CPlayer& player) const;
	void BeginThrowSlowMotion();
	void EndThrowSlowMotion(_bool bImmediate = false);

private:
	enum class PHASE
	{
		CAST,
		ATTACK,
		RECOVERY
	};

	static constexpr _float ANCIENT_LIGHTNING_CAST_START_RATIO = 0.55f;
	static constexpr _float ANCIENT_LIGHTNING_ATTACK_DURATION = 1.1f;
	static constexpr _float ANCIENT_LIGHTNING_ATTACK_STOP_DURATION = 1.5f;
	static constexpr _float ANCIENT_LIGHTNING_LAST_ATTACK = 0.2f;
	static constexpr _float RECOVERY_EXIT_RATIO = 1.f;
	static constexpr _float RECOVERY_INPUT_EXIT_RATIO = 0.35f;
	static constexpr _float ANCIENT_THROW_FACING_END_RATIO = 0.2f;
	static constexpr _float ANCIENT_THROW_PULL_END_ANIM_RATIO = 0.1f;
	static constexpr _float ANCIENT_THROW_STATE_RELEASE_RATIO = 0.65f;
	static constexpr _float ANCIENT_THROW_INPUT_RELEASE_RATIO = 0.35f;
	static constexpr _float ANCIENT_THROW_SLOW_HOLD_DURATION = 0.5f;
	static constexpr _float ANCIENT_THROW_BLUR_INTENSITY = 3.f;
	static constexpr _float ANCIENT_THROW_RELEASE_BLUR_INTENSITY = 4.5f;
	static constexpr _float ANCIENT_THROW_WAIT_ANIM_SPEED = 0.1f;
	static constexpr _float ANCIENT_THROW_TURN_SPEED = 540.f;
	static constexpr _float ANCIENT_THROW_HOLD_SIDE_OFFSET = 3.f;
	static constexpr _float ANCIENT_THROW_HOLD_HEIGHT = 4.2f;
	static constexpr _float ANCIENT_THROW_HOLD_FORWARD_OFFSET = 1.2f;
	static constexpr _float ANCIENT_THROW_PULL_ARC_HEIGHT = 1.5f;
	static constexpr _float ANCIENT_THROW_PULL_SPIN_TURNS = 0.75f;
	static constexpr _float ANCIENT_THROW_HOLD_SPIN_TURNS_PER_SECOND = 0.5f;
	static constexpr _float ANCIENT_THROW_DIRECT_SPEED = 220.f;
	static constexpr _float ANCIENT_THROW_SPIN_SPEED = 14.f;
	static constexpr _float ANCIENT_THROW_SLOW_SCALE = 0.55f;
	static constexpr _float ANCIENT_THROW_SLOW_POST_LAUNCH_DURATION = 0.08f;
	static constexpr _float ANCIENT_THROW_SLOW_BLEND_IN = 0.015f;
	static constexpr _float ANCIENT_THROW_SLOW_BLEND_OUT = 0.04f;
	static constexpr _float ANCIENT_THROW_SLOW_MAX_UNSCALED_DURATION = 1.25f;
	static constexpr _float ANCIENT_THROW_FOV_Y = 58.f;
	static constexpr _float ANCIENT_THROW_FOV_BLEND_IN_RESPONSE = 6.f;
	static constexpr _float ANCIENT_THROW_FOV_BLEND_OUT_RESPONSE = 2.5f;

	int32_t m_iLightningCastAnimation{ -1 };
	int32_t m_iLightningEndAnimation{ -1 };
	int32_t m_iAncientThrowLeftAnimation{ -1 };
	int32_t m_iAncientThrowRightAnimation{ -1 };
	PHASE m_ePhase{ PHASE::CAST };
	_bool m_bAnimationIndicesCached{};
	_bool m_bOnceLightning{};
	_bool m_bOnceLastLightning{};
	_float m_fAnimationRatio{};
	_float m_fAncientElapsed{};
	_float m_fSpawnDelay{};
	std::optional<CHandle> m_hThrowBarrel{};
	std::optional<CHandle> m_hThrowDestination{};
	std::optional<CHandle> m_hThrowFovCamera{};
	_bool m_bThrowReleased{};
	_bool m_bThrowSlowMotionActive{};
	_bool m_bThrowBlurActive{};
	_float m_fPreviousThrowBlurIntensity{};
	_float m_fThrowSlowHoldUnscaledElapsed{};
	_float m_fThrowPostLaunchUnscaledElapsed{};
	_float3 m_vThrowPullStartCenter{};
	_float4 m_vThrowPullStartRotation{ 0.f, 0.f, 0.f, 1.f };
};

NS_END
