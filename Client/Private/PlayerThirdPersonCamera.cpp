#include "pch.h"
#include "PlayerThirdPersonCamera.h"

#include "GameInstance.h"
#include "Client_Defines.h"
#include "ClientEvents.h"
#include "Player.h"
#include "ComCharacterMotor.h"

NS_USING(Client)

CPlayerThirdPersonCamera::CPlayerThirdPersonCamera() = default;

CPlayerThirdPersonCamera::CPlayerThirdPersonCamera(const CPlayerThirdPersonCamera& rhs)
	: CCameraObject{ rhs }
{
}

CPlayerThirdPersonCamera::~CPlayerThirdPersonCamera()
{
	// 이벤트 구독 해제
	//CGameInstance::Get().EventUnsubscribe<FRequestPlayerCameraShake>(m_iShakeListenerID);
	CGameInstance::Get().EventUnsubscribeAll(GetHandle());
}

HRESULT CPlayerThirdPersonCamera::Initialize(void* pArg)
{
	auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc ||
		pDesc->fDistance <= 0.f ||
		pDesc->fMinPitch > pDesc->fMaxPitch ||
		!std::isfinite(pDesc->fShoulderOffset) ||
		!std::isfinite(pDesc->fHorizontalDeadZoneRadius) ||
		pDesc->fHorizontalDeadZoneRadius < 0.f ||
		!std::isfinite(pDesc->fVerticalDeadZoneHalfHeight) ||
		pDesc->fVerticalDeadZoneHalfHeight < 0.f ||
		!std::isfinite(pDesc->fVerticalFollowSpeed) ||
		pDesc->fVerticalFollowSpeed <= 0.f)
		return E_FAIL;

	if (FAILED(CCameraObject::Initialize(pArg)))
		return E_FAIL;

	SetTimeDomain(TIME_DOMAIN::UNSCALED);

	m_hTarget = pDesc->hTarget;
	m_fYaw = pDesc->fYaw;
	m_fPitch = std::clamp(
		pDesc->fPitch,
		pDesc->fMinPitch,
		pDesc->fMaxPitch);
	m_fDistance = pDesc->fDistance;
	m_fCurrentDistance = m_fDistance;
	m_fMinPitch = pDesc->fMinPitch;
	m_fMaxPitch = pDesc->fMaxPitch;
	m_fMouseSensitivity = pDesc->fMouseSensitivity;
	m_fBaseFovY = pDesc->fFovY;
	m_fFovOverrideTarget = m_fBaseFovY;
	m_fShoulderOffset = pDesc->fShoulderOffset;
	m_fCurrentShoulderOffset = m_fShoulderOffset;
	m_fHorizontalDeadZoneRadius = pDesc->fHorizontalDeadZoneRadius;
	m_fVerticalDeadZoneHalfHeight = pDesc->fVerticalDeadZoneHalfHeight;
	m_fVerticalFollowSpeed = pDesc->fVerticalFollowSpeed;
	m_vFollowPivot = {};
	m_bFollowPivotInitialized = false;


	// 이벤트 등록
	m_iShakeListenerID = CGameInstance::Get().EventSubscribe<FRequestPlayerCameraShake>(GetHandle(), [this](const FRequestPlayerCameraShake& Event)
		{
			if (CGameInstance::Get().GetActiveCamera() != this)
			{
				return;
			}
			BeginShake(Event);
		}
	);

	return S_OK;
}

_bool CPlayerThirdPersonCamera::BeginFovOverride(
	_float fTargetFovY,
	_float fResponse)
{
	if (!std::isfinite(fTargetFovY) || fTargetFovY <= 0.f ||
		fTargetFovY >= 180.f || !std::isfinite(fResponse) || fResponse <= 0.f)
	{
		return false;
	}

	m_fFovOverrideTarget = fTargetFovY;
	m_fFovTransitionResponse = fResponse;
	m_bFovOverrideActive = true;
	m_bFovTransitionActive = true;
	return true;
}

void CPlayerThirdPersonCamera::EndFovOverride(_float fRestoreResponse)
{
	m_bFovOverrideActive = false;
	m_bFovTransitionActive = true;
	m_fFovTransitionResponse =
		std::isfinite(fRestoreResponse) && fRestoreResponse > 0.f ?
		fRestoreResponse : 10.f;
}

_bool CPlayerThirdPersonCamera::BeginDistanceOverride(
	_float fDistanceOffset, _float fResponse)
{
	if (!std::isfinite(fDistanceOffset) || fDistanceOffset < 0.f ||
		!std::isfinite(fResponse) || fResponse <= 0.f)
	{
		return false;
	}

	m_fDistanceOverrideOffset = fDistanceOffset;
	m_fDistanceOverrideResponse = fResponse;
	m_bDistanceOverrideActive = true;
	return true;
}

void CPlayerThirdPersonCamera::EndDistanceOverride(_float fRestoreResponse)
{
	m_bDistanceOverrideActive = false;
	m_fDistanceOverrideResponse =
		std::isfinite(fRestoreResponse) && fRestoreResponse > 0.f ?
		fRestoreResponse : 8.f;
}

_bool CPlayerThirdPersonCamera::BeginHeightOverride(
	_float fHeightOffset, _float fResponse)
{
	if (!std::isfinite(fHeightOffset) || fHeightOffset < 0.f ||
		!std::isfinite(fResponse) || fResponse <= 0.f)
	{
		return false;
	}

	m_fHeightOverrideOffset = fHeightOffset;
	m_fHeightOverrideResponse = fResponse;
	m_bHeightOverrideActive = true;
	return true;
}

void CPlayerThirdPersonCamera::EndHeightOverride(_float fRestoreResponse)
{
	m_bHeightOverrideActive = false;
	m_fHeightOverrideResponse =
		std::isfinite(fRestoreResponse) && fRestoreResponse > 0.f ?
		fRestoreResponse : 8.f;
}

void CPlayerThirdPersonCamera::PriorityUpdate(_float fTimeDelta)
{
	if (CGameInstance::Get().GetActiveCamera() != this ||
		!CGameInstance::Get().GetMouseFix())
	{
		return;
	}

	// MouseMove는 이미 프레임 동안 누적된 상대 이동량이다.
	// 여기에 가변 DeltaTime을 다시 곱하면 프레임 드랍 때 회전 감도가 달라져 끊겨 보인다.
	constexpr _float MOUSE_REFERENCE_FRAME = 1.f / 60.f;
	m_fYaw += CGameInstance::Get().MouseMove(MOUSEMOVESTATE::X) * m_fMouseSensitivity * MOUSE_REFERENCE_FRAME;
	m_fPitch += CGameInstance::Get().MouseMove(MOUSEMOVESTATE::Y) * m_fMouseSensitivity * MOUSE_REFERENCE_FRAME;
	m_fPitch = std::clamp(m_fPitch, m_fMinPitch, m_fMaxPitch);
}

void CPlayerThirdPersonCamera::UpdateFollow(_float fTimeDelta)
{
	if (CGameInstance::Get().GetActiveCamera() != this)
		return;

	auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(m_hTarget);
	if (!pTarget)
		return;

	_float3 vTargetPosition = pTarget->GetTransform().GetPosition();
	_bool bFlightCamera = false;
	_float fFlightVerticalLookAhead = 0.f;
	_float fFlightSpeed = 0.f;
	if (auto* pPlayer = Cast<CPlayer>(pTarget))
	{
		bFlightCamera = pPlayer->IsFlyRequested();
		if (bFlightCamera)
		{
			if (auto* pMotor = pPlayer->GetCharacterMotor())
			{
				const _float3 vVelocity = pMotor->GetVelocity();
				fFlightSpeed = std::sqrt(
					vVelocity.x * vVelocity.x +
					vVelocity.y * vVelocity.y +
					vVelocity.z * vVelocity.z);
				fFlightVerticalLookAhead = std::clamp(
					vVelocity.y * m_fFlightVerticalLookAheadTime,
					-m_fFlightMaxVerticalLookAhead,
					m_fFlightMaxVerticalLookAhead);
			}
		}
		// [LSY] 랙돌 중에는 고정된 Player Transform 대신 물리 Hips 위치를 추적한다.
		_float3 vRagdollFollowPosition{};
		if (pPlayer->TryGetRagdollFollowPosition(vRagdollFollowPosition))
			vTargetPosition = vRagdollFollowPosition;
	}
	const _float fTargetSpeedEffectRatio = bFlightCamera
		? std::clamp(
			(fFlightSpeed - m_fSpeedEffectStartSpeed) /
			(m_fSpeedEffectFullSpeed - m_fSpeedEffectStartSpeed),
			0.f, 1.f)
		: 0.f;

	const _float fTargetOffsetY = bFlightCamera
		? std::lerp(
			m_fFlightTargetOffsetY,
			m_fBoostTargetOffsetY,
			m_fCurrentSpeedEffectRatio)
		: CAMERA_TARGET_OFFSET_Y;
	const _float3 vPlayerFocus{
		vTargetPosition.x,
		vTargetPosition.y + fTargetOffsetY + fFlightVerticalLookAhead,
		vTargetPosition.z};

	if (!m_bFollowPivotInitialized)
	{
		m_vFollowPivot = vPlayerFocus;
		m_bFollowPivotInitialized = true;
	}

	const _float fHorizontalDeadZone = bFlightCamera
		? std::lerp(
			m_fFlightHorizontalDeadZoneRadius,
			m_fBoostHorizontalDeadZoneRadius,
			m_fCurrentSpeedEffectRatio)
		: m_fHorizontalDeadZoneRadius;
	const _vector vHorizontalDelta = XMVectorSet(vPlayerFocus.x - m_vFollowPivot.x, 0.f, vPlayerFocus.z - m_vFollowPivot.z, 0.f);
	_float fDeadZoneDistance{};
	XMStoreFloat(&fDeadZoneDistance, XMVector3Length(vHorizontalDelta));

	if (fDeadZoneDistance > fHorizontalDeadZone)
	{
		const _float fExcessDistance = fDeadZoneDistance - fHorizontalDeadZone;
		const _vector vPivotDisplacement = vHorizontalDelta / fDeadZoneDistance * fExcessDistance;

		_float3 vPivotMove{};
		XMStoreFloat3(&vPivotMove, vPivotDisplacement);
		const _float fHorizontalFollowSpeed = std::lerp(
			m_fFlightHorizontalFollowSpeed,
			m_fBoostHorizontalFollowSpeed,
			m_fCurrentSpeedEffectRatio);
		const _float fHorizontalFollowRatio = bFlightCamera
			? 1.f - std::exp(-fHorizontalFollowSpeed * std::max(fTimeDelta, 0.f))
			: 1.f;
		m_vFollowPivot.x += vPivotMove.x * fHorizontalFollowRatio;
		m_vFollowPivot.z += vPivotMove.z * fHorizontalFollowRatio;
	}

	// 비행 중에는 세로 데드존을 좁히고 추종 속도를 높여 급격한 고도 변화도 따라간다.
	const _float fVerticalDeadZone = bFlightCamera
		? m_fFlightVerticalDeadZoneHalfHeight
		: m_fVerticalDeadZoneHalfHeight;
	const _float fVerticalFollowSpeed = bFlightCamera
		? m_fFlightVerticalFollowSpeed
		: m_fVerticalFollowSpeed;

	_float fDesiredPivotY = m_vFollowPivot.y;
	if (vPlayerFocus.y > m_vFollowPivot.y + fVerticalDeadZone)
	{
		fDesiredPivotY = vPlayerFocus.y - fVerticalDeadZone;
	}
	else if (vPlayerFocus.y < m_vFollowPivot.y - fVerticalDeadZone)
	{
		fDesiredPivotY = vPlayerFocus.y + fVerticalDeadZone;
	}

	const _float fSpeedEffectBlendRatio = 1.f - std::exp(
		-m_fSpeedEffectResponse * std::max(fTimeDelta, 0.f));
	m_fCurrentSpeedEffectRatio = std::lerp(
		m_fCurrentSpeedEffectRatio,
		fTargetSpeedEffectRatio,
		fSpeedEffectBlendRatio);

	const _float fBaseDesiredFovY =
		m_fBaseFovY + m_fSpeedFovExpansion * m_fCurrentSpeedEffectRatio;
	if (m_bFovOverrideActive || m_bFovTransitionActive)
	{
		const _float fTargetFovY = m_bFovOverrideActive ?
			m_fFovOverrideTarget : fBaseDesiredFovY;
		const _float fFovBlendRatio = 1.f - std::exp(
			-m_fFovTransitionResponse * std::max(fTimeDelta, 0.f));
		const _float fBlendedFovY = std::lerp(
			GetFovY(), fTargetFovY, fFovBlendRatio);
		if (std::abs(fBlendedFovY - fTargetFovY) <= 0.01f)
		{
			SetFovY(fTargetFovY);
			if (!m_bFovOverrideActive)
				m_bFovTransitionActive = false;
		}
		else
		{
			SetFovY(fBlendedFovY);
		}
	}
	else if (std::abs(GetFovY() - fBaseDesiredFovY) > 0.01f)
	{
		SetFovY(fBaseDesiredFovY);
	}
	const _float fVerticalFollowRatio = 1.f - std::exp(-fVerticalFollowSpeed * std::max(fTimeDelta, 0.f));
	m_vFollowPivot.y = std::lerp(m_vFollowPivot.y, fDesiredPivotY, fVerticalFollowRatio);

	if (bFlightCamera)
	{
		// 기존 hard clamp 대신 안전 구도 경계까지 부드럽게 보정한다.
		// FixedUpdate 위치가 계단식으로 바뀌어도 카메라가 한 프레임에 순간 이동하지 않는다.
		const _float fVerticalError = vPlayerFocus.y - m_vFollowPivot.y;
		if (std::abs(fVerticalError) > m_fFlightMaxVerticalCompositionError)
		{
			const _float fSafePivotY = vPlayerFocus.y - std::copysign(
				m_fFlightMaxVerticalCompositionError, fVerticalError);
			const _float fCorrectionRatio = 1.f - std::exp(
				-m_fFlightCompositionCorrectionSpeed * std::max(fTimeDelta, 0.f));
			m_vFollowPivot.y = std::lerp(
				m_vFollowPivot.y, fSafePivotY, fCorrectionRatio);
		}
	}

	// 비행 중에는 캐릭터가 더 크게 보이도록 카메라를 당기고,
	// 탑승/하차 시에는 거리 변화가 튀지 않도록 지수 보간한다.
	const _float fTargetDistance = bFlightCamera
		? m_fFlightDistance + m_fSpeedDistanceExtension * m_fCurrentSpeedEffectRatio
		: m_fDistance + (m_bDistanceOverrideActive ?
			m_fDistanceOverrideOffset : 0.f);
	const _float fDistanceResponse = bFlightCamera ?
		m_fFlightDistanceResponse : m_fDistanceOverrideResponse;
	const _float fDistanceRatio = 1.f - std::exp(
		-fDistanceResponse * std::max(fTimeDelta, 0.f));
	m_fCurrentDistance = std::lerp(
		m_fCurrentDistance, fTargetDistance, fDistanceRatio);
	const _float fTargetHeightOverride = m_bHeightOverrideActive ?
		m_fHeightOverrideOffset : 0.f;
	const _float fHeightRatio = 1.f - std::exp(
		-m_fHeightOverrideResponse * std::max(fTimeDelta, 0.f));
	m_fCurrentHeightOverrideOffset = std::lerp(
		m_fCurrentHeightOverrideOffset, fTargetHeightOverride, fHeightRatio);
	const _float fTargetShoulderOffset = bFlightCamera
		? m_fFlightShoulderOffset
		: m_fShoulderOffset;
	const _float fShoulderRatio = 1.f - std::exp(
		-m_fFlightShoulderResponse * std::max(fTimeDelta, 0.f));
	m_fCurrentShoulderOffset = std::lerp(
		m_fCurrentShoulderOffset, fTargetShoulderOffset, fShoulderRatio);
	const _float fTargetCameraHeightOffset = bFlightCamera
		? m_fFlightCameraHeightOffset
		: 0.f;
	m_fCurrentFlightCameraHeightOffset = std::lerp(
		m_fCurrentFlightCameraHeightOffset,
		fTargetCameraHeightOffset,
		fShoulderRatio);

	const _float fYawRadian = XMConvertToRadians(m_fYaw);
	const _float fPitchRadian = XMConvertToRadians(m_fPitch);
	const _float fHorizontalDistance = std::cos(fPitchRadian) * m_fCurrentDistance;
	const _float3 vForward{ std::sin(fYawRadian), 0.f, std::cos(fYawRadian) };
	const _float3 vRight{std::cos(fYawRadian), 0.f, -std::sin(fYawRadian) };
	const _float3 vCompositionPivot{m_vFollowPivot.x + vRight.x * m_fCurrentShoulderOffset, m_vFollowPivot.y, m_vFollowPivot.z + vRight.z * m_fCurrentShoulderOffset };
	const _float3 vDesiredPosition{vCompositionPivot.x - vForward.x * fHorizontalDistance, vCompositionPivot.y + std::sin(fPitchRadian) * m_fCurrentDistance + m_fCurrentFlightCameraHeightOffset + m_fCurrentHeightOverrideOffset, vCompositionPivot.z - vForward.z * fHorizontalDistance };


	_float3 finalPosition{};
	const _bool bCameraCollision = PlayerToCameraSphereSweep(
		m_vFollowPivot, vDesiredPosition, CAMERA_COLLISION_RADIUS, finalPosition);

	if (!m_bSmoothedCameraPositionInitialized || !bFlightCamera)
	{
		m_vSmoothedCameraPosition = finalPosition;
		m_bSmoothedCameraPositionInitialized = true;
	}
	else if (bCameraCollision)
	{
		// 벽 안으로 보간되지 않도록 충돌 방향으로 당겨질 때는 즉시 반영한다.
		m_vSmoothedCameraPosition = finalPosition;
	}
	else
	{
		// 자유 비행 및 충돌 해제 시에는 회전 위치를 부드럽게 따라가게 한다.
		const _float fCameraPositionRatio = 1.f - std::exp(
			-m_fFlightCameraPositionResponse * std::max(fTimeDelta, 0.f));
		m_vSmoothedCameraPosition.x = std::lerp(m_vSmoothedCameraPosition.x, finalPosition.x, fCameraPositionRatio);
		m_vSmoothedCameraPosition.y = std::lerp(m_vSmoothedCameraPosition.y, finalPosition.y, fCameraPositionRatio);
		m_vSmoothedCameraPosition.z = std::lerp(m_vSmoothedCameraPosition.z, finalPosition.z, fCameraPositionRatio);
	}

	auto& CameraTransform = GetTransform();
	CameraTransform.SetPosition(m_vSmoothedCameraPosition);
	CameraTransform.LookAt(XMLoadFloat3(&vCompositionPivot));

	// 셰이킹 상태 합성
	// LookAt이 만든 기본 상태 보관
	const _vector vBasePosition = CameraTransform.GetLoadedPostion();
	const _vector qBaseRotation = CameraTransform.GetLoadedQuaternion();

	const _vector vCameraRight = CameraTransform.GetState(STATE::RIGHT);
	const _vector vCameraUp = CameraTransform.GetState(STATE::UP);

	_float3 vLocalPositionShake{};
	_float3 vRotationShake{};

	EvaluateShake(fTimeDelta, vLocalPositionShake, vRotationShake);

	// 고속 비행 시 서로 다른 주기의 미세 진동을 합성해 반복 패턴을 줄인다.
	m_fTurbulenceElapsed += std::max(fTimeDelta, 0.f);
	const _float fTurbulenceRatio = std::clamp(
		(m_fCurrentSpeedEffectRatio - m_fTurbulenceStartRatio) /
		(1.f - m_fTurbulenceStartRatio),
		0.f, 1.f);
	if (fTurbulenceRatio > FLT_EPSILON)
	{
		const _float fFrequency = std::lerp(
			m_fTurbulenceMinFrequency,
			m_fTurbulenceMaxFrequency,
			fTurbulenceRatio);
		const _float fPhase = m_fTurbulenceElapsed * fFrequency;
		const _float fAmplitude =
			fTurbulenceRatio * fTurbulenceRatio * (3.f - 2.f * fTurbulenceRatio);
		vLocalPositionShake.x +=
			std::sin(fPhase * 1.13f) * m_fTurbulencePositionAmplitude * fAmplitude;
		vLocalPositionShake.y +=
			std::sin(fPhase * 1.71f + 1.2f) * m_fTurbulencePositionAmplitude * 0.7f * fAmplitude;
		vRotationShake.x +=
			std::sin(fPhase * 1.37f + 0.4f) * m_fTurbulenceRotationAmplitude * fAmplitude;
		vRotationShake.z +=
			std::sin(fPhase * 1.91f + 2.1f) * m_fTurbulenceRotationAmplitude * 0.6f * fAmplitude;
	}

	// 로컬 위치 흔들림을 월드 공간으로 변환
	const _vector vWorldPositionShake = vCameraRight * vLocalPositionShake.x + vCameraUp * vLocalPositionShake.y;

	CameraTransform.SetPosition(vBasePosition + vWorldPositionShake);

	// LookAt이 만든 기본 회전에 로컬 회전 흔들림 합성
	const _vector qShake = XMQuaternionRotationRollPitchYaw(
		XMConvertToRadians(vRotationShake.x),
		XMConvertToRadians(vRotationShake.y),
		XMConvertToRadians(vRotationShake.z));

	CameraTransform.SetQuaternion(XMQuaternionMultiply(qBaseRotation, qShake));

	CameraTransform.Update();
	UpdateViewMatrix();
}

_bool CPlayerThirdPersonCamera::PlayerToCameraSphereSweep(const _float3& PlayerPosition, const _float3& CameraPosition, _float fCollisionRadius, _float3& OutCameraPosition) const
{
	OutCameraPosition = CameraPosition;

	if (!std::isfinite(fCollisionRadius) || fCollisionRadius <= 0.f)
	{
		return false;
	}

	const _vector vTargetPosition = XMLoadFloat3(&PlayerPosition);
	const _vector vCameraOffset = XMLoadFloat3(&CameraPosition) - vTargetPosition;

	_float fCameraDistance{};
	XMStoreFloat(&fCameraDistance, XMVector3Length(vCameraOffset));
	if (!std::isfinite(fCameraDistance) || fCameraDistance <= FLT_EPSILON)
	{
		return false;
	}

	CPhysXManager* pPhysXManager = CGameInstance::Get().GetPhysXManager();
	if (pPhysXManager == nullptr)
	{
		return false;
	}

	_float3 vDirection{};
	XMStoreFloat3(&vDirection, vCameraOffset / fCameraDistance);

	PX_SWEEP_DESC Desc{};
	Desc.tGeometry.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE;
	Desc.tGeometry.fRadius = fCollisionRadius;
	Desc.tPose.vPosition = PlayerPosition;
	Desc.vDirection = vDirection;
	Desc.fMaxDistance = fCameraDistance;
	Desc.tFilter.bQueryStatic = true;
	Desc.tFilter.bQueryDynamic = false;
	Desc.tFilter.bIncludeTrigger = false;
	Desc.tFilter.iQueryMask =
		ETOUI(COLLISION_LAYER::DEFAULT)
		| ETOUI(COLLISION_LAYER::WORLD_STATIC);
	//| ETOUI(COLLISION_LAYER::MOVING_PLATFORM)


	PX_SWEEP_RESULT Hit{};
	if (!pPhysXManager->Sweep(Desc, Hit) || !Hit.bHit || !std::isfinite(Hit.fDistance) || Hit.fDistance <= FLT_EPSILON)
	{
		return false;
	}

	const _float fCorrectedDistance = std::max(0.f, Hit.fDistance - CAMERA_COLLISION_PADDING);
	XMStoreFloat3(&OutCameraPosition, vTargetPosition + XMLoadFloat3(&vDirection) * fCorrectedDistance);

	return true;
}

void CPlayerThirdPersonCamera::BeginShake(const FRequestPlayerCameraShake& Event)
{
	if (!std::isfinite(Event.fIntensity) ||
		!std::isfinite(Event.fDuration) ||
		!std::isfinite(Event.fFrequency) ||
		Event.fIntensity <= 0.f ||
		Event.fDuration <= 0.f ||
		Event.fFrequency <= 0.f)
	{
		return;
	}

	m_ShakeState.bActive = true;
	m_ShakeState.fElapsed = 0.f;
	m_ShakeState.fDuration = Event.fDuration;
	m_ShakeState.fIntensity =std::clamp(Event.fIntensity, 0.f, 1.f);
	m_ShakeState.fFrequency = Event.fFrequency;
}

void CPlayerThirdPersonCamera::EvaluateShake(_float fTimeDelta, _float3& OutLocalPositionOffset, _float3& OutRotationOffset)
{
	OutLocalPositionOffset = {};
	OutRotationOffset = {};

	if (!m_ShakeState.bActive)
	{
		return;
	}

	m_ShakeState.fElapsed += std::max(fTimeDelta, 0.f);

	const _float fRatio = std::clamp(m_ShakeState.fElapsed / m_ShakeState.fDuration, 0.f, 1.f);

	// 끝으로 갈수록 부드럽게 감소
	const _float fEnvelope = (1.f - fRatio) * (1.f - fRatio);
	const _float fAmplitude = m_ShakeState.fIntensity * fEnvelope;
	const _float fPhase = m_ShakeState.fElapsed * m_ShakeState.fFrequency * XM_2PI;

	const _float fNoiseX = std::sin(fPhase);
	const _float fNoiseY =std::sin(fPhase * 1.37f + 1.3f);
	const _float fNoiseRoll =std::sin(fPhase * 1.79f + 2.1f);

	// 월드 단위
	constexpr _float MAX_POSITION_SHAKE = 0.12f;

	// Degree 단위
	constexpr _float MAX_ROTATION_SHAKE = 1.5f;

	OutLocalPositionOffset.x = fNoiseX * MAX_POSITION_SHAKE * fAmplitude;
	OutLocalPositionOffset.y = fNoiseY * MAX_POSITION_SHAKE * fAmplitude;

	OutRotationOffset.x = fNoiseY * MAX_ROTATION_SHAKE * fAmplitude;
	OutRotationOffset.y = fNoiseX * MAX_ROTATION_SHAKE * fAmplitude;
	OutRotationOffset.z = fNoiseRoll * MAX_ROTATION_SHAKE * 0.5f * fAmplitude;

	if (m_ShakeState.fElapsed >= m_ShakeState.fDuration)
	{
		m_ShakeState = {};
	}
}

UPtr<CPlayerThirdPersonCamera> CPlayerThirdPersonCamera::Create()
{
	auto pInstance = ToUPtr(new CPlayerThirdPersonCamera{});
	if (FAILED(pInstance->InitializePrototype()))
		return nullptr;
	return pInstance;
}

UPtr<CPrototype> CPlayerThirdPersonCamera::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CPlayerThirdPersonCamera{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
		return nullptr;
	return pInstance;
}
