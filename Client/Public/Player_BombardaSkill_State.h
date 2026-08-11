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
};

NS_END
