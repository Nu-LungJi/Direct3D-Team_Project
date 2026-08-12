#include "pch.h"
#include "Player_ConfringoSkill_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "PlayerAnimationRatioGuard.h"

NS_USING(Client)

void CPlayer_ConfringoSkill_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !HasTarget(*pPlayer) || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	SetSkillControl(*pPlayer, true, true, true);
	// 콘프링고는 동작이 큰 강공격 계열 애니메이션만 사용한다.
	if (!PlayTargetAttack(*pPlayer, true, 0.14f))
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	pPlayer->SetCurrentMoveSpeed(0.f);
	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_bCastingCueReached = false;
	m_bProjectileCueReached = false;

}

void CPlayer_ConfringoSkill_State::Update(
	CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	m_fAnimRatio = PlayerAnimationRatioGuard::Sanitize(pPlayer->GetAnimator()->GetPlayAnimRatio());
	pPlayer->SetCurrentMoveSpeed(0.f);

	if (!m_bCastingCueReached &&
		m_fAnimRatio >= CASTING_EFFECT_RATIO)
	{
		m_bCastingCueReached = true;

		// [콘프링고 이펙트 1 - 캐스팅]
		// 이 위치에서 파티클 매니저로 캐스팅 이펙트를 재생한다.
		// 완드를 휘두르며 불꽃 스파클이 튀기 시작하는 구간이다.
	}

	if (!m_bProjectileCueReached &&
		m_fAnimRatio >= PROJECTILE_RELEASE_RATIO)
	{
		m_bProjectileCueReached = true;

		// [콘프링고 이펙트 2 - 머즐 스모크]
		// 이 위치에서 현재 완드 끝 소켓을 기준으로 파티클을 재생한다.
		// 투사체 발사 프레임에 연기가 링 형태로 퍼지도록 제작한다.

		// [콘프링고 이펙트 3 - 투사체]
		// 이 위치에서 콘프링고 투사체 객체를 생성한다.
		// 시작 위치는 완드 끝 소켓, 도착 위치는 락온 타깃 또는 피격 위치다.
		// 날아가는 불꽃 파티클 갱신은 투사체 클래스 내부에서 처리한다.
	}

	switch (m_ePhase)
	{
	case PHASE::CAST:

		if (m_fAnimRatio >= CAST_TO_RECOVERY_RATIO)
			m_ePhase = PHASE::RECOVERY;
		break;

	case PHASE::RECOVERY:

		if (m_fAnimRatio >= RECOVERY_EXIT_RATIO || pPlayer->GetAnimator()->GetFinish())
		{
			RequestLocomotion(pStateMachine);
		}
		break;
	}
}

void CPlayer_ConfringoSkill_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		ResetSkillControl(*pPlayer);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_bCastingCueReached = false;
	m_bProjectileCueReached = false;
}

SPtr<CPlayer_ConfringoSkill_State> CPlayer_ConfringoSkill_State::Create()
{
	return ToSPtr(new CPlayer_ConfringoSkill_State{});
}
