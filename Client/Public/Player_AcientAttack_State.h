
#pragma once
#include "Player_SkillStateBase.h"


NS_BEGIN(Client)


class CPlayer_AcientAttack_State final : public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_AcientAttack_State, CPlayer_SkillStateBase)

public:
	enum class ACIENT_SKILL {
		ACIENT_LIGHTENING,
		ACIENT_THROW,
		END
	};
private:
	CPlayer_AcientAttack_State() = default;
	~CPlayer_AcientAttack_State() override = default;

public:
	void Enter(CStateMachine* stateMachine) override;
	void Update(CStateMachine* stateMachine, _float deltaTime) override;
	void Exit(CStateMachine* stateMachine) override;

	static SPtr<CPlayer_AcientAttack_State> Create();

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

	_bool			  m_bAnimationIndicesCached = false;

	std::array<int32_t, (size_t)ETOUI(ACIENT_SKILL::END)> m_AcientCast_Animations{};
	std::array<int32_t, (size_t)ETOUI(ACIENT_SKILL::END)> m_AcientEnd_Animations{};
	int32_t m_iAncientThrowLeftAnimation{ -1 };
	int32_t m_iAncientThrowRightAnimation{ -1 };

	PHASE m_ePhase = PHASE::CAST;
	static constexpr _float ACIENT_LIGHTENING_CAST_START_RATIO = 0.55f;
	
	static constexpr _float ACIENT_LIGHTENING_ATTACK_DURATION = 1.1f;
	static constexpr _float ACIENT_LIGHTENING_ATTACK_STOP_DURATION = 1.5f;
	static constexpr _float ACIENT_LIGHTENING_LAST_ATTACK= 0.2f;
	static constexpr _float RECOVERY_EXIT_RATIO = 1.f;
	_bool   m_bOnceLighting = false;
	_bool   m_bOnceLastLighting = false;
	_float	m_fAnimRatio = 0.f;
	_float	m_fAcientElapsed = 0.f;
	_float	m_fSpawnDelay = 0.f;
	std::optional<CHandle> m_hThrowBarrel{};
	std::optional<CHandle> m_hThrowDestination{};
	_bool m_bThrowReleased{};
	_bool m_bThrowSlowMotionActive{};
	std::optional<CHandle> m_hThrowFovCamera{};
	_float m_fThrowSequenceElapsed{};
	_float m_fThrowPostLaunchUnscaledElapsed{};
	_float3 m_vThrowPullStartCenter{};
	_float4 m_vThrowPullStartRotation{ 0.f, 0.f, 0.f, 1.f };
	static constexpr _float ACIENT_THROW_FACING_END_RATIO = 0.2f;
	static constexpr _float ACIENT_THROW_LAUNCH_RATIO = 0.32f;
	static constexpr _float ACIENT_THROW_STATE_RELEASE_RATIO = 0.85f;
	static constexpr _float ACIENT_THROW_PULL_DURATION = 0.72f;
	static constexpr _float ACIENT_THROW_WAIT_ANIM_SPEED = 0.75f;
	static constexpr _float ACIENT_THROW_TURN_SPEED = 540.f;
	static constexpr _float ACIENT_THROW_HOLD_SIDE_OFFSET = 2.4f;
	static constexpr _float ACIENT_THROW_HOLD_HEIGHT = 3.6f;
	static constexpr _float ACIENT_THROW_HOLD_FORWARD_OFFSET = 1.2f;
	static constexpr _float ACIENT_THROW_PULL_ARC_HEIGHT = 1.5f;
	static constexpr _float ACIENT_THROW_PULL_SPIN_TURNS = 0.75f;
	static constexpr _float ACIENT_THROW_DIRECT_SPEED = 220.f;
	static constexpr _float ACIENT_THROW_SPIN_SPEED = 14.f;
	static constexpr _float ACIENT_THROW_SLOW_SCALE = 0.2f;
	static constexpr _float ACIENT_THROW_SLOW_START_PULL_RATIO = 0.86f;
	static constexpr _float ACIENT_THROW_SLOW_POST_LAUNCH_DURATION = 0.08f;
	static constexpr _float ACIENT_THROW_SLOW_BLEND_IN = 0.06f;
	static constexpr _float ACIENT_THROW_SLOW_BLEND_OUT = 0.1f;
	static constexpr _float ACIENT_THROW_SLOW_MAX_UNSCALED_DURATION = 1.25f;
	static constexpr _float ACIENT_THROW_FOV_Y = 58.f;
	static constexpr _float ACIENT_THROW_FOV_BLEND_IN_RESPONSE = 6.f;
	static constexpr _float ACIENT_THROW_FOV_BLEND_OUT_RESPONSE = 2.5f;
};


NS_END
