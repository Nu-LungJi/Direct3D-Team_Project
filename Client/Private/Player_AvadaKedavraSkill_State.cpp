#include "pch.h"
#include "Player_AvadaKedavraSkill_State.h"

NS_USING(Client)

void CPlayer_AvadaKedavraSkill_State::Enter(CStateMachine*)
{
}

void CPlayer_AvadaKedavraSkill_State::Update(CStateMachine*, _float)
{
}

void CPlayer_AvadaKedavraSkill_State::Exit(CStateMachine*)
{
}

SPtr<CPlayer_AvadaKedavraSkill_State> CPlayer_AvadaKedavraSkill_State::Create()
{
	return ToSPtr(new CPlayer_AvadaKedavraSkill_State{});
}
