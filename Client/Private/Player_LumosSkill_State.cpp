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
	if (m_iCastAnimation < 0)
	{
		pPlayer->ToggleLumos();
		RequestLocomotion(pStateMachine);
		return;
	}
	SetSkillControl(*pPlayer, true, false, false);
	pPlayer->SetCurrentMoveSpeed(0.f);
	pPlayer->GetAnimator()->Play_Anim(m_iCastAnimation, false, 0.15f);
	m_bToggleApplied = false;
}

void CPlayer_LumosSkill_State::Update(CStateMachine* pStateMachine, _float)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}
	const _float fRatio = PlayerAnimationRatioGuard::Sanitize(
		pPlayer->GetAnimator()->GetPlayAnimRatio());
	if (!m_bToggleApplied && fRatio >= TOGGLE_RATIO)
	{
		pPlayer->ToggleLumos();
		m_bToggleApplied = true;
	}
	if (fRatio >= EXIT_RATIO || pPlayer->GetAnimator()->GetFinish())
		RequestLocomotion(pStateMachine);
}

void CPlayer_LumosSkill_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		ResetSkillControl(*pPlayer);
	m_bToggleApplied = false;
}

void CPlayer_LumosSkill_State::CacheAnimation(const CPlayer& player)
{
	if (m_bAnimationCached)
		return;
	m_iCastAnimation = FindAnimationIndex(
		player, "AN_ProfessorSharp_MasterRig_Hu_BM_Spell_Revelio_anm.bin");
	m_bAnimationCached = true;
}

SPtr<CPlayer_LumosSkill_State> CPlayer_LumosSkill_State::Create()
{
	return ToSPtr(new CPlayer_LumosSkill_State{});
}
