#pragma once
#include "Player_SkillStateBase.h"

NS_BEGIN(Client)

class CPlayer_BombardaSkill_State final : public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_BombardaSkill_State, CPlayer_SkillStateBase)

private:
	CPlayer_BombardaSkill_State() = default;
	~CPlayer_BombardaSkill_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Exit(CStateMachine* pStateMachine) override;

	static SPtr<CPlayer_BombardaSkill_State> Create();

private:
	enum class PHASE : uint8_t
	{
		CAST,
		RELEASE,
		RECOVERY
	};

	PHASE m_ePhase{ PHASE::CAST };
	_float m_fAnimRatio{};
	_bool m_bCastingEffectCueReached{};
	_bool m_bReleaseEffectCueReached{};
	_bool m_bImpactEffectCueReached{};

	static constexpr _float CASTING_EFFECT_RATIO = 0.08f;
	static constexpr _float RELEASE_EFFECT_RATIO = 0.28f;
	static constexpr _float IMPACT_EFFECT_RATIO = 0.34f;
	static constexpr _float RELEASE_TO_RECOVERY_RATIO = 0.38f;
	static constexpr _float RECOVERY_EXIT_RATIO = 0.52f;
};

NS_END
