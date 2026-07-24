#include "pch.h"
#include "Player_Roll_State.h"

#include "Player.h"
#include "Player_StateMachine.h"
#include "ComAnimator.h"
#include "ComCharacterMoveIntent.h"
#include "ComModelInstance.h"
#include "ResModel.h"
#include "ResModelAnim.h"

NS_USING(Client)

void CPlayer_Roll_State::Enter(CStateMachine* pStateMachine)
{
	
}

void CPlayer_Roll_State::Exit(CStateMachine* pStateMachine)
{

}

void CPlayer_Roll_State::Update(CStateMachine* pStateMachine,_float fTimeDelta)
{
	
}

int32_t CPlayer_Roll_State::FindAnimationIndex(const CPlayer& player,_string_view sAnimationName) const
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

SPtr<CPlayer_Roll_State> CPlayer_Roll_State::Create()
{
	return ToSPtr(new CPlayer_Roll_State{});
}
