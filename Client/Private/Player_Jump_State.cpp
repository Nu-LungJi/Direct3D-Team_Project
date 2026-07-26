#include "pch.h"
#include "Player_Jump_State.h"

#include "Player.h"
#include "Player_StateMachine.h"
#include "ComAnimator.h"
#include "ComCharacterMotor.h"
#include "ComCharacterMoveIntent.h"
#include "ComModelInstance.h"
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
	if (m_StartAnimations[(size_t)JUMP_STATE::IDLE] >= 0)
	{

		const _float Move = player->GetCurrentMoveSpeed();

		if (Move >= 7.5f && Move <= 10.f) {

			animator->Play_Anim(m_StartAnimations[(size_t)JUMP_STATE::JOG], false, 0.08f);
		}
		else if (Move > 10.f) {

			animator->Play_Anim(m_StartAnimations[(size_t)JUMP_STATE::SPRINT], false, 0.08f);
		}
		else {
			animator->Play_Anim(m_StartAnimations[(size_t)JUMP_STATE::IDLE], false, 0.08f);

		}
	}
	
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
	const _float fVerticalSpeed = motor->GetVelocity().y;
	if (!bGrounded || fVerticalSpeed > 0.f)
		m_bWasAirborne = true;

	
	const _float Ratio = animator->GetPlayAnimRatio();

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

		if (animator->GetFinish())
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
		animator->Play_Anim(m_iFallAnimation, true, 0.6f);
}

void CPlayer_Jump_State::PlayLand(CPlayer& player)
{
	m_ePhase = PHASE::LAND;

	auto* animator = player.GetAnimator();

	

	if (animator)
	{
		//Jog 목표 속도 : 7.5f
		//Sprint 목표 속도 : 15.f
		
		const _float Move = player.GetCurrentMoveSpeed();

		if (Move > std::numeric_limits<_float>::epsilon() &&  Move<=10.f) {
			animator->Play_Anim(m_LandAnimations[(size_t)JUMP_STATE::JOG], false, 0.08f);

		}
		else if (Move > 10.f) {

			animator->Play_Anim(m_LandAnimations[(size_t)JUMP_STATE::SPRINT], false, 0.08f);
		}
		else if(Move <= std::numeric_limits<_float>::epsilon()) {
			animator->Play_Anim(m_LandAnimations[(size_t)JUMP_STATE::IDLE], false, 0.08f);

		}
	}
}

SPtr<CPlayer_Jump_State> CPlayer_Jump_State::Create()
{
	return ToSPtr(new CPlayer_Jump_State{});
}
