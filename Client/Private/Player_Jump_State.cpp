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
	if (!animator || !moveIntent || !player->GetCharacterMotor())
	{
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		return;
	}

	m_iStartAnimation = FindAnimationIndex(
		*player,
		"AN_ProfessorSharp_MasterRig_Hu_BM_JumpStart_anm.bin");
	m_iFallAnimation = FindAnimationIndex(
		*player,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Jump_Fall_anm.bin");
	m_iLandAnimation = FindAnimationIndex(
		*player,
		"AN_ProfessorSharp_MasterRig_Hu_BM_Land_Soft_v2_anm.bin");

	if (m_iStartAnimation < 0 ||
		m_iFallAnimation < 0 ||
		m_iLandAnimation < 0)
	{
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		return;
	}

	m_ePhase = PHASE::START;
	m_bWasAirborne = !player->GetCharacterMotor()->IsGrounded();
	player->SetMovementLocked(false);
	player->SetRootMotionRotationActive(false);
	player->SetRootMotionTranslationActive(false);

	if (m_bWasAirborne)
	{
		// Preserve the current locomotion pose for a short drop. Fall starts
		// only after downward velocity reaches the configured threshold.
	}
	else
	{
		moveIntent->RequestJump();
		animator->Play_Anim(m_iStartAnimation, false, 0.08f);
	}
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

void CPlayer_Jump_State::Update(
	CStateMachine* pStateMachine,
	_float fTimeDelta)
{
	(void)fTimeDelta;

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

	switch (m_ePhase)
	{
	case PHASE::START:
		if (m_bWasAirborne && bGrounded)
		{
			playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		}
		else if (m_bWasAirborne &&
			fVerticalSpeed <= m_fFallStartVerticalSpeed)
		{
			PlayFall(*player);
		}
		break;

	case PHASE::FALL:
		if (m_bWasAirborne && bGrounded && fVerticalSpeed <= 0.f)
			PlayLand(*player);
		break;

	case PHASE::LAND:
		if (animator->GetFinish())
			playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		break;
	}
}

int32_t CPlayer_Jump_State::FindAnimationIndex(
	const CPlayer& player,
	_string_view sAnimationName) const
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
	auto* animator = player.GetAnimator();
	if (!animator || m_iFallAnimation < 0)
		return;

	m_ePhase = PHASE::FALL;
	animator->Play_Anim(m_iFallAnimation, true, 0.1f);
}

void CPlayer_Jump_State::PlayLand(CPlayer& player)
{
	auto* animator = player.GetAnimator();
	if (!animator || m_iLandAnimation < 0)
		return;

	m_ePhase = PHASE::LAND;
	animator->Play_Anim(m_iLandAnimation, false, 0.08f);
}

SPtr<CPlayer_Jump_State> CPlayer_Jump_State::Create()
{
	return ToSPtr(new CPlayer_Jump_State{});
}
