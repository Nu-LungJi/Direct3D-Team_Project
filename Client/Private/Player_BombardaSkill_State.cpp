#include "pch.h"
#include "Player_BombardaSkill_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "PlayerAnimationRatioGuard.h"

NS_USING(Client)

void CPlayer_BombardaSkill_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !HasTarget(*pPlayer) || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	// 봄바르다는 시전 중 WASD를 막고 공격 애니메이션의 회전 Root Motion만 사용한다.
	SetSkillControl(*pPlayer, true, false, true, true);
	pPlayer->SetCurrentMoveSpeed(0.f);

	// 타겟의 현재 360도 방향에 대응하는 강공격 캐스팅 애니메이션을 재생한다.
	if (!PlayTargetAttack(*pPlayer, true, 0.14f))
	{
		RequestLocomotion(pStateMachine);
		return;
	}
	// 공용 공격 재생 함수가 translation도 켜므로 봄바르다는 회전만 남긴다.
	pPlayer->SetRootMotionTranslationActive(false);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_bCastingEffectCueReached = false;
	m_bReleaseEffectCueReached = false;
	m_bImpactEffectCueReached = false;
}

void CPlayer_BombardaSkill_State::Update(CStateMachine* pStateMachine, _float)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	auto* pAnimator = pPlayer->GetAnimator();
	m_fAnimRatio = PlayerAnimationRatioGuard::Sanitize(
		pAnimator->GetPlayAnimRatio());
	pPlayer->SetCurrentMoveSpeed(0.f);

	if (!m_bCastingEffectCueReached &&
		m_fAnimRatio >= CASTING_EFFECT_RATIO)
	{
		m_bCastingEffectCueReached = true;
		// [봄바르다 이펙트 1 - Casting]
		// 완드를 크게 휘두르기 시작하는 위치. 완드 끝 캐스팅 파티클을 재생한다.
	}

	if (!m_bReleaseEffectCueReached &&
		m_fAnimRatio >= RELEASE_EFFECT_RATIO)
	{
		m_bReleaseEffectCueReached = true;
		m_ePhase = PHASE::RELEASE;
		// [봄바르다 이펙트 2 - Release]
		// 완드가 타겟을 향하는 발사 프레임. 머즐 및 투사체 생성 코드를 넣는다.
	}

	if (!m_bImpactEffectCueReached &&
		m_fAnimRatio >= IMPACT_EFFECT_RATIO)
	{
		m_bImpactEffectCueReached = true;
		// [봄바르다 이펙트 3 - Impact]
		// 실제 투사체를 붙이면 이 타이밍 주석 대신 투사체 충돌 지점에서 폭발을 재생한다.
	}

	switch (m_ePhase)
	{
	case PHASE::CAST:
		break;

	case PHASE::RELEASE:
		if (m_fAnimRatio >= RELEASE_TO_RECOVERY_RATIO)
			m_ePhase = PHASE::RECOVERY;
		break;

	case PHASE::RECOVERY:
		if (m_fAnimRatio >= RECOVERY_EXIT_RATIO || pAnimator->GetFinish())
			RequestLocomotion(pStateMachine);
		break;
	}
}

void CPlayer_BombardaSkill_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		ResetSkillControl(*pPlayer);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_bCastingEffectCueReached = false;
	m_bReleaseEffectCueReached = false;
	m_bImpactEffectCueReached = false;
}

SPtr<CPlayer_BombardaSkill_State> CPlayer_BombardaSkill_State::Create()
{
	return ToSPtr(new CPlayer_BombardaSkill_State{});
}
