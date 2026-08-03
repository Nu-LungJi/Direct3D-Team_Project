#include "pch.h"
#include "Player_DescendoSkill_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "PlayerAnimationRatioGuard.h"
#include "Monster.h"
NS_USING(Client)

void CPlayer_DescendoSkill_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	if (!HasTarget(*pPlayer))
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
	SetSkillControl(*pPlayer, true, true, false);
	pPlayer->SetCurrentMoveSpeed(0.f);
	pPlayer->SetPlayerCurSKill(PLAYER_SKILL_TYPE::DESCENDO);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	CGameInstance::Get().PlayEffect("Descendo", *pPlayer->GetTransform().GetWorldMatrix());

}

void CPlayer_DescendoSkill_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	// 고쳐야 할거 
	m_DescendoCast_Animation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Slam_Dwn_anm.bin");
	m_DescendoEnd_Animation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_BM_RF_Cast_Casual_Fwd_Descendo_anm.bin");
	m_AttackFail_Animation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_LF_Atk_Heavy_Fail_anm.bin");

	m_bAnimationIndicesCached =
		m_DescendoCast_Animation >= 0 &&
		m_DescendoEnd_Animation >= 0 &&
		m_AttackFail_Animation >= 0;
}

void CPlayer_DescendoSkill_State::Update(CStateMachine* pStateMachine, _float)
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
		if (m_fAnimRatio >= CAST_START_RATIO)
		{
			m_ePhase = PHASE::ATTACK;
			if (!PlayRandomTargetAttack(*pPlayer))
				RequestLocomotion(pStateMachine);
		}
		break;

	case PHASE::ATTACK:
	{
		if (m_fAnimRatio >= CAST_END_RATIO)
		{
		/*	if (!TryApplySkillToTarget(*pPlayer, PLAYER_SKILL_TYPE::DESCENDO))
			{
				m_ePhase = PHASE::ATTACK_FAILED;
				pAnimator->Play_Anim(m_AttackFail_Animation, false, 0.2f);
				break;
			}*/
			if (auto pMonster = CGameInstance::Get().GetGameObjectByHandleT<CMonster>(pPlayer->GetTargetHandle()))
				pMonster->Check_Table(PLAYER_SKILL_TYPE::DESCENDO);
			m_ePhase = PHASE::PUSH;
			pAnimator->Play_Anim(m_DescendoCast_Animation, false, 0.25f);
			pAnimator->GetCurAnimState().fSpeed = 1.f;
		}
		break;
	}

	case PHASE::ATTACK_FAILED:
		if (pAnimator->GetFinish())
			RequestLocomotion(pStateMachine);
		break;

	case PHASE::PUSH:
		if (m_fAnimRatio >= ATTACK_END_RATIO && m_fAnimRatio != 1.f)
		{
			m_ePhase = PHASE::RECOVERY;
			RequestLocomotion(pStateMachine);
		}
		break;

	case PHASE::RECOVERY:
		if (m_fAnimRatio >= RECOVERY_EXIT_RATIO)
			RequestLocomotion(pStateMachine);
		break;
	}
}

void CPlayer_DescendoSkill_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		ResetSkillControl(*pPlayer);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
}

SPtr<CPlayer_DescendoSkill_State> CPlayer_DescendoSkill_State::Create()
{
	return ToSPtr(new CPlayer_DescendoSkill_State{});
}
