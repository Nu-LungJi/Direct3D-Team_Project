#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)

class CPlayer;

class CPlayer_Skill_State final : public CState
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Skill_State, CState)

private:
	CPlayer_Skill_State() = default;
	~CPlayer_Skill_State() override = default;

public:


	void Enter(CStateMachine* pStateMachine) override;
	void Exit(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

	static SPtr<CPlayer_Skill_State> Create();

private:
	int32_t FindAnimationIndex(const CPlayer& player, _string_view sAnimationName) const;


};

NS_END
