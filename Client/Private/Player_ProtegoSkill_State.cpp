#include "pch.h"
#include "Player_ProtegoSkill_State.h"

#include "GameInstance.h"
#include "Player.h"
#include "ComAnimator.h"
#include "ComSound.h"
#include "PlayerAnimationRatioGuard.h"

NS_USING(Client)

void CPlayer_ProtegoSkill_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
		return;

	m_fAnimationRatio = 0.f;

	auto* pAnimator = pPlayer->GetAnimator();
	CacheAnimationIndices(*pPlayer);
	if (!pAnimator || !m_bAnimationIndicesCached)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	// 프로테고 준비/유지 중에는 이동 입력을 허용한다.
	SetSkillControl(*pPlayer, false, false, false, false);
	pPlayer->ActivateProtego(PROTEGO_DURATION);
	if (auto* pSound = pPlayer->GetSound())
	{
		pSound->PlaySlot2D(
			E::StringID{ "PLAYER_VOICE_PROTEGO" },
			"./Resources/SampleClient/Sound/Player/Spell/Protego/Protego_Man.wav",
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 2.f,
				.fPitch = 1.f,
				.iPriority = 80,
				.bLoop = false
			});
	}
	// 하체 locomotion은 유지하고 몸통 위쪽으로만 프로테고를 시전한다.
	if (!pPlayer->PlayUpperBodyAnimation(
		m_iProtegoStartAnimation, "RightArm", 1, false, 0.1f))
	{
		RequestLocomotion(pStateMachine);
	}
}

void CPlayer_ProtegoSkill_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	m_iProtegoStartAnimation = FindAnimationIndex(player,
		"AN_ProfessorSharp_MasterRig_Hu_Cmbt_Protego_Start_anm.bin");
	m_bAnimationIndicesCached = m_iProtegoStartAnimation >= 0;
}

void CPlayer_ProtegoSkill_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
		return;

	auto* pAnimator = pPlayer->GetAnimator();
	if (!pAnimator)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	// 상체 레이어는 Animator가 끝까지 재생하고 자동 페이드한다.
	// 상태는 즉시 locomotion으로 복귀시켜 이동 애니메이션을 계속 갱신한다.
	pPlayer->PrepareLocomotionResume();
	RequestLocomotion(pStateMachine);
}

void CPlayer_ProtegoSkill_State::Exit(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (pPlayer)
		ResetSkillControl(*pPlayer);
	m_fAnimationRatio = 0.f;
}

SPtr<CPlayer_ProtegoSkill_State> CPlayer_ProtegoSkill_State::Create()
{
	return ToSPtr(new CPlayer_ProtegoSkill_State{});
}
