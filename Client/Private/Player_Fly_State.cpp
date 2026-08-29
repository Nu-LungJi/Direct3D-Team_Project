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
	ResetRadialBlur();

	auto* pPlayer = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!pPlayer)
	{
		return;
	}

	CacheAnimationIndices(*pPlayer);
	m_iActiveAnimation = -1;
	m_eFlightPhase = FLIGHT_PHASE::MOUNTING;
	m_fLiftElapsed = 0.f;
	m_fAppliedLiftHeight = m_fMountInitialHeight;
	m_fCurrentFlightSpeed = 0.f;
	m_bBoosting = false;
	pPlayer->SetBroomBoostEffectRatio(0.f);
	m_vFlightDirection = {};
	// 이동 인텐트를 지우기 전에 탑승을 요청한 순간의 이동 상태를 보존한다.
	// 이후 리프트 단계가 끝나도 달리며 탔는지 여부가 바뀌지 않는다.
	m_bMountFromMovement = pPlayer->HasRawMoveInput();
	m_vMountGlideDirection = {};
	m_fMountGlideSpeed = 0.f;
	if (m_bMountFromMovement)
	{
		_vector vMountDirection = XMLoadFloat3(&pPlayer->GetRawMoveDirection());
		vMountDirection = XMVectorSetY(vMountDirection, 0.f);
		if (XMVectorGetX(XMVector3LengthSq(vMountDirection)) > FLT_EPSILON)
		{
			XMStoreFloat3(&m_vMountGlideDirection, XMVector3Normalize(vMountDirection));
			m_fMountGlideSpeed = pPlayer->GetCurrentMoveSpeed();
		}
	}
	pPlayer->SetMovementLocked(true);
	// 탑승 애니메이션의 원본 Root Motion 높이를 적용하면 현재 지면 아래로 내려간다.
	// 따라서 비행 상태가 직접 탑승 높이와 이동을 관리한다.
	pPlayer->SetRootMotionTranslationActive(false);
	pPlayer->SetRootMotionRotationActive(false);
	if (auto* pMoveIntent = pPlayer->GetMoveIntent())
	{
		pMoveIntent->ClearMoveIntent();
		pMoveIntent->AddExternalDisplacement({ 0.f, m_fMountInitialHeight, 0.f });
	}
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
	pPlayer->SetBroomVisible(true);
	pPlayer->SetBroomMovementRatio(0.f);
	pPlayer->SetBroomBoostEffectRatio(0.f);
	const int32_t iMountAnimation =
		m_bMountFromMovement && m_iMountJogAnimation >= 0
		? m_iMountJogAnimation
		: m_iMountHoverAnimation;
	if (iMountAnimation >= 0)
	{
		pAnimator->Play_Anim(iMountAnimation, false, 0.12f);
		m_iActiveAnimation = iMountAnimation;
	}
}

void CPlayer_Fly_State::Exit(CStateMachine* pStateMachine)
{
	ResetRadialBlur();

	auto* pPlayer = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!pPlayer)
	{
		return;
	}

	pPlayer->SetMovementLocked(false);
	pPlayer->SetBroomVisible(false);
	pPlayer->SetBroomMovementRatio(0.f);
	pPlayer->SetBroomBoostEffectRatio(0.f);
	pPlayer->SetRootMotionTranslationActive(false);
	pPlayer->SetRootMotionRotationActive(false);
	if (auto* pMoveIntent = pPlayer->GetMoveIntent())
		pMoveIntent->ClearMoveIntent();
	// 지상 상태가 다시 중력과 이동을 담당하도록 원래 설정을 복구한다.
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
		ResetRadialBlur();
		return;
	}

	if (!pPlayer->IsFlyRequested())
	{
		ResetRadialBlur();

		// 하차 애니메이션 완료를 기다리지 않고 즉시 지상 이동 상태로 복귀한다.
		pPlayer->SetMovementLocked(false);
		pMoveIntent->ClearMoveIntent();
		pMoveIntent->ClearFacingIntent();
		if (auto* pMotor = pPlayer->GetCharacterMotor())
		{
			_float3 vVelocity = pMotor->GetVelocity();
			vVelocity.x = 0.f;
			vVelocity.y = -m_fDismountInitialFallSpeed;
			vVelocity.z = 0.f;
			pMotor->SetVelocity(vVelocity);
			pMotor->SetUseGravity(true);
		}
		if (auto* pPlayerStateMachine = Cast<CPlayer_StateMachine>(pStateMachine))
		{
			pPlayerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		}
		return;
	}

	// 카메라(마우스 시점)를 기준으로 3차원 비행 입력을 만든다.
	// W/S는 카메라가 보는 상하 각도까지 그대로 따라가며, Space/Ctrl은 수직 보조 입력이다.
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
	// 전방 벡터의 Y를 제거하지 않아, 마우스로 위/아래를 본 채 W를 누르면 상승/하강한다.
	// 좌우 벡터만 수평으로 고정해 A/D 입력이 고도까지 흔들지 않도록 한다.
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
		const _vector vTargetDirection = XMVector3Normalize(vFlightInput);

		// 출발할 때는 입력 방향을 즉시 잡고, 비행 중에는 새 마우스 방향을 부드럽게 따라간다.
		// 별도 Turn 상태 없이 이동 궤적 자체가 완만하게 휘는 빗자루 조향 방식이다.
		if (m_fCurrentFlightSpeed <= m_fHoverSpeedThreshold)
		{
			XMStoreFloat3(&m_vFlightDirection, vTargetDirection);
		}
		else
		{
			const _float fDirectionRatio = 1.f - std::exp(
				-m_fFlightDirectionResponse * std::max(fTimeDelta, 0.f));
			_vector vCurrentDirection = XMLoadFloat3(&m_vLastFlightDirection);
			vCurrentDirection = XMVector3Normalize(XMVectorLerp(
				vCurrentDirection, vTargetDirection, fDirectionRatio));
			XMStoreFloat3(&m_vFlightDirection, vCurrentDirection);
		}
		m_vLastFlightDirection = m_vFlightDirection;
	}
	else
	{
		m_vFlightDirection = {};
	}

	const _bool bMountControlEnabled =
		m_eFlightPhase == FLIGHT_PHASE::MOUNTING &&
		pAnimator->GetPlayAnimRatio() >= m_fMountControlEnableRatio;
	if (bMountControlEnabled)
	{
		pPlayer->SetMovementLocked(false);
	}

	const _bool bCanControlFlight =
		bMountControlEnabled ||
		m_eFlightPhase == FLIGHT_PHASE::HOVER ||
		m_eFlightPhase == FLIGHT_PHASE::INTO_FLY ||
		m_eFlightPhase == FLIGHT_PHASE::FLYING ||
		m_eFlightPhase == FLIGHT_PHASE::INTO_HOVER;
	if (bCanControlFlight)
	{
		// 입력 유무와 부스트 키에 따라 목표 속도로 부드럽게 가감속한다.

		m_bBoosting =
			CGameInstance::Get().KeyPressing(DIK_LSHIFT);


		if (m_bBoosting) {
			m_fBoostTimeAcc += fTimeDelta;
			if (CGameInstance::Get().Get_RadialBlurIntensity() < m_fBloomLimit) {
				m_fCurBlurIntensity += fTimeDelta;
			}
			else {
				m_fCurBlurIntensity = 5.0f;
			}
		}
		else {
			if (m_fCurBlurIntensity > 0.0f) {
				m_fCurBlurIntensity -= fTimeDelta *3;
			}
			else {
				m_fCurBlurIntensity = 0.0f;
			}
		}
		CGameInstance::Get().Set_RadialBlurIntensity(m_fCurBlurIntensity);

		const _float fTargetSpeed = bHasFlightInput
			? (m_bBoosting ? m_fBoostFlightSpeed : m_fCruiseFlightSpeed)
			: 0.f;
		const _float fAcceleration = m_bBoosting
			? m_fBoostFlightAcceleration
			: m_fFlightAcceleration;
		const _float fSpeedStep =
			(bHasFlightInput ? fAcceleration : m_fFlightDeceleration) *
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
			// 별도 Turn 애니메이션은 사용하지 않고 모델의 수평 방향만 이동 궤적을 따라간다.
			// 상승/하강 기울기는 기존 Up/Down 비행 애니메이션이 표현한다.
			_float3 vFacingDirection = bHasFlightInput ? m_vFlightDirection : m_vLastFlightDirection;
			vFacingDirection.y = 0.f;
			if (vFacingDirection.x * vFacingDirection.x +
				vFacingDirection.z * vFacingDirection.z > FLT_EPSILON)
			{
				pMoveIntent->SetFacingIntent(vFacingDirection, m_fFacingTurnSpeed);
			}
		}
		else
		{
			m_fCurrentFlightSpeed = 0.f;
			pMoveIntent->ClearMoveIntent();
			pMoveIntent->ClearFacingIntent();
		}
	}
	// 부스트 사운드 
	if (m_bBoosting && m_fBoostTimeAcc >= 1.f && !m_bBoostSoundPlayed) {
		m_iBroomLoopSoundID = E::CGameInstance::Get().GetSoundManager()->Play2D("./Resources/SampleClient/Sound/Player/Movement/BroomFly.wav", SOUND_PLAY_DESC{
		.sBusID = SOUND_BUS::SFX,
		.fVolume = 1.f,
		.fPitch = 1.f,
		.iPriority = 64,
		.bLoop = true
			});
		m_bBoostSoundPlayed = true;
	}
	if (CGameInstance::Get().KeyUp(DIK_LSHIFT)) {
		if (m_iBroomLoopSoundID != INVALID_SOUND_ID)
		{
			CGameInstance::Get().GetSoundManager()->FadeOutAndStop(m_iBroomLoopSoundID, 1.0f);
			m_iBroomLoopSoundID = INVALID_SOUND_ID;
		}
		m_bBoostSoundPlayed = false;
		m_fBoostTimeAcc = 0.f;
	}



	// 호버 전환은 완전히 멈춘 뒤가 아니라 입력을 놓는 순간 시작한다.
	// 전환 애니메이션 동안 실제 속도는 위 감속 로직으로 자연스럽게 0에 수렴한다.
	// 빗자루 높이는 일반 비행 중에는 유지하고 실제 최고 속도에 도달했을 때만 올린다.
	const _float fMaximumSpeedThreshold = m_fBoostFlightSpeed - 0.01f;
	pPlayer->SetBroomMovementRatio(
		m_fCurrentFlightSpeed >= fMaximumSpeedThreshold ? 1.f : 0.f);
	const _float fBoostSpeedRatio = std::clamp(
		(m_fCurrentFlightSpeed - m_fCruiseFlightSpeed) /
		std::max(m_fBoostFlightSpeed - m_fCruiseFlightSpeed, FLT_EPSILON),
		0.f, 1.f);
	pPlayer->SetBroomBoostEffectRatio(
		m_bBoosting && bHasFlightInput
		? std::lerp(0.3f, 1.f, fBoostSpeedRatio)
		: 0.f);

	// 시작/정지 임계값을 분리해 경계 속도에서 전환 애니메이션이 반복되는 것을 막는다.
	const _bool bAlreadyFlying =
		m_eFlightPhase == FLIGHT_PHASE::INTO_FLY ||
		m_eFlightPhase == FLIGHT_PHASE::FLYING;
	const _bool bWantsToFly = bAlreadyFlying
		? m_fCurrentFlightSpeed > m_fFlyExitSpeedThreshold
		: m_fCurrentFlightSpeed >= m_fFlyEnterSpeedThreshold;

	switch (m_eFlightPhase)
	{
	case FLIGHT_PHASE::LIFTING:
	{
		// 최초 진입 시 캐릭터를 지면에서 탑승 위치까지 부드럽게 띄운다.
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
		// SmoothStep으로 시작과 끝의 급격한 높이 변화를 막는다.
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
			m_bMountFromMovement && m_iMountJogAnimation >= 0
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
			pAnimator->Play_Anim(m_iHoverAnimation, true, 0.3f);
			m_iActiveAnimation = m_iHoverAnimation;
		}
		return;
	}

	case FLIGHT_PHASE::MOUNTING:
	{
		const _float fMountRatio = std::clamp(pAnimator->GetPlayAnimRatio(), 0.f, 1.f);
		const _float fSmoothedRatio =
			fMountRatio * fMountRatio * (3.f - 2.f * fMountRatio);
		const _float fDesiredLiftHeight = m_fMountInitialHeight +
			(m_fMountLiftHeight - m_fMountInitialHeight) * fSmoothedRatio;
		const _float fLiftDelta = fDesiredLiftHeight - m_fAppliedLiftHeight;
		if (std::abs(fLiftDelta) > std::numeric_limits<_float>::epsilon())
		{
			pMoveIntent->AddExternalDisplacement({ 0.f, fLiftDelta, 0.f });
			m_fAppliedLiftHeight = fDesiredLiftHeight;
		}
		if (m_bMountFromMovement && m_fMountGlideSpeed > FLT_EPSILON)
		{
			// 이동 중 탑승할 때만 기존 진행 방향으로 미끄러지며 서서히 감속한다.
			const _float fGlideSpeedRatio = std::lerp(
				1.f, m_fMountGlideMinSpeedRatio, fSmoothedRatio);
			const _float fGlideDistance =
				m_fMountGlideSpeed * fGlideSpeedRatio * std::max(fTimeDelta, 0.f);
			pMoveIntent->AddExternalDisplacement({
				m_vMountGlideDirection.x * fGlideDistance,
				0.f,
				m_vMountGlideDirection.z * fGlideDistance });
		}
		// 탑승 동작이 끝날 때까지 조작을 잠그고 이후 호버 상태로 넘어간다.
		if (!pAnimator->GetFinish()) 
		{
			return;
		}
		pPlayer->SetMovementLocked(false);
		m_eFlightPhase = FLIGHT_PHASE::HOVER;
		if (m_iHoverAnimation >= 0)
		{
			pAnimator->Play_Anim(m_iHoverAnimation, true, 0.3f);
			m_iActiveAnimation = m_iHoverAnimation;
		}
		return;
	}

	case FLIGHT_PHASE::HOVER:
		// 정지 상태에서 이동 입력이 생기면 비행 시작 전환 애니메이션을 재생한다.
		if (!bWantsToFly) 
		{
			break;
		}
		m_eFlightPhase = FLIGHT_PHASE::INTO_FLY;
		if (m_iIntoFlyAnimation >= 0)
		{
			pAnimator->Play_Anim(m_iIntoFlyAnimation, false, 0.22f);
			m_iActiveAnimation = m_iIntoFlyAnimation;
			return;
		}
		m_eFlightPhase = FLIGHT_PHASE::FLYING;
		break;

	case FLIGHT_PHASE::INTO_FLY:
		// 비행 시작 전환이 끝난 뒤 반복 비행 애니메이션을 선택한다.
		if (!pAnimator->GetFinish()) 
		{
			return;
		}
		m_eFlightPhase = FLIGHT_PHASE::FLYING;
		break;

	case FLIGHT_PHASE::FLYING:
		// 정지할 때 IntoHover를 한 번 더 거치면 호버 자세가 이중으로 바뀌므로
		// 현재 비행 자세에서 Hover_Idle로 직접 블렌딩한다.
		if (bWantsToFly) 
		{
			break;
		}
		m_eFlightPhase = FLIGHT_PHASE::HOVER;
		break;

	case FLIGHT_PHASE::INTO_HOVER:
		// 정지 전환 중 다시 입력하면 호버까지 기다리지 않고 즉시 비행으로 복귀한다.
		if (bWantsToFly)
		{
			m_eFlightPhase = FLIGHT_PHASE::INTO_FLY;
			if (m_iIntoFlyAnimation >= 0)
			{
				pAnimator->Play_Anim(m_iIntoFlyAnimation, false, 0.22f);
				m_iActiveAnimation = m_iIntoFlyAnimation;
				return;
			}
			m_eFlightPhase = FLIGHT_PHASE::FLYING;
			break;
		}
		if (!pAnimator->GetFinish()) 
		{
			return;
		}
		m_eFlightPhase = FLIGHT_PHASE::HOVER;
		break;

	case FLIGHT_PHASE::DISMOUNTING:
	{
		// 하차가 끝나면 중력을 복구하고 지상 이동 상태로 반환한다.
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
	}

	// Turn 전용 좌우 애니메이션 없이 속도와 수직 방향만으로 비행 자세를 고른다.
	const int32_t iDesiredAnimation = m_eFlightPhase == FLIGHT_PHASE::HOVER
		? m_iHoverAnimation
		: ResolveFlightAnimation();
	if (iDesiredAnimation < 0 || iDesiredAnimation == m_iActiveAnimation) 
	{
		return;
	}
	pAnimator->Play_Anim(iDesiredAnimation, true, 0.35f);
	m_iActiveAnimation = iDesiredAnimation;
}

void CPlayer_Fly_State::ResetRadialBlur()
{
	m_bBoosting = false;
	m_fCurBlurIntensity = 0.f;
	CGameInstance::Get().Set_RadialBlurIntensity(0.f);
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
	m_iSlowForwardAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_Flynostirrups_Hover_Fly_Fwd_anm.bin");
	m_iFastForwardAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Idle_Fwd_anm.bin");
	m_iTurboForwardAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Turbo_Fwd_anm.bin");
	m_bAnimationIndicesCached = true;
}

int32_t CPlayer_Fly_State::ResolveFlightAnimation() const
{
	const _float fSpeed = m_fCurrentFlightSpeed;
	if (fSpeed <= m_fHoverSpeedThreshold)
	{
		return m_iHoverAnimation;
	}

	// Shift 입력 순간이 아니라 실제 속도가 터보 구간에 들어왔을 때만 터보 자세를 사용한다.
	// Shift를 놓은 뒤에도 속도가 임계값 아래로 내려올 때까지 자세를 유지해 전환이 튀지 않는다.
	const _bool bTurboSpeed =
		fSpeed >= m_fTurboAnimationSpeedThreshold;
	// Space/Ctrl은 비행 궤적의 높이만 바꾼다.
	// Up/Down 전용 클립은 상체가 수직으로 서는 자세이므로 재생하지 않고
	// 기존 전진 비행 자세를 그대로 유지한다.

	// 좌우 전용 클립은 몸 방향이 크게 틀어지므로 사용하지 않는다.
	// 터보 중 A/D를 눌러도 Turbo_Fwd 자세를 유지하고 이동 궤적만 바뀐다.
	if (bTurboSpeed && m_iTurboForwardAnimation >= 0)
		return m_iTurboForwardAnimation;
	// Idle_Fwd는 뒷짐 자세가 섞이므로 제외하고 일반 전진은 Hover_Fly_Fwd만 사용한다.
	return m_iSlowForwardAnimation;
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
