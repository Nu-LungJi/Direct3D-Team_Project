#include "pch.h"
#include "Player_Knockdown_State.h"

#include "Player.h"
#include "Player_StateMachine.h"
#include "ComAnimator.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
#include "ComModelInstance.h"
#include "ResModel.h"
#include "ResModelAnim.h"

NS_USING(Client)

void CPlayer_Knockdown_State::Enter(CStateMachine* pStateMachine)
{
	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	auto* playerStateMachine = Cast<CPlayer_StateMachine>(pStateMachine);
	if (!player || !playerStateMachine)
		return;

	CacheAnimationIndices(*player);
	m_eDirection = ResolveDirection(*player);
	m_fSequenceTime = 0.f;
	m_fLandingSlideTime = 0.f;
	m_vLandingSlideVelocity = {};
	m_bWasAirborne = false;
	m_bLandingSliding = false;
	player->SetCurrentMoveSpeed(0.f);
	player->SetMovementLocked(true);
	player->SetInvincible(true);
	player->SetRootMotionTranslationScale(AIRBORNE_ROOT_MOTION_SCALE);
	player->SetRootMotionTranslationActive(true);
	player->SetRootMotionRotationActive(false);
	if (auto* moveIntent = player->GetMoveIntent())
	{
		moveIntent->ClearMoveIntent();
		moveIntent->ClearFacingIntent();
	}

	auto* motor = player->GetCharacterMotor();
	if (!motor || !PlaySequence(*player, SEQUENCE::AIRBORNE))
	{
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		return;
	}

	m_fPreviousGravity = motor->GetGravity();
	m_bPreviousUseGravity = motor->IsUsingGravity();
	m_bPreviousPreserveHorizontalVelocity = motor->IsPreservingHorizontalVelocity();
	m_bMotorSettingsCaptured = true;
	motor->SetUseGravity(true);
	motor->SetGravity(KNOCKDOWN_GRAVITY);
	motor->SetPreserveHorizontalVelocity(true);

	_vector vLaunchDirection = player->GetTransform().GetState(STATE::POSITION) -
		XMLoadFloat3(&player->GetKnockdownAttackPosition());
	vLaunchDirection = XMVectorSetY(vLaunchDirection, 0.f);
	if (XMVectorGetX(XMVector3LengthSq(vLaunchDirection)) <= FLT_EPSILON)
		vLaunchDirection = -XMVectorSetY(player->GetTransform().GetState(STATE::LOOK), 0.f);
	if (XMVectorGetX(XMVector3LengthSq(vLaunchDirection)) <= FLT_EPSILON)
		vLaunchDirection = XMVectorSet(0.f, 0.f, -1.f, 0.f);

	vLaunchDirection = XMVector3Normalize(vLaunchDirection) * LAUNCH_HORIZONTAL_SPEED;
	_float3 vLaunchVelocity{};
	XMStoreFloat3(&vLaunchVelocity, vLaunchDirection);
	vLaunchVelocity.y = LAUNCH_VERTICAL_SPEED;
	motor->SetVelocity(vLaunchVelocity);
}

void CPlayer_Knockdown_State::Exit(CStateMachine* pStateMachine)
{
	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!player)
		return;

	player->SetMovementLocked(false);
	player->SetRootMotionTranslationActive(false);
	player->SetRootMotionTranslationScale(1.f);
	player->SetRootMotionRotationActive(false);
	player->SetInvincible(false);
	if (auto* motor = player->GetCharacterMotor(); motor && m_bMotorSettingsCaptured)
	{
		motor->SetGravity(m_fPreviousGravity);
		motor->SetUseGravity(m_bPreviousUseGravity);
		motor->SetPreserveHorizontalVelocity(m_bPreviousPreserveHorizontalVelocity);
	}
	m_eSequence = SEQUENCE::END;
	m_fSequenceTime = 0.f;
	m_fLandingSlideTime = 0.f;
	m_vLandingSlideVelocity = {};
	m_bWasAirborne = false;
	m_bLandingSliding = false;
	m_bMotorSettingsCaptured = false;
}

void CPlayer_Knockdown_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
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

	m_fSequenceTime += fTimeDelta;
	if (m_eSequence == SEQUENCE::AIRBORNE)
	{
		const _bool bGrounded = motor->IsGrounded();
		const _float fVerticalSpeed = motor->GetVelocity().y;
		if (!bGrounded || fVerticalSpeed > 0.f)
			m_bWasAirborne = true;

		if (m_bWasAirborne && bGrounded && fVerticalSpeed <= 0.f)
		{
			m_bLandingSliding = true;
			m_fLandingSlideTime = 0.f;
			m_vLandingSlideVelocity = motor->GetVelocity();
			m_vLandingSlideVelocity.y = 0.f;
			if (!PlaySequence(*player, SEQUENCE::SPLAT_HOLD))
				playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		}
		return;
	}

	if (m_eSequence == SEQUENCE::SPLAT_HOLD)
	{
		if (m_bLandingSliding)
		{
			m_fLandingSlideTime += fTimeDelta;
			const _float fRatio = std::clamp(
				m_fLandingSlideTime / LANDING_SLIDE_DURATION, 0.f, 1.f);
			const _float fRemain = 1.f - fRatio;
			const _float fSpeedScale = fRemain * fRemain;

			_float3 vVelocity = motor->GetVelocity();
			vVelocity.x = m_vLandingSlideVelocity.x * fSpeedScale;
			vVelocity.z = m_vLandingSlideVelocity.z * fSpeedScale;
			motor->SetVelocity(vVelocity);

			if (fRatio >= 1.f)
			{
				m_bLandingSliding = false;
				m_fSequenceTime = 0.f;
				motor->SetPreserveHorizontalVelocity(false);
			}
			return;
		}

		if (m_fSequenceTime >= DOWN_HOLD_DURATION &&
			!PlaySequence(*player, SEQUENCE::GETUP))
		{
			playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		}
		return;
	}

	if (!animator->GetFinish())
		return;

	playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
}

void CPlayer_Knockdown_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	static constexpr std::array<const char*, DIRECTION_COUNT> DIRECTION_NAMES{
		"Fwd", "Bwd", "Lft", "Rht"
	};
	static constexpr std::array<const char*, SEQUENCE_COUNT> SEQUENCE_NAMES{
		"9m_Start", "SplatHold", "Getup"
	};

	for (size_t iDirection = 0; iDirection < DIRECTION_COUNT; ++iDirection)
	{
		m_AnimationSets[iDirection].iAnimations.fill(-1);
		for (size_t iSequence = 0; iSequence < SEQUENCE_COUNT; ++iSequence)
		{
			const _string sAnimationName =
				"AN_ProfessorSharp_MasterRig_Hu_Rct_Knockdown_" +
				_string{ DIRECTION_NAMES[iDirection] } + "_" +
				SEQUENCE_NAMES[iSequence] + "_anm.bin";
			m_AnimationSets[iDirection].iAnimations[iSequence] =
				FindAnimationIndex(player, sAnimationName);
		}
	}
	m_bAnimationIndicesCached = true;
}

CPlayer_Knockdown_State::DIRECTION CPlayer_Knockdown_State::ResolveDirection(const CPlayer& player) const
{
	_vector vPlayerLook = XMVectorSetY(player.GetTransform().GetState(STATE::LOOK), 0.f);
	_vector vPlayerRight = XMVectorSetY(player.GetTransform().GetState(STATE::RIGHT), 0.f);
	_vector vToAttacker = XMVectorSetY(
		XMLoadFloat3(&player.GetKnockdownAttackPosition()) -
		player.GetTransform().GetState(STATE::POSITION), 0.f);

	constexpr _float EPSILON = std::numeric_limits<_float>::epsilon();
	if (XMVectorGetX(XMVector3LengthSq(vPlayerLook)) <= EPSILON ||
		XMVectorGetX(XMVector3LengthSq(vPlayerRight)) <= EPSILON ||
		XMVectorGetX(XMVector3LengthSq(vToAttacker)) <= EPSILON)
		return DIRECTION::BWD;

	vPlayerLook = XMVector3Normalize(vPlayerLook);
	vPlayerRight = XMVector3Normalize(vPlayerRight);
	vToAttacker = XMVector3Normalize(vToAttacker);
	const _float fForward = XMVectorGetX(XMVector3Dot(vPlayerLook, vToAttacker));
	const _float fRight = XMVectorGetX(XMVector3Dot(vPlayerRight, vToAttacker));
	if (std::abs(fForward) >= std::abs(fRight))
		return fForward >= 0.f ? DIRECTION::BWD : DIRECTION::FWD;
	return fRight >= 0.f ? DIRECTION::LFT : DIRECTION::RHT;
}

_bool CPlayer_Knockdown_State::PlaySequence(CPlayer& player, SEQUENCE eSequence)
{
	const size_t iDirection = static_cast<size_t>(m_eDirection);
	const size_t iSequence = static_cast<size_t>(eSequence);
	if (iDirection >= DIRECTION_COUNT || iSequence >= SEQUENCE_COUNT)
		return false;

	const int32_t iAnimation = m_AnimationSets[iDirection].iAnimations[iSequence];
	if (iAnimation < 0 || !player.GetAnimator())
		return false;

	const _bool bUseScaledRootMotion = eSequence == SEQUENCE::AIRBORNE;
	player.SetRootMotionTranslationScale(
		bUseScaledRootMotion ? AIRBORNE_ROOT_MOTION_SCALE : 1.f);
	player.SetRootMotionTranslationActive(bUseScaledRootMotion);
	player.SetRootMotionRotationActive(false);
	const _float fBlendDuration = eSequence == SEQUENCE::SPLAT_HOLD
		? LANDING_BLEND_DURATION
		: BLEND_DURATION;
	player.GetAnimator()->Play_Anim(
		iAnimation,
		eSequence == SEQUENCE::SPLAT_HOLD,
		fBlendDuration);
	m_eSequence = eSequence;
	m_fSequenceTime = 0.f;
	return true;
}

int32_t CPlayer_Knockdown_State::FindAnimationIndex(const CPlayer& player, _string_view sAnimationName) const
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

SPtr<CPlayer_Knockdown_State> CPlayer_Knockdown_State::Create()
{
	return ToSPtr(new CPlayer_Knockdown_State{});
}
