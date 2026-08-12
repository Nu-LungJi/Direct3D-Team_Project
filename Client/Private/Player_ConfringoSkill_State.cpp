#include "pch.h"
#include "Player_ConfringoSkill_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "ComSound.h"
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
	if (!PlayTargetAttack(*pPlayer, true, 0.14f))
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	if (auto* pSound = pPlayer->GetSound())
	{
		pSound->PlaySlot2D(
			E::StringID{ "PLAYER_VOICE_CONFRINGO" },
			"./Resources/SampleClient/Sound/Player/SkillEffect/Confringo/Confringo_Voice_Male.wav",
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 1.f,
				.fPitch = 1.f,
				.iPriority = 80,
				.bLoop = false
			});
	}

	pPlayer->SetCurrentMoveSpeed(0.f);
	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_bCastingCueReached = false;
	m_bProjectileCueReached = false;

}

void CPlayer_ConfringoSkill_State::Update(
	CStateMachine* pStateMachine, _float)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer || !pPlayer->GetAnimator())
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	m_fAnimRatio = PlayerAnimationRatioGuard::Sanitize(
		pPlayer->GetAnimator()->GetPlayAnimRatio());
	pPlayer->SetCurrentMoveSpeed(0.f);

	if (!m_bCastingCueReached &&
		m_fAnimRatio >= CASTING_EFFECT_RATIO)
	{
		m_bCastingCueReached = true;
		pPlayer->StartConfringoCastEffect();
	}

	if (!m_bProjectileCueReached &&
		m_fAnimRatio >= PROJECTILE_RELEASE_RATIO)
	{
		m_bProjectileCueReached = true;
		if (!pPlayer->FireConfringoProjectile())
		{
			DEBUG_LOG(
				"[Confringo] Failed to spawn projectile.\n");
		}
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
	{
		pPlayer->StopConfringoCastEffect();
		ResetSkillControl(*pPlayer);
	}

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_bCastingCueReached = false;
	m_bProjectileCueReached = false;
}

SPtr<CPlayer_ConfringoSkill_State> CPlayer_ConfringoSkill_State::Create()
{
	return ToSPtr(new CPlayer_ConfringoSkill_State{});
}
