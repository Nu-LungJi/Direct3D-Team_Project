#pragma once
#include "Player_SkillStateBase.h"

NS_BEGIN(Client)

class CPlayer_ConfringoSkill_State final : public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_ConfringoSkill_State, CPlayer_SkillStateBase)

private:
	CPlayer_ConfringoSkill_State() = default;
	~CPlayer_ConfringoSkill_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Exit(CStateMachine* pStateMachine) override;
	static SPtr<CPlayer_ConfringoSkill_State> Create();

private:
	enum class PHASE : uint8_t
	{
		CAST,
		RECOVERY
	};

	PHASE m_ePhase{ PHASE::CAST };
	_float m_fAnimRatio{};
	_bool m_bCastingCueReached{};
	_bool m_bProjectileCueReached{};

	static constexpr _float CASTING_EFFECT_RATIO = 0.08f;
	static constexpr _float PROJECTILE_RELEASE_RATIO = 0.28f;
	static constexpr _float CAST_TO_RECOVERY_RATIO = 0.32f;
	static constexpr _float RECOVERY_EXIT_RATIO = 0.45f;
};

NS_END
