#include "pch.h"
#include "Player_LumosSkill_State.h"

NS_USING(Client)

void CPlayer_LumosSkill_State::Enter(CStateMachine*)
{
}

void CPlayer_LumosSkill_State::Update(CStateMachine*, _float)
{
}

void CPlayer_LumosSkill_State::Exit(CStateMachine*)
{
}

SPtr<CPlayer_LumosSkill_State> CPlayer_LumosSkill_State::Create()
{
	return ToSPtr(new CPlayer_LumosSkill_State{});
}
