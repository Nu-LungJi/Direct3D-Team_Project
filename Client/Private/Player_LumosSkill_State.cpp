#include "pch.h"
#include "Player_LumosSkill_State.h"
#include "Player.h"
#include "ComAnimator.h"

NS_USING(Client)

void CPlayer_LumosSkill_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	CacheAnimation(*pPlayer);
	SetSkillControl(*pPlayer, false, false, false, false);

	const _bool bTurningOff = pPlayer->IsLumosActive();
	pPlayer->SetLumosActive(!bTurningOff);

	const int32_t iToggleAnimation = bTurningOff
		? m_iStopAnimation
		: m_iStartAnimation;
	if (iToggleAnimation >= 0)
	{
		if (pPlayer->PlayUpperBodyAnimation(
			iToggleAnimation, "RightArm", 1, false, 0.14f))
		{
			pPlayer->GetAnimator()->SetUpperAnimationFadeOutDuration(0.3f);
		}
	}
}

void CPlayer_LumosSkill_State::Update(CStateMachine* pStateMachine, _float)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}


	pPlayer->PrepareLocomotionResume();
	RequestLocomotion(pStateMachine);
}

void CPlayer_LumosSkill_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		ResetSkillControl(*pPlayer);
}

void CPlayer_LumosSkill_State::CacheAnimation(const CPlayer& player)
{
	if (m_bAnimationCached)
		return;
	m_iStartAnimation = FindAnimationIndex(
		player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Lumos_Start_anm.bin");
	m_iStopAnimation = FindAnimationIndex(
		player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Lumos_Stop_anm.bin");
	m_bAnimationCached = true;
}

SPtr<CPlayer_LumosSkill_State> CPlayer_LumosSkill_State::Create()
{
	return ToSPtr(new CPlayer_LumosSkill_State{});
}
