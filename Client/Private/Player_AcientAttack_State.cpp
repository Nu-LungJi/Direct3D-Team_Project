#include "pch.h"
#include "Player_AcientAttack_State.h"

#include "Player.h"
#include "ComCharacterMoveIntent.h"

#include "ComAnimator.h"
NS_USING(Client)

void CPlayer_AcientAttack_State::Enter(CStateMachine* pStateMachine)
{
	

}


void CPlayer_AcientAttack_State::CacheAnimationIndices(const CPlayer& player)
{
	

}
void CPlayer_AcientAttack_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	
}

void CPlayer_AcientAttack_State::Exit(CStateMachine* pStateMachine)
{
	
}

SPtr<CPlayer_AcientAttack_State> CPlayer_AcientAttack_State::Create()
{
	return ToSPtr(new CPlayer_AcientAttack_State{});
}
