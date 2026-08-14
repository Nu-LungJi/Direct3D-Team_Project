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

	// [LSY] 시전 중 이동 입력을 막고 공격 애니메이션의 회전 Root Motion만 사용한다.
	SetSkillControl(*pPlayer, true, false, true, true);
	pPlayer->SetCurrentMoveSpeed(0.f);

	// [LSY] 진입 시점의 타깃 방향으로 강공격 캐스팅 애니메이션을 재생한다.
	if (!PlayTargetAttack(*pPlayer, true, 0.14f))
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	if (auto* pSound = pPlayer->GetSound())
	{
		pSound->PlaySlot2D(
			E::StringID{ "PLAYER_VOICE_BOMBARDA" },
			"./Resources/SampleClient/Sound/Player/SkillEffect/Bombarda/Bombarda_Voice_Male.wav",
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 1.f,
				.fPitch = 1.f,
				.iPriority = 80,
				.bLoop = false
			});
	}
	// [LSY] 공용 공격 함수가 이동 Root Motion도 켜므로 봄바르다는 회전만 남긴다.
	pPlayer->SetRootMotionTranslationActive(false);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_bCastingEffectCueReached = false;
	m_bReleaseEffectCueReached = false;
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
		// [LSY] 애니메이션 비율 Cue는 연출 시작 신호만 컨트롤러에 전달한다.
		m_bCastingEffectCueReached = true;
		pPlayer->StartBombardaCastEffect();
	}

	if (!m_bReleaseEffectCueReached &&
		m_fAnimRatio >= RELEASE_EFFECT_RATIO)
	{
		// [LSY] 실제 충돌과 임팩트 처리는 생성된 투사체가 담당한다.
		m_bReleaseEffectCueReached = true;
		m_ePhase = PHASE::RELEASE;
		if (!pPlayer->FireBombardaProjectile())
		{
			DEBUG_LOG(
				"[Bombarda] Failed to spawn projectile.\n");
		}
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
	{
		pPlayer->StopBombardaCastEffect();
		ResetSkillControl(*pPlayer);
	}

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_bCastingEffectCueReached = false;
	m_bReleaseEffectCueReached = false;
}

SPtr<CPlayer_BombardaSkill_State> CPlayer_BombardaSkill_State::Create()
{
	return ToSPtr(new CPlayer_BombardaSkill_State{});
}
