#include "pch.h"
#include "Player_DescendoSkill_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "PlayerAnimationRatioGuard.h"

NS_USING(Client)

void CPlayer_DescendoSkill_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	if (!HasValidTarget(*pPlayer))
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	CacheAnimationIndices(*pPlayer);

	if (m_DescendoCast_Animation < 0 || m_DescendoEnd_Animation < 0)
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

	SetSkillControl(*pPlayer, true, true, false);
	pPlayer->SetCurrentMoveSpeed(0.f);
	pPlayer->SetPlayerCurSKill(PLAYER_SKILL_TYPE::DESCENDO);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
}

void CPlayer_DescendoSkill_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	// 고쳐야 할거 
	m_DescendoCast_Animation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_BM_RF_Cast_Casual_Fwd_Accio_anm.bin");
	m_DescendoEnd_Animation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_AccioPull_anm");

	m_bAnimationIndicesCached = m_DescendoCast_Animation >= 0 && m_DescendoEnd_Animation >= 0;
}

void CPlayer_DescendoSkill_State::Update(CStateMachine* pStateMachine, _float)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	pPlayer->SetCurrentMoveSpeed(0.f);

	auto* pAnimator = pPlayer->GetAnimator();
	if (!pAnimator)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	m_fAnimRatio = PlayerAnimationRatioGuard::Sanitize(pAnimator->GetPlayAnimRatio());

	switch (m_ePhase)
	{
	case PHASE::CAST:
		if (m_fAnimRatio >= CAST_START_RATIO)
		{
			m_ePhase = PHASE::ATTACK;
			pAnimator->Play_Anim(m_DescendoCast_Animation,false,0.24f);
		}
		break;

	case PHASE::ATTACK:
		if (m_fAnimRatio >= ATTACK_END_RATIO)
		{
			m_ePhase = PHASE::RECOVERY;
			pAnimator->Play_Anim(m_DescendoEnd_Animation, false, 0.25f);
			pAnimator->GetCurAnimState().fSpeed = 1.f;
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
