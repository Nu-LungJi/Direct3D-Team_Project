#pragma once
#include "Player_SkillStateBase.h"

NS_BEGIN(Client)

class CPlayer_LumosSkill_State final : public CPlayer_SkillStateBase
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_LumosSkill_State, CPlayer_SkillStateBase)

private:
	int32_t m_iStartAnimation{ -1 };
	int32_t m_iHoldAnimation{ -1 };
	int32_t m_iStopAnimation{ -1 };
	_bool m_bAnimationCached{};
	void CacheAnimation(const CPlayer& player);

	private:
	CPlayer_LumosSkill_State() = default;
	~CPlayer_LumosSkill_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Exit(CStateMachine* pStateMachine) override;

	static SPtr<CPlayer_LumosSkill_State> Create();
};

NS_END
