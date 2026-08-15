#include "pch.h"
#include "Player_AvadaKedavraSkill_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "GameInstance.h"
#include "PlayerAnimationRatioGuard.h"

NS_USING(Client)

void CPlayer_AvadaKedavraSkill_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	CacheAnimationIndices(*pPlayer);
	if (m_iCastAnimation < 0)
	{
		DEBUG_LOG("[AvadaKedavra] Cast animation was not found.\n");
		RequestLocomotion(pStateMachine);
		return;
	}

	if (auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(pPlayer->GetTargetHandle()))
	{
		_vector vTargetPosition = pTarget->GetTransform().GetState(STATE::POSITION);
		vTargetPosition = XMVectorSetY(
			vTargetPosition,
			XMVectorGetY(pPlayer->GetTransform().GetState(STATE::POSITION)));
		pPlayer->GetTransform().LookAt(vTargetPosition);
	}

	SetSkillControl(*pPlayer, true, false, false, true);
	pPlayer->SetCurrentMoveSpeed(0.f);
	pPlayer->GetAnimator()->Play_Anim(
		m_iCastAnimation, false, CAST_BLEND_DURATION);

	m_ePhase = PHASE::CAST_BEGIN;
	m_fAnimRatio = 0.f;

	// [AVADA_CAST_BEGIN] 충전 시작
	// 완드 끝 충전 이펙트와 주문 음성은 이 위치에서 시작한다.
	// 이펙트는 아직 연결하지 않는다.
}

void CPlayer_AvadaKedavraSkill_State::Update(CStateMachine* pStateMachine, _float)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	auto* pAnimator = pPlayer ? pPlayer->GetAnimator() : nullptr;
	if (!pPlayer || !pAnimator)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	pPlayer->SetCurrentMoveSpeed(0.f);
	m_fAnimRatio = PlayerAnimationRatioGuard::Sanitize(
		pAnimator->GetPlayAnimRatio());

	switch (m_ePhase)
	{
	case PHASE::CAST_BEGIN:
		if (m_fAnimRatio >= RELEASE_RATIO)
		{
			m_ePhase = PHASE::RELEASE;

			// [AVADA_RELEASE] 실제 마법 방출 프레임
			// 아바다 투사체 생성, 머즐 플래시 및 발사 사운드는 이 위치에 연결한다.
			// 이펙트와 투사체는 아직 생성하지 않는다.
		}
		break;

	case PHASE::RELEASE:
		if (m_fAnimRatio >= RECOVERY_RATIO)
		{
			m_ePhase = PHASE::RECOVERY;

			// [AVADA_RECOVERY] 조작 복구 가능 시점
			// 후딜 캔슬 또는 다음 상태 입력 허용 처리는 이 위치에 연결한다.
		}
		break;

	case PHASE::RECOVERY:
		if (m_fAnimRatio >= RECOVERY_EXIT_RATIO || pAnimator->GetFinish())
			RequestLocomotion(pStateMachine);
		break;
	}
}

void CPlayer_AvadaKedavraSkill_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
	{
		ResetSkillControl(*pPlayer);
	}

	m_ePhase = PHASE::CAST_BEGIN;
	m_fAnimRatio = 0.f;
}

void CPlayer_AvadaKedavraSkill_State::CacheAnimationIndices(
	const CPlayer& player)
{
	if (m_bAnimationsCached)
		return;

	m_iCastAnimation = FindAnimationIndex(player,
		"AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Finisher_03_Cast_anm.bin");
	m_bAnimationsCached = true;
}

SPtr<CPlayer_AvadaKedavraSkill_State> CPlayer_AvadaKedavraSkill_State::Create()
{
	return ToSPtr(new CPlayer_AvadaKedavraSkill_State{});
}
