#include "pch.h"
#include "Player_BombardaSkill_State.h"

NS_USING(Client)

void CPlayer_BombardaSkill_State::Enter(CStateMachine*)
{
}

void CPlayer_BombardaSkill_State::Update(CStateMachine*, _float)
{
}

void CPlayer_BombardaSkill_State::Exit(CStateMachine*)
{
}

SPtr<CPlayer_BombardaSkill_State> CPlayer_BombardaSkill_State::Create()
{
	return ToSPtr(new CPlayer_BombardaSkill_State{});
}
