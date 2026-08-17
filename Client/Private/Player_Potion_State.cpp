#include "pch.h"
#include "Player_Potion_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "PlayerAnimationRatioGuard.h"

NS_USING(Client)

void CPlayer_Potion_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	CacheAnimation(*pPlayer);
	SetSkillControl(*pPlayer, false, false, false, false);

	if (m_iDrinkAnimation < 0 || !pPlayer->PlayUpperBodyAnimation(
		m_iDrinkAnimation, "Spine", 5, false, 0.05f))
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	pPlayer->GetAnimator()->SetUpperAnimationSpeed(3.2f);
}

void CPlayer_Potion_State::Update(CStateMachine* pStateMachine, _float)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	// The upper animation remains on the animator and fades automatically when
	// it finishes. Return control to locomotion immediately so lower-body
	// direction changes and movement animations continue updating underneath it.
	pPlayer->PrepareLocomotionResume();
	RequestLocomotion(pStateMachine);
}

void CPlayer_Potion_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		ResetSkillControl(*pPlayer);
}

void CPlayer_Potion_State::CacheAnimation(const CPlayer& player)
{
	if (m_bAnimationCached)
		return;

	m_iDrinkAnimation = FindAnimationIndex(
		player, "AN_ProfessorSharp_MasterRig_Hu_Env_IntrAct_Drink_Beer_anm.bin");
	m_bAnimationCached = true;
}

SPtr<CPlayer_Potion_State> CPlayer_Potion_State::Create()
{
	return ToSPtr(new CPlayer_Potion_State{});
}
