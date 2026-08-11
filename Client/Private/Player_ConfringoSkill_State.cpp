#include "pch.h"
#include "Player_ConfringoSkill_State.h"

NS_USING(Client)

void CPlayer_ConfringoSkill_State::Enter(CStateMachine*)
{
}

void CPlayer_ConfringoSkill_State::Update(CStateMachine*, _float)
{
}

void CPlayer_ConfringoSkill_State::Exit(CStateMachine*)
{
}

SPtr<CPlayer_ConfringoSkill_State> CPlayer_ConfringoSkill_State::Create()
{
	return ToSPtr(new CPlayer_ConfringoSkill_State{});
}
