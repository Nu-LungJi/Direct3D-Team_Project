#include "pch.h"
#include "Player_Fly_State.h"

#include "Player.h"
#include "Player_StateMachine.h"
#include "ComAnimator.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
#include "ComModelInstance.h"
#include "ResModel.h"
#include "ResModelAnim.h"

NS_USING(Client)

CPlayer_Fly_State::CPlayer_Fly_State() = default;

void CPlayer_Fly_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!pPlayer)
	{
		return;
	}

	CacheAnimationIndices(*pPlayer);
	m_iActiveAnimation = -1;
	m_eFlightPhase = FLIGHT_PHASE::LIFTING;
	m_fLiftElapsed = 0.f;
	m_fAppliedLiftHeight = 0.f;
	m_fCurrentFlightSpeed = 0.f;
	m_vFlightDirection = {};
	pPlayer->SetMovementLocked(true);
	// Broom mount clips contain vertical root motion authored around their
	// original scene origin. Applying that delta to the CCT makes the player
	// animate below the current ground. Flight entry owns its world height.
	pPlayer->SetRootMotionTranslationActive(false);
	pPlayer->SetRootMotionRotationActive(false);
	if (auto* pMoveIntent = pPlayer->GetMoveIntent())
		pMoveIntent->ClearMoveIntent();
	if (auto* pMotor = pPlayer->GetCharacterMotor())
	{
		_float3 vVelocity = pMotor->GetVelocity();
		vVelocity.y = 0.f;
		pMotor->SetVelocity(vVelocity);
		pMotor->SetUseGravity(false);
	}

	auto* pAnimator = pPlayer->GetAnimator();
	if (!pAnimator)
	{
		return;
	}
}

void CPlayer_Fly_State::Exit(CStateMachine* pStateMachine)
{
	auto* pPlayer = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!pPlayer)
	{
		return;
	}

	pPlayer->SetMovementLocked(false);
	pPlayer->SetRootMotionTranslationActive(false);
	pPlayer->SetRootMotionRotationActive(false);
	if (auto* pMoveIntent = pPlayer->GetMoveIntent())
		pMoveIntent->ClearMoveIntent();
	if (auto* pMotor = pPlayer->GetCharacterMotor())
	{
		pMotor->SetUseGravity(true);
	}
}

void CPlayer_Fly_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{

	auto* pPlayer = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!pPlayer)
	{
		return;
	}

	auto* pAnimator = pPlayer->GetAnimator();
	auto* pMoveIntent = pPlayer->GetMoveIntent();
	if (!pAnimator || !pMoveIntent)
	{
		return;
	}

	if (!pPlayer->IsFlyRequested() &&
		m_eFlightPhase != FLIGHT_PHASE::LIFTING &&
		m_eFlightPhase != FLIGHT_PHASE::MOUNTING &&
		m_eFlightPhase != FLIGHT_PHASE::DISMOUNTING)
	{
		m_eFlightPhase = FLIGHT_PHASE::DISMOUNTING;
		pPlayer->SetMovementLocked(true);
		if (m_iDismountAnimation >= 0)
		{
			pAnimator->Play_Anim(m_iDismountAnimation, false, 0.15f);
			m_iActiveAnimation = m_iDismountAnimation;
		}
	}

	// Flight owns the final movement intent. Ground locomotion only supplies a
	// horizontal direction, so rebuild a camera-relative 3D input here.
	_float fForwardInput{};
	_float fRightInput{};
	_float fVerticalInput{};
	if (CGameInstance::Get().KeyPressing(DIK_W) ||
		CGameInstance::Get().KeyPressing(DIK_UP))
		fForwardInput += 1.f;
	if (CGameInstance::Get().KeyPressing(DIK_S) ||
		CGameInstance::Get().KeyPressing(DIK_DOWN))
		fForwardInput -= 1.f;
	if (CGameInstance::Get().KeyPressing(DIK_D) ||
		CGameInstance::Get().KeyPressing(DIK_RIGHT))
		fRightInput += 1.f;
	if (CGameInstance::Get().KeyPressing(DIK_A) ||
		CGameInstance::Get().KeyPressing(DIK_LEFT))
		fRightInput -= 1.f;
	if (CGameInstance::Get().KeyPressing(DIK_SPACE))
		fVerticalInput += 1.f;
	if (CGameInstance::Get().KeyPressing(DIK_LCONTROL))
		fVerticalInput -= 1.f;

	_vector vCameraForward = pPlayer->GetTransform().GetState(STATE::LOOK);
	_vector vCameraRight = pPlayer->GetTransform().GetState(STATE::RIGHT);
	if (auto* pCamera = CGameInstance::Get().GetActiveCamera("PlayerCamera"))
	{
		vCameraForward = pCamera->GetTransform().GetState(STATE::LOOK);
		vCameraRight = pCamera->GetTransform().GetState(STATE::RIGHT);
	}
	// Keep the camera pitch while flying: looking up/down and pressing W/S
	// must climb/dive instead of being forced onto the ground plane.
	vCameraRight = XMVectorSetY(vCameraRight, 0.f);
	if (XMVectorGetX(XMVector3LengthSq(vCameraForward)) > FLT_EPSILON)
		vCameraForward = XMVector3Normalize(vCameraForward);
	if (XMVectorGetX(XMVector3LengthSq(vCameraRight)) > FLT_EPSILON)
		vCameraRight = XMVector3Normalize(vCameraRight);

	_vector vFlightInput =
		vCameraForward * fForwardInput +
		vCameraRight * fRightInput +
		XMVectorSet(0.f, fVerticalInput, 0.f, 0.f);
	const _bool bHasFlightInput =
		XMVectorGetX(XMVector3LengthSq(vFlightInput)) > FLT_EPSILON;
	if (bHasFlightInput)
	{
		vFlightInput = XMVector3Normalize(vFlightInput);
		XMStoreFloat3(&m_vFlightDirection, vFlightInput);
		m_vLastFlightDirection = m_vFlightDirection;
	}
	else
	{
		m_vFlightDirection = {};
	}

	const _bool bCanControlFlight =
		m_eFlightPhase == FLIGHT_PHASE::HOVER ||
		m_eFlightPhase == FLIGHT_PHASE::INTO_FLY ||
		m_eFlightPhase == FLIGHT_PHASE::FLYING ||
		m_eFlightPhase == FLIGHT_PHASE::INTO_HOVER;
	if (bCanControlFlight)
	{
		const _bool bBoost =
			CGameInstance::Get().KeyPressing(DIK_LSHIFT);
		const _float fTargetSpeed = bHasFlightInput
			? (bBoost ? m_fBoostFlightSpeed : m_fCruiseFlightSpeed)
			: 0.f;
		const _float fSpeedStep =
			(bHasFlightInput ? m_fFlightAcceleration : m_fFlightDeceleration) *
			std::max(fTimeDelta, 0.f);
		if (m_fCurrentFlightSpeed < fTargetSpeed)
			m_fCurrentFlightSpeed = std::min(
				m_fCurrentFlightSpeed + fSpeedStep, fTargetSpeed);
		else
			m_fCurrentFlightSpeed = std::max(
				m_fCurrentFlightSpeed - fSpeedStep, fTargetSpeed);

		if (m_fCurrentFlightSpeed > FLT_EPSILON)
		{
			pMoveIntent->SetMoveIntent(
				bHasFlightInput ? m_vFlightDirection : m_vLastFlightDirection,
				m_fCurrentFlightSpeed);
			_float3 vFacingDirection = bHasFlightInput
				? m_vFlightDirection
				: m_vLastFlightDirection;
			vFacingDirection.y = 0.f;
			if (vFacingDirection.x * vFacingDirection.x +
				vFacingDirection.z * vFacingDirection.z > FLT_EPSILON)
			{
				pMoveIntent->SetFacingIntent(
					vFacingDirection, m_fFacingTurnSpeed);
			}
		}
		else
		{
			m_fCurrentFlightSpeed = 0.f;
			pMoveIntent->ClearMoveIntent();
			pMoveIntent->ClearFacingIntent();
		}
	}
	const _bool bWantsToFly =
		bHasFlightInput || m_fCurrentFlightSpeed > m_fHoverSpeedThreshold;

	switch (m_eFlightPhase)
	{
	case FLIGHT_PHASE::LIFTING:
	{
		if (!pPlayer->IsFlyRequested())
		{
			if (auto* pMotor = pPlayer->GetCharacterMotor())
				pMotor->SetUseGravity(true);
			if (auto* pPlayerStateMachine =
				Cast<CPlayer_StateMachine>(pStateMachine))
			{
				pPlayerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
			}
			return;
		}

		m_fLiftElapsed += std::max(fTimeDelta, 0.f);
		const _float fLiftRatio = m_fMountLiftDuration > 0.f
			? std::clamp(m_fLiftElapsed / m_fMountLiftDuration, 0.f, 1.f)
			: 1.f;
		// Smoothly rise fast at first and settle onto the fixed mount height.
		const _float fSmoothedRatio =
			fLiftRatio * fLiftRatio * (3.f - 2.f * fLiftRatio);
		const _float fDesiredLiftHeight =
			m_fMountLiftHeight * fSmoothedRatio;
		const _float fLiftDelta =
			fDesiredLiftHeight - m_fAppliedLiftHeight;
		if (std::abs(fLiftDelta) > std::numeric_limits<_float>::epsilon())
		{
			pMoveIntent->AddExternalDisplacement({ 0.f, fLiftDelta, 0.f });
			m_fAppliedLiftHeight = fDesiredLiftHeight;
		}

		if (fLiftRatio < 1.f)
			return;

		m_eFlightPhase = FLIGHT_PHASE::MOUNTING;
		const int32_t iMountAnimation =
			pPlayer->HasRawMoveInput() && m_iMountJogAnimation >= 0
			? m_iMountJogAnimation
			: m_iMountHoverAnimation;
		if (iMountAnimation >= 0)
		{
			pAnimator->Play_Anim(iMountAnimation, false, 0.15f);
			m_iActiveAnimation = iMountAnimation;
			return;
		}

		pPlayer->SetMovementLocked(false);
		m_eFlightPhase = FLIGHT_PHASE::HOVER;
		if (m_iHoverAnimation >= 0)
		{
			pAnimator->Play_Anim(m_iHoverAnimation, true, 0.15f);
			m_iActiveAnimation = m_iHoverAnimation;
		}
		return;
	}

	case FLIGHT_PHASE::MOUNTING:
		if (!pAnimator->GetFinish()) 
		{
			return;
		}
		pPlayer->SetMovementLocked(false);
		m_eFlightPhase = FLIGHT_PHASE::HOVER;
		if (m_iHoverAnimation >= 0)
		{
			pAnimator->Play_Anim(m_iHoverAnimation, true, 0.15f); 
			m_iActiveAnimation = m_iHoverAnimation; 
		}
		return;

	case FLIGHT_PHASE::HOVER:
		if (!bWantsToFly) 
		{
			break;
		}
		m_eFlightPhase = FLIGHT_PHASE::INTO_FLY;
		if (m_iIntoFlyAnimation >= 0)
		{
			pAnimator->Play_Anim(m_iIntoFlyAnimation, false, 0.12f); 
			m_iActiveAnimation = m_iIntoFlyAnimation; 
			return; 
		}
		m_eFlightPhase = FLIGHT_PHASE::FLYING;
		break;

	case FLIGHT_PHASE::INTO_FLY:
		if (!pAnimator->GetFinish()) 
		{
			return;
		}
		m_eFlightPhase = FLIGHT_PHASE::FLYING;
		break;

	case FLIGHT_PHASE::FLYING:
		if (bWantsToFly) 
		{
			break;
		}
		m_eFlightPhase = FLIGHT_PHASE::INTO_HOVER;
		if (m_iIntoHoverAnimation >= 0)
		{
			pAnimator->Play_Anim(m_iIntoHoverAnimation, false, 0.12f); 
			m_iActiveAnimation = m_iIntoHoverAnimation; 
			return; 
		}
		m_eFlightPhase = FLIGHT_PHASE::HOVER;
		break;

	case FLIGHT_PHASE::INTO_HOVER:
		if (!pAnimator->GetFinish()) 
		{
			return;
		}
		m_eFlightPhase = FLIGHT_PHASE::HOVER;
		break;

	case FLIGHT_PHASE::DISMOUNTING:
		if (!pAnimator->GetFinish()) 
		{
			return;
		}
		if (auto* pMotor = pPlayer->GetCharacterMotor())
		{
			pMotor->SetUseGravity(true);
		}
		if (auto* pPlayerStateMachine = Cast<CPlayer_StateMachine>(pStateMachine)) 
		{
			pPlayerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		}
		return;
	}

	const int32_t iDesiredAnimation = m_eFlightPhase == FLIGHT_PHASE::HOVER ? m_iHoverAnimation : ResolveAnimation(*pPlayer);
	if (iDesiredAnimation < 0 || iDesiredAnimation == m_iActiveAnimation) 
	{
		return;
	}
	pAnimator->Play_Anim(iDesiredAnimation, true, 0.2f);
	m_iActiveAnimation = iDesiredAnimation;
}

void CPlayer_Fly_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached) 
	{
		return;
	}

	m_iHoverAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Hover_Idle_anm.bin");
	m_iMountHoverAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_Mount_Hover_anm.bin");
	m_iMountJogAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_Mount_Fly_fJog_anm.bin");
	m_iDismountAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_NoStirrups_Dismount_anm.bin");
	m_iIntoFlyAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Hover_IntoFly_anm.bin");
	m_iIntoHoverAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Fly_IntoHover_anm.bin");
	m_iForwardAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Idle_Fwd_anm.bin");
	m_iSlowUpAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Slow_Up_anm.bin");
	m_iSlowDownAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Slow_Dn_anm.bin");
	m_iSlowLeftAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Slow_Lft_anm.bin");
	m_iSlowRightAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Slow_Rht_anm.bin");
	m_iFastUpAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Fast_Up_anm.bin");
	m_iFastDownAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Fast_Dn_anm.bin");
	m_iFastLeftAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Fast_Lft_anm.bin");
	m_iFastRightAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Fast_Rht_anm.bin");
	m_bAnimationIndicesCached = true;
}

int32_t CPlayer_Fly_State::ResolveAnimation(const CPlayer& player) const
{
	const _float fSpeed = m_fCurrentFlightSpeed;
	if (fSpeed <= m_fHoverSpeedThreshold)
	{
		return m_iHoverAnimation;
	}

	const _float3& vInput =
		(m_vFlightDirection.x != 0.f ||
		 m_vFlightDirection.y != 0.f ||
		 m_vFlightDirection.z != 0.f)
		? m_vFlightDirection
		: m_vLastFlightDirection;
	const _bool bFast = fSpeed >= m_fFastSpeedThreshold;
	if (vInput.y >= m_fVerticalInputThreshold) 
	{
		return bFast ? m_iFastUpAnimation : m_iSlowUpAnimation;
	}
	if (vInput.y <= -m_fVerticalInputThreshold) 
	{
		return bFast ? m_iFastDownAnimation : m_iSlowDownAnimation;
	}

	const _float fAngle = CalculateSignedAngle(player, vInput);
	if (fAngle >= m_fTurnAngleThreshold) 
	{
		return bFast ? m_iFastRightAnimation : m_iSlowRightAnimation;
	}
	if (fAngle <= -m_fTurnAngleThreshold) 
	{
		return bFast ? m_iFastLeftAnimation : m_iSlowLeftAnimation;
	}
	return m_iForwardAnimation;
}

_float CPlayer_Fly_State::CalculateSignedAngle(const CPlayer& player, const _float3& vMoveDirection) const
{
	_vector vLook = XMVectorSetY(player.GetTransform().GetState(STATE::LOOK), 0.f);
	_vector vRight = XMVectorSetY(player.GetTransform().GetState(STATE::RIGHT), 0.f);
	_vector vMove = XMVectorSetY(XMLoadFloat3(&vMoveDirection), 0.f);
	if (XMVectorGetX(XMVector3LengthSq(vMove)) <= std::numeric_limits<_float>::epsilon())
	{
		return 0.f;
	}

	vLook = XMVector3Normalize(vLook);
	vRight = XMVector3Normalize(vRight);
	vMove = XMVector3Normalize(vMove);
	const _float fForward = XMVectorGetX(XMVector3Dot(vMove, vLook));
	const _float fRight = XMVectorGetX(XMVector3Dot(vMove, vRight));
	return XMConvertToDegrees(std::atan2(fRight, fForward));
}

int32_t CPlayer_Fly_State::FindAnimationIndex(const CPlayer& player, const _string_view& sAnimationName) const
{
	auto* pModelInstance = player.GetModelInstance();
	if (!pModelInstance || !pModelInstance->GetModel()) 
	{
		return -1;
	}

	const auto& animations = pModelInstance->GetModel()->GetAnimations();
	for (size_t i = 0; i < animations.size(); ++i)
	{
		if (animations[i] && animations[i]->GetAnimName() == sAnimationName)
		{
			return static_cast<int32_t>(i);
		}
	}
	return -1;
}

SPtr<CPlayer_Fly_State> CPlayer_Fly_State::Create() { 
	return ToSPtr(new CPlayer_Fly_State{}); 
}
