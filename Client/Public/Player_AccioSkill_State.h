
#pragma once
#include "Player_SkillStateBase.h"


NS_BEGIN(Client)

class CAccioBall;
class CPlayer;

class CPlayer_AccioSkill_State final : public CPlayer_SkillStateBase
{
public:
	enum class ACCIOSTATE
	{
		MONSTER,
		OBJECT,
		END
	};
public:
	DECLARE_DERIVED_TYPE(CPlayer_AccioSkill_State, CPlayer_SkillStateBase)


private:
	CPlayer_AccioSkill_State() = default;
	~CPlayer_AccioSkill_State() override = default;

public:
	void Enter(CStateMachine* stateMachine) override;
	void Update(CStateMachine* stateMachine, _float deltaTime) override;
	void Exit(CStateMachine* stateMachine) override;

	static SPtr<CPlayer_AccioSkill_State> Create();

private:
	void CacheAnimationIndices(const CPlayer& player);
	_bool EnterObjectAccio(CPlayer& player, CAccioBall& ball);
	void UpdateObjectAccio(CStateMachine* pStateMachine, CPlayer& player,
		_float fTimeDelta);
	void UpdateObjectAnimation(CPlayer& player, CAccioBall* pBall,
		_bool bPullRequested);
	void UpdateObjectPullEffect(CPlayer& player, CAccioBall* pBall,
		_float fTimeDelta, _bool bPullRequested);
	void UpdateObjectGrabEffect(CAccioBall* pBall, _float fTimeDelta,
		_bool bPullRequested);
	void ReleaseObjectControl(CPlayer& player);
	void StopObjectEffects();
	void ResetObjectState();
private:
	enum class PHASE
	{
		CAST,
		ATTACK,
		ATTACK_FAILED,
		PULL,
		RECOVERY
	};

	_bool	m_bAnimationIndicesCached = false;

	int32_t m_AccioCast_Animation{};
	int32_t m_AccioEnd_Animation{};
	int32_t m_AttackFail_Animation{};

	PHASE m_ePhase = PHASE::CAST;
	ACCIOSTATE m_eAccio = ACCIOSTATE::END;
	static constexpr _float CAST_START_RATIO = 0.f;
	static constexpr _float CAST_END_RATIO = 0.30f;
	static constexpr _float MONSTER_PULL_TIME = 0.f;
	static constexpr _float ATTACK_END_RATIO = 0.7f;
	static constexpr _float RECOVERY_EXIT_RATIO = 0.3f;
	_float	m_fAnimationRatio = 0.f;
	_bool	m_bPulling = true;
	EFFECT_INSTANCE_ID m_iAccioEffectID{ INVALID_EFFECT_INSTANCE_ID };

	// [LSY] 공 아씨오는 같은 상태 안에서 별도 수명으로 관리한다.
	CHandle m_hObjectBall{};
	EFFECT_INSTANCE_ID m_iObjectPullEffectID{ INVALID_EFFECT_INSTANCE_ID };
	EFFECT_INSTANCE_ID m_iObjectGrabEffectID{ INVALID_EFFECT_INSTANCE_ID };
	_float m_fObjectPullBlend{};
	_float m_fObjectGrabBlend{};
	_bool m_bObjectAnimationPlaying{};
	_bool m_bObjectAnimationHeld{};
	_bool m_bObjectAnimationReleasing{};
	_bool m_bObjectFacingActive{};

	static constexpr _float OBJECT_FACING_TURN_SPEED = 720.f;
	static constexpr _float OBJECT_PULL_HOLD_RATIO = 0.45f;
	static constexpr _float OBJECT_PULL_FADE_IN_TIME = 0.1f;
	static constexpr _float OBJECT_PULL_FADE_OUT_TIME = 0.35f;
	static constexpr _float OBJECT_GRAB_FADE_IN_TIME = 0.25f;
	static constexpr _float OBJECT_GRAB_FADE_OUT_TIME = 0.5f;
	static constexpr _float OBJECT_GRAB_MAX_ALPHA = 0.5882353f;

};


NS_END
