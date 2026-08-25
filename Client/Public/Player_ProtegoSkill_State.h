#pragma once
#include "Player_SkillStateBase.h"

NS_BEGIN(Client)

class CPlayer_ProtegoSkill_State final : public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_ProtegoSkill_State, CPlayer_SkillStateBase)

private:
	CPlayer_ProtegoSkill_State() = default;
	~CPlayer_ProtegoSkill_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Exit(CStateMachine* pStateMachine) override;

	static SPtr<CPlayer_ProtegoSkill_State> Create();

private:
	static constexpr _float PROTEGO_DURATION = 1.5f;
	static constexpr _float PHASE_EXIT_RATIO = 0.92f;
	void CacheAnimationIndices(const CPlayer& player);

	_float m_fAnimationRatio{};
	int32_t m_iProtegoStartAnimation{ -1 };
	_bool m_bAnimationIndicesCached{};
};

NS_END
