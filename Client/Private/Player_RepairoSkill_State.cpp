#include "pch.h"
#include "Player_RepairoSkill_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "PlayerAnimationRatioGuard.h"

NS_USING(Client)

void CPlayer_RepairoSkill_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	auto* pAnimator = pPlayer->GetAnimator();
	if (!pAnimator)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	CacheAnimationIndices(*pPlayer);
	if (!m_bAnimationIndicesCached)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	SetSkillControl(*pPlayer, true, true, false);
	pPlayer->SetCurrentMoveSpeed(0.f);
	pAnimator->Play_Anim(m_iRepairoStartAnimation, false, 0.2f);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
}

void CPlayer_RepairoSkill_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	m_iRepairoStartAnimation = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_AOE_Repairo_Start_anm.bin");
	m_iRepairoLoopAnimation = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_AOE_Repairo_Loop_anm.bin");
	m_iRepairoEndAnimation = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_AOE_Repairo_End_anm.bin");

	m_bAnimationIndicesCached =
		m_iRepairoStartAnimation >= 0 &&
		m_iRepairoLoopAnimation >= 0 &&
		m_iRepairoEndAnimation >= 0;
}

void CPlayer_RepairoSkill_State::Update(CStateMachine* pStateMachine, _float)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	auto* pAnimator = pPlayer->GetAnimator();
	if (!pAnimator)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	m_fAnimRatio = PlayerAnimationRatioGuard::Sanitize(pAnimator->GetPlayAnimRatio());
	pPlayer->SetCurrentMoveSpeed(0.f);

	switch (m_ePhase)
	{
	case PHASE::CAST:
		if (m_fAnimRatio >= PHASE_EXIT_RATIO || pAnimator->GetFinish())
		{
			m_ePhase = PHASE::REPAIRO;
			m_fAnimRatio = 0.f;
			pAnimator->Play_Anim(m_iRepairoLoopAnimation, false, 0.15f);
			// TODO: 탐색 범위 판정 및 오브젝트 강조 연출 호출
		}
		break;

	case PHASE::REPAIRO:
		if (m_fAnimRatio >= PHASE_EXIT_RATIO || pAnimator->GetFinish())
		{
			m_ePhase = PHASE::RECOVERY;
			m_fAnimRatio = 0.f;
			pAnimator->Play_Anim(m_iRepairoEndAnimation, false, 0.15f);
		}
		break;

	case PHASE::RECOVERY:
		if (m_fAnimRatio >= PHASE_EXIT_RATIO || pAnimator->GetFinish())
			RequestLocomotion(pStateMachine);
		break;
	}
}

void CPlayer_RepairoSkill_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		ResetSkillControl(*pPlayer);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
}

SPtr<CPlayer_RepairoSkill_State> CPlayer_RepairoSkill_State::Create()
{
	return ToSPtr(new CPlayer_RepairoSkill_State{});
}
