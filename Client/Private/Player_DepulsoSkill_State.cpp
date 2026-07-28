#include "pch.h"
#include "Player_DepulsoSkill_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "PlayerAnimationRatioGuard.h"

NS_USING(Client)

void CPlayer_DepulsoSkill_State::Enter(CStateMachine* pStateMachine)
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
	// Depulso 이동은 애니메이션 Root Motion이 아니라 아래의 조절 가능한
	// 전방 이동 구간을 사용한다.
	SetSkillControl(*pPlayer, true, true, false);
	pPlayer->SetCurrentMoveSpeed(0.f);
	pPlayer->SetPlayerCurSKill(PLAYER_SKILL_TYPE::DEPULSO);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
}

void CPlayer_DepulsoSkill_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	// 고쳐야 할거 
	m_DepulsoCast_Animation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Charge_Depulso_anm.bin");
	m_DepulsoEnd_Animation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Charge_Depulso_anm.bin");

	m_bAnimationIndicesCached = m_DepulsoCast_Animation >= 0 && m_DepulsoEnd_Animation >= 0;
}

void CPlayer_DepulsoSkill_State::Update(CStateMachine* pStateMachine, _float)
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
		if (m_fAnimRatio >= CAST_END_RATIO) {
			// 밀기 시작
			m_ePhase = PHASE::PUSH;
			pAnimator->Play_Anim(m_DepulsoCast_Animation, false, 0.2f);
		}

		break;
	}

	case PHASE::PUSH:
		if (m_fAnimRatio >= ATTACK_END_RATIO && m_fAnimRatio != 1.f) {
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

void CPlayer_DepulsoSkill_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		ResetSkillControl(*pPlayer);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
}

SPtr<CPlayer_DepulsoSkill_State> CPlayer_DepulsoSkill_State::Create()
{
	return ToSPtr(new CPlayer_DepulsoSkill_State{});
}
