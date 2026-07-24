#include "pch.h"
#include "Player_Attack_State.h"

#include "Player.h"
#include "Player_StateMachine.h"
#include "ComAnimator.h"
#include "ComCharacterMotor.h"
#include "ComCharacterMoveIntent.h"
#include "ComModelInstance.h"
#include "ResModel.h"
#include "ResModelAnim.h"

NS_USING(Client)

void CPlayer_Attack_State::Enter(CStateMachine* pStateMachine)
{
	
}

void CPlayer_Attack_State::Exit(CStateMachine* pStateMachine)
{

}

void CPlayer_Attack_State::Update(
	CStateMachine* pStateMachine,
	_float fTimeDelta)
{
	
}

int32_t CPlayer_Attack_State::FindAnimationIndex(
	const CPlayer& player,
	_string_view sAnimationName) const
{
	auto* modelInstance = player.GetModelInstance();
	if (!modelInstance || !modelInstance->GetModel())
		return -1;

	const auto& animations = modelInstance->GetModel()->GetAnimations();
	for (size_t i = 0; i < animations.size(); ++i)
	{
		if (animations[i] && animations[i]->GetAnimName() == sAnimationName)
			return static_cast<int32_t>(i);
	}

	return -1;
}


SPtr<CPlayer_Attack_State> CPlayer_Attack_State::Create()
{
	return ToSPtr(new CPlayer_Attack_State{});
}
