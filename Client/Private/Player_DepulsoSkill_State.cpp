#include "pch.h"
#include "Player_DepulsoSkill_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "PlayerAnimationRatioGuard.h"
#include "Monster.h"
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
	if (auto pMonster = CGameInstance::Get().GetGameObjectByHandleT<CMonster>(pPlayer->GetTargetHandle()))
		pMonster->Check_Table(PLAYER_SKILL_TYPE::DEPULSO);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_fPreviousAnimRatio = 0.f;
	CGameInstance::Get().PlayEffect("Depulso", *pPlayer->GetTransform().GetWorldMatrix());

}

void CPlayer_DepulsoSkill_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	// 고쳐야 할거 
	m_DepulsoCast_Animation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Charge_Depulso_anm.bin");
	m_DepulsoEnd_Animation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Charge_Depulso_anm.bin");
	m_AttackFail_Animation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_LF_Atk_Heavy_Fail_anm.bin");

	m_bAnimationIndicesCached =
		m_DepulsoCast_Animation >= 0 &&
		m_DepulsoEnd_Animation >= 0 &&
		m_AttackFail_Animation >= 0;
}

void CPlayer_DepulsoSkill_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
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
			m_fPreviousAnimRatio = 0.f;
			if (!PlayRandomTargetAttack(*pPlayer))
				RequestLocomotion(pStateMachine);
			m_fAnimRatio = 0.f;
		}
		break;

	case PHASE::ATTACK:
	{
		const _float fMoveTime =
			PlayerAnimationRatioGuard::CalculateActiveDeltaTime(
				m_fPreviousAnimRatio,
				m_fAnimRatio,
				ATTACK_MOVE_START_RATIO,
				ATTACK_MOVE_END_RATIO,
				fTimeDelta);

		if (fMoveTime > 0.f)
		{
			if (auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(
				pPlayer->GetTargetHandle()))
			{
				_float3 vTargetDirection{};
				XMStoreFloat3(
					&vTargetDirection,
					pTarget->GetTransform().GetState(STATE::POSITION) -
					pPlayer->GetTransform().GetState(STATE::POSITION));
				pPlayer->ApplyDirectionalMovement(
					vTargetDirection,
					ATTACK_MOVE_SPEED,
					fMoveTime);
			}
		}

		if (m_fAnimRatio >= CAST_END_RATIO) {
	/*		if (!TryApplySkillToTarget(*pPlayer, PLAYER_SKILL_TYPE::DEPULSO))
			{
				m_ePhase = PHASE::ATTACK_FAILED;
				pAnimator->Play_Anim(m_AttackFail_Animation, false, 0.2f);
				break;
			}*/

			// 밀기 시작
			m_ePhase = PHASE::PUSH;
			pAnimator->Play_Anim(m_DepulsoCast_Animation, false, 0.2f);
		}

		break;
	}

	case PHASE::ATTACK_FAILED:
		if (pAnimator->GetFinish())
			RequestLocomotion(pStateMachine);
		break;

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

	m_fPreviousAnimRatio = m_fAnimRatio;
}

void CPlayer_DepulsoSkill_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		ResetSkillControl(*pPlayer);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_fPreviousAnimRatio = 0.f;
}

SPtr<CPlayer_DepulsoSkill_State> CPlayer_DepulsoSkill_State::Create()
{
	return ToSPtr(new CPlayer_DepulsoSkill_State{});
}
