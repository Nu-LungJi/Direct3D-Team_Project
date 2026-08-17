#include "pch.h"
#include "Player_LumosSkill_State.h"
#include "Player.h"
#include "ComAnimator.h"
#include "PlayerAnimationRatioGuard.h"

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
	m_bTurningOff = pPlayer->IsLumosActive();
	m_bToggleApplied = false;

	if (m_bTurningOff)
	{
		pPlayer->SetLumosActive(false);
		m_bToggleApplied = true;
		if (m_iStopAnimation >= 0 && pPlayer->PlayUpperBodyAnimation(
			m_iStopAnimation, "RightArm", 1, false, 0.1f))
		{
			SetSkillControl(*pPlayer, true, false, false, true);
			return;
		}

		RequestLocomotion(pStateMachine);
		return;
	}

	if (m_iStartAnimation < 0 || !pPlayer->PlayUpperBodyAnimation(
		m_iStartAnimation, "RightArm", 1, false, 0.12f))
	{
		pPlayer->SetLumosActive(true);
		m_bToggleApplied = true;
		if (m_iHoldAnimation >= 0)
			pPlayer->PlayUpperBodyAnimation(
				m_iHoldAnimation, "RightArm", 1, true, 0.12f);
		RequestLocomotion(pStateMachine);
		return;
	}


	SetSkillControl(*pPlayer, true, false, false, true);
}

void CPlayer_LumosSkill_State::Update(CStateMachine* pStateMachine, _float)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}
	auto* pAnimator = pPlayer->GetAnimator();
	const _float fRatio = PlayerAnimationRatioGuard::Sanitize(
		pAnimator->GetUpperAnimRatio());
	if (fRatio >= MOVEMENT_RELEASE_RATIO)
		pPlayer->SetMovementLocked(false);

	if (!m_bTurningOff && !m_bToggleApplied && fRatio >= TOGGLE_RATIO)
	{
		pPlayer->SetLumosActive(true);
		m_bToggleApplied = true;
	}

	if (fRatio < EXIT_RATIO && !pAnimator->IsUpperAnimationFinished())
		return;

	if (!m_bTurningOff)
	{
		if (!m_bToggleApplied)
			pPlayer->SetLumosActive(true);
		if (m_iHoldAnimation >= 0)
			pPlayer->PlayUpperBodyAnimation(
				m_iHoldAnimation, "RightArm", 1, true, 0.1f);
	}
	else
	{
		pAnimator->Stop_UpperAnim(0.1f);
	}

	if (!RequestLocomotion(pStateMachine) && !m_bTurningOff)
	{
		pPlayer->SetLumosActive(false);
		pAnimator->Stop_UpperAnim(0.1f);
	}
}

void CPlayer_LumosSkill_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		ResetSkillControl(*pPlayer);
	m_bToggleApplied = false;
	m_bTurningOff = false;
}

void CPlayer_LumosSkill_State::CacheAnimation(const CPlayer& player)
{
	if (m_bAnimationCached)
		return;
	m_iStartAnimation = FindAnimationIndex(
		player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Lumos_Start_anm.bin");
	m_iHoldAnimation = FindAnimationIndex(
		player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Lumos_Hold_anm.bin");
	if (m_iHoldAnimation < 0)
	{
		m_iHoldAnimation = FindAnimationIndex(
			player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Lumos_Hold_01_anm.bin");
	}
	m_iStopAnimation = FindAnimationIndex(
		player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Lumos_Stop_anm.bin");
	m_bAnimationCached = true;
}

SPtr<CPlayer_LumosSkill_State> CPlayer_LumosSkill_State::Create()
{
	return ToSPtr(new CPlayer_LumosSkill_State{});
}
