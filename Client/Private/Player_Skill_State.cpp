#include "pch.h"
#include "Player_Skill_State.h"

#include "Player.h"
#include "Player_StateMachine.h"
#include "ComAnimator.h"
#include "ComCharacterMoveIntent.h"
#include "ComModelInstance.h"
#include "ResModel.h"
#include "ResModelAnim.h"

NS_USING(Client)

void CPlayer_Skill_State::Enter(CStateMachine* pStateMachine)
{
	
}

void CPlayer_Skill_State::Exit(CStateMachine* pStateMachine)
{

}

void CPlayer_Skill_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	
}

int32_t CPlayer_Skill_State::FindAnimationIndex(const CPlayer& player, _string_view sAnimationName) const
{
	
	return -1;
}

SPtr<CPlayer_Skill_State> CPlayer_Skill_State::Create()
{
	return ToSPtr(new CPlayer_Skill_State{});
}
