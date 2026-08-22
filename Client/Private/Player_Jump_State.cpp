#include "pch.h"
#include "Player_Jump_State.h"

#include "Player.h"
#include "Player_StateMachine.h"
#include "PlayerAnimationRatioGuard.h"
#include "ComAnimator.h"
#include "ComCharacterMotor.h"
#include "ComCharacterMoveIntent.h"
#include "ComModelInstance.h"
#include "ComSound.h"
#include "ResModel.h"
#include "ResModelAnim.h"

NS_USING(Client)

void CPlayer_Jump_State::Enter(CStateMachine* pStateMachine)
{
	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	auto* playerStateMachine = Cast<CPlayer_StateMachine>(pStateMachine);
	if (!player || !playerStateMachine)
		return;

	auto* animator = player->GetAnimator();
	auto* moveIntent = player->GetMoveIntent();
	auto* motor = player->GetCharacterMotor();
	if (!animator || !moveIntent || !motor)
	{
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		return;
	}
	m_StartAnimations[(size_t)JUMP_STATE::IDLE] = FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_Hu_BM_JumpStart_anm.bin");
	m_StartAnimations[(size_t)JUMP_STATE::JOG] = FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_Hu_BM_Jump_LF_anm.bin");
	m_StartAnimations[(size_t)JUMP_STATE::SPRINT] = FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_Hu_BM_Jump_Fwd_LU_anm.bin");

	m_iFallAnimation = FindAnimationIndex(*player,"AN_ProfessorSharp_MasterRig_Hu_BM_Jump_Fall_anm.bin");

	m_LandAnimations[(size_t)JUMP_STATE::IDLE] = FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_Hu_BM_Land_Soft_v2_anm.bin");
	m_LandAnimations[(size_t)JUMP_STATE::JOG] = FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_Hu_Sneak_Land_2Jog_v4_anm.bin");
	m_LandAnimations[(size_t)JUMP_STATE::SPRINT] = FindAnimationIndex(*player, "AN_ProfessorSharp_MasterRig_Hu_BM_Land_2Sprint_v2_anm.bin");
	player->SetMovementLocked(false);
	player->SetRootMotionRotationActive(false);
	player->SetRootMotionTranslationActive(false);

	m_bWasAirborne = !motor->IsGrounded();
	if (m_bWasAirborne)
	{
		// Locomotion에서 절벽 낙하로 들어온 경우에는 점프 힘을 주지 않는다.
		PlayFall(*player);
		return;
	}

	m_ePhase = PHASE::START;
	moveIntent->RequestJump();
	if (auto* pSound = player->GetSound())
	{
		static constexpr const char* JUMP_VOICES[] =
		{
			"./Resources/SampleClient/Sound/Player/Movement/Jump/Player_JumpVoice_01.wav",
			"./Resources/SampleClient/Sound/Player/Movement/Jump/Player_JumpVoice_02.wav",
		};
		const int iSoundIndex = Engine::RandInt(
			0, static_cast<int>(std::size(JUMP_VOICES)) - 1);
		pSound->PlaySlot2D(
			E::StringID{ "PLAYER_JUMP_VOICE" },
			JUMP_VOICES[iSoundIndex],
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 0.1f,
				.fPitch = 1.05f,
				.iPriority = 80,
				.bLoop = false
			},
			SOUND_SLOT_PLAY_MODE::OVERLAP);
	}

	const _float fMoveSpeed = player->GetCurrentMoveSpeed();
	JUMP_STATE eJumpState = JUMP_STATE::IDLE;
	if (fMoveSpeed > 10.f)
		eJumpState = JUMP_STATE::SPRINT;
	else if (fMoveSpeed > std::numeric_limits<_float>::epsilon())
		eJumpState = JUMP_STATE::JOG;

	int32_t iStartAnimation =
		m_StartAnimations[static_cast<size_t>(eJumpState)];
	if (iStartAnimation < 0)
		iStartAnimation = m_StartAnimations[static_cast<size_t>(JUMP_STATE::IDLE)];

	if (iStartAnimation >= 0)
		animator->Play_Anim(iStartAnimation, false, 0.08f);
	else
		PlayFall(*player);
}

void CPlayer_Jump_State::Exit(CStateMachine* pStateMachine)
{
	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!player)
		return;

	player->SetMovementLocked(false);
	player->SetRootMotionRotationActive(false);
	player->SetRootMotionTranslationActive(false);
	m_bWasAirborne = false;
}

void CPlayer_Jump_State::Update(CStateMachine* pStateMachine,_float fTimeDelta)
{

	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	auto* playerStateMachine = Cast<CPlayer_StateMachine>(pStateMachine);
	if (!player || !playerStateMachine)
		return;

	auto* animator = player->GetAnimator();
	auto* motor = player->GetCharacterMotor();
	if (!animator || !motor)
	{
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		return;
	}

	const _bool bGrounded = motor->IsGrounded();
	if (!bGrounded && player->HasRawMoveInput())
	{
		auto* moveIntent = player->GetMoveIntent();
		if (moveIntent)
		{
			moveIntent->SetFacingIntent(
				player->GetRawMoveDirection(),
				360.f);
		}
	}
	const _float fVerticalSpeed = motor->GetVelocity().y;
	if (!bGrounded || fVerticalSpeed > 0.f)
		m_bWasAirborne = true;

	
	const _float Ratio =
		PlayerAnimationRatioGuard::Sanitize(
			animator->GetPlayAnimRatio());

	switch (m_ePhase)
	{
	case PHASE::START:
		if (m_bWasAirborne && bGrounded && fVerticalSpeed <= 0.f)
		{
			PlayLand(*player);
		}
		else if ((m_bWasAirborne && fVerticalSpeed <= m_fFallStartVerticalSpeed) || Ratio >= m_fJumpStartEnd)
		{
			PlayFall(*player);
		}
		break;

	case PHASE::FALL:
		if (m_bWasAirborne && bGrounded && fVerticalSpeed <= 0.f)
			PlayLand(*player);
		break;

	case PHASE::LAND:
		if (player->HasRawMoveInput())
		{
			auto* moveIntent = player->GetMoveIntent();
			if (moveIntent)
			{
				const _float fTurnSpeed =
					player->IsSprintRequested() ? 240.f : 360.f;
				moveIntent->SetFacingIntent(
					player->GetRawMoveDirection(),
					fTurnSpeed);
			}
		}

		// 이동 입력이 있으면 착지의 체중 이동이 끝난 뒤 Locomotion으로 자연스럽게 잇는다.
		if ((player->HasRawMoveInput() && Ratio >= m_fMovingLandExitRatio) ||
			animator->GetFinish())
			playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		break;
	}
}

int32_t CPlayer_Jump_State::FindAnimationIndex(const CPlayer& player,_string_view sAnimationName) const
{
	auto* modelInstance = player.GetModelInstance();
	if (!modelInstance || !modelInstance->GetModel())
		return -1;

	const auto& animations = modelInstance->GetModel()->GetAnimations();
	for (size_t i = 0; i < animations.size(); ++i)
	{
		if (animations[i] && animations[i]->GetAnimName() == sAnimationName)
			return static_cast<int32_t>(i);
	}

	return -1;
}

void CPlayer_Jump_State::PlayFall(CPlayer& player)
{
	m_ePhase = PHASE::FALL;

	auto* animator = player.GetAnimator();
	if (animator && m_iFallAnimation >= 0)
		animator->Play_Anim(m_iFallAnimation, true, 0.12f);
}

void CPlayer_Jump_State::PlayLand(CPlayer& player)
{
	m_ePhase = PHASE::LAND;
	if (auto* pSound = player.GetSound())
	{
		static constexpr const char* LAND_SOUNDS[] =
		{
			"./Resources/SampleClient/Sound/Player/Movement/Jump/Player_Land_01.wav",
			"./Resources/SampleClient/Sound/Player/Movement/Jump/Player_Land_02.wav",
			"./Resources/SampleClient/Sound/Player/Movement/Jump/Player_Land_03.wav",
			"./Resources/SampleClient/Sound/Player/Movement/Jump/Player_Land_Body.wav",
		};
		const int iSoundIndex = Engine::RandInt(
			0, static_cast<int>(std::size(LAND_SOUNDS)) - 1);
		pSound->PlaySlot2D(
			E::StringID{ "PLAYER_LAND" },
			LAND_SOUNDS[iSoundIndex],
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::SFX,
				.fVolume = 0.18f,
				.fPitch = 1.f,
				.iPriority = 72,
				.bLoop = false
			},
			SOUND_SLOT_PLAY_MODE::OVERLAP);
	}

	auto* animator = player.GetAnimator();

	

	if (animator)
	{
		const _float fMoveSpeed = player.GetCurrentMoveSpeed();
		JUMP_STATE eJumpState = JUMP_STATE::IDLE;
		if (fMoveSpeed > 10.f)
			eJumpState = JUMP_STATE::SPRINT;
		else if (fMoveSpeed > std::numeric_limits<_float>::epsilon())
			eJumpState = JUMP_STATE::JOG;

		int32_t iLandAnimation =
			m_LandAnimations[static_cast<size_t>(eJumpState)];
		if (iLandAnimation < 0)
			iLandAnimation = m_LandAnimations[static_cast<size_t>(JUMP_STATE::IDLE)];

		if (iLandAnimation >= 0)
			animator->Play_Anim(iLandAnimation, false, 0.08f);
	}
}

SPtr<CPlayer_Jump_State> CPlayer_Jump_State::Create()
{
	return ToSPtr(new CPlayer_Jump_State{});
}
