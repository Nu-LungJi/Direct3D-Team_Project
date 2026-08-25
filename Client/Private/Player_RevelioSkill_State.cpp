#include "pch.h"
#include "Player_RevelioSkill_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "ComSound.h"
#include "PlayerAnimationRatioGuard.h"

NS_USING(Client)

void CPlayer_RevelioSkill_State::Enter(CStateMachine* pStateMachine)
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

	CacheAnimationIndices(*pPlayer);
	if (m_iRevelioAnimation < 0)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	if (auto* pSound = pPlayer->GetSound())
	{
		pSound->PlaySlot2D(
			E::StringID{ "PLAYER_VOICE_REVELIO" },
			"./Resources/SampleClient/Sound/Player/Spell/Revelio/Revelio_Man.wav",
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 0.1f,
				.fPitch = 1.f,
				.iPriority = 64,
				.bLoop = false
			});
	}

	SetSkillControl(*pPlayer, true, true, false);
	pPlayer->SetCurrentMoveSpeed(0.f);
	pAnimator->Play_Anim(m_iRevelioAnimation, false, 0.2f);

	m_ePhase = PHASE::CAST;
	m_fAnimationRatio = 0.f;
}

void CPlayer_RevelioSkill_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	m_iRevelioAnimation = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Spell_Revelio_anm.bin");

	m_bAnimationIndicesCached = m_iRevelioAnimation >= 0;
}

void CPlayer_RevelioSkill_State::Update(CStateMachine* pStateMachine,_float)
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

	m_fAnimationRatio = PlayerAnimationRatioGuard::Sanitize(pAnimator->GetPlayAnimRatio());
	pPlayer->SetCurrentMoveSpeed(0.f);

	switch (m_ePhase)
	{
	case PHASE::CAST:
		if (m_fAnimationRatio >= REVEAL_START_RATIO)
		{
			m_ePhase = PHASE::REVEAL;
			// TODO: 탐색 범위 판정 및 오브젝트 강조 연출 호출
		}
		break;

	case PHASE::REVEAL:
		if (m_fAnimationRatio >= RECOVERY_START_RATIO)
			m_ePhase = PHASE::RECOVERY;
		break;

	case PHASE::RECOVERY:
		if (m_fAnimationRatio >= RECOVERY_EXIT_RATIO || pAnimator->GetFinish())
			RequestLocomotion(pStateMachine);
		break;
	}
}

void CPlayer_RevelioSkill_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		ResetSkillControl(*pPlayer);

	m_ePhase = PHASE::CAST;
	m_fAnimationRatio = 0.f;
}

SPtr<CPlayer_RevelioSkill_State> CPlayer_RevelioSkill_State::Create()
{
	return ToSPtr(new CPlayer_RevelioSkill_State{});
}
