#include "pch.h"
#include "Player_AccioSkill_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "PlayerAnimationRatioGuard.h"

#include "ComCharacterMoveIntent.h"

#include "Monster.h"
NS_USING(Client)

void CPlayer_AccioSkill_State::Enter(CStateMachine* pStateMachine)
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
	pPlayer->SetPlayerCurSKill(PLAYER_SKILL_TYPE::ACCIO);
	if (auto pMonster = CGameInstance::Get().GetGameObjectByHandleT<CMonster>(pPlayer->GetTargetHandle()))
		pMonster->Check_Table(PLAYER_SKILL_TYPE::ACCIO);
	
	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_bPulling = true;
}

void CPlayer_AccioSkill_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;


	//m_AccioCast_Animation = FindAnimationIndex( player, "AN_ProfessorSharp_MasterRig_Hu_BM_RF_Cast_Casual_Fwd_Accio_anm.bin");
	m_AccioCast_Animation = FindAnimationIndex( player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_AccioPull_anm.bin");
	m_AccioEnd_Animation = FindAnimationIndex( player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_AccioPull_anm.bin");

	m_bAnimationIndicesCached = m_AccioCast_Animation >= 0 && m_AccioEnd_Animation >= 0;
}

void CPlayer_AccioSkill_State::Update(CStateMachine* pStateMachine, _float deltatime)
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

	m_fAnimRatio =PlayerAnimationRatioGuard::Sanitize(pAnimator->GetPlayAnimRatio());

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
			if (!TryApplySkillToTarget(*pPlayer, PLAYER_SKILL_TYPE::ACCIO))
			{
				m_ePhase = PHASE::ATTACK_FAILED;
				break;
			}

			// 끌어 오기 시작
			m_ePhase = PHASE::PULL;
			pAnimator->Play_Anim(m_AccioCast_Animation, false, 0.2f);
		}

		break;
	}


	case PHASE::ATTACK_FAILED:
		if (pAnimator->GetFinish())
			RequestLocomotion(pStateMachine);
		break;

	case PHASE::PULL: {
		auto* Target = CGameInstance::Get().GetGameObjectByHandleT<CMonster>(pPlayer->GetTargetHandle());
		
		if (!Target)
		{
			RequestLocomotion(pStateMachine);
			return;
		}

		
		if (m_fAnimRatio >= MONSTER_PULL_TIME && m_bPulling && Target->Monster_Type(MONSTER_TYPE::NORMAL))
		{
			auto* pMoveIntent = Target->GetComponent<CComCharacterMoveIntent>("ComCharacterMoveIntent");

			if (!pMoveIntent)
			{
				m_bPulling = false;
				break;
			}

			_vector targetPos = Target->GetTransform().GetState(STATE::POSITION);

			const _vector playerPos = pPlayer->GetTransform().GetState(STATE::POSITION);

			const _vector playerLook = XMVector3Normalize(XMVectorSetY(pPlayer->GetTransform().GetState(STATE::LOOK), 0.f));

			_vector destination = playerPos + playerLook * 10.f;

			_vector offset = { 0.f,0.3f,0.f };

			targetPos = offset + targetPos;

			destination = XMVectorSetY(destination, XMVectorGetY(targetPos));

			const _float lerpRatio = 1.f - expf(-10.f * deltatime);

			const _vector nextPos = XMVectorLerp(targetPos, destination, lerpRatio);

			_float3 nextPosition{};
			XMStoreFloat3(&nextPosition, nextPos);

			pMoveIntent->RequestWarp(nextPosition);

			if (XMVectorGetX(XMVector3LengthSq(destination - targetPos)) <= 0.01f)
			{
				_float3 destinationPosition{};
				XMStoreFloat3(&destinationPosition, destination);

				pMoveIntent->RequestWarp(destinationPosition);
				m_bPulling = false;
				m_ePhase = PHASE::RECOVERY;
			}
		}
		else {
			m_bPulling = false;
			m_ePhase = PHASE::RECOVERY;
		}

		if (m_fAnimRatio >= ATTACK_END_RATIO)
			RequestLocomotion(pStateMachine);
		break;


	}

	case PHASE::RECOVERY:
		if (m_fAnimRatio >= RECOVERY_EXIT_RATIO)
			RequestLocomotion(pStateMachine);
		break;
	}
}

void CPlayer_AccioSkill_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		ResetSkillControl(*pPlayer);
	m_bPulling = false;
	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
}

SPtr<CPlayer_AccioSkill_State> CPlayer_AccioSkill_State::Create()
{
	return ToSPtr(new CPlayer_AccioSkill_State{});
}
