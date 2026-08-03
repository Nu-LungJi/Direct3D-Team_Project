#include "pch.h"
#include "PlayerThirdPersonCamera.h"

#include "GameInstance.h"
#include "Client_Defines.h"
#include "ClientEvents.h"
#include "Player.h"

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

	m_hTarget = pDesc->hTarget;
	m_fPitch = std::clamp(
		pDesc->fPitch,
		pDesc->fMinPitch,
		pDesc->fMaxPitch);
	m_fDistance = pDesc->fDistance;
	m_fMinPitch = pDesc->fMinPitch;
	m_fMaxPitch = pDesc->fMaxPitch;
	m_fMouseSensitivity = pDesc->fMouseSensitivity;
	m_fShoulderOffset = pDesc->fShoulderOffset;
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

void CPlayerThirdPersonCamera::PriorityUpdate(_float fTimeDelta)
{
	if (CGameInstance::Get().GetActiveCamera() != this ||
		!CGameInstance::Get().GetMouseFix())
	{
		return;
	}

	m_fYaw += CGameInstance::Get().MouseMove(MOUSEMOVESTATE::X) * m_fMouseSensitivity * fTimeDelta;
	m_fPitch += CGameInstance::Get().MouseMove(MOUSEMOVESTATE::Y) * m_fMouseSensitivity * fTimeDelta;
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
	if (auto* pPlayer = Cast<CPlayer>(pTarget))
	{
		// [LSY] 랙돌 중에는 고정된 Player Transform 대신 물리 Hips 위치를 추적한다.
		_float3 vRagdollFollowPosition{};
		if (pPlayer->TryGetRagdollFollowPosition(vRagdollFollowPosition))
			vTargetPosition = vRagdollFollowPosition;
	}

	const _float3 vPlayerFocus{
		vTargetPosition.x,
		vTargetPosition.y + CAMERA_TARGET_OFFSET_Y,
		vTargetPosition.z};

	if (!m_bFollowPivotInitialized)
	{
		m_vFollowPivot = vPlayerFocus;
		m_bFollowPivotInitialized = true;
	}

	const _vector vHorizontalDelta = XMVectorSet(vPlayerFocus.x - m_vFollowPivot.x, 0.f, vPlayerFocus.z - m_vFollowPivot.z, 0.f);
	_float fDeadZoneDistance{};
	XMStoreFloat(&fDeadZoneDistance, XMVector3Length(vHorizontalDelta));

	if (fDeadZoneDistance > m_fHorizontalDeadZoneRadius)
	{
		const _float fExcessDistance = fDeadZoneDistance - m_fHorizontalDeadZoneRadius;
		const _vector vPivotDisplacement = vHorizontalDelta / fDeadZoneDistance * fExcessDistance;

		_float3 vPivotMove{};
		XMStoreFloat3(&vPivotMove, vPivotDisplacement);
		m_vFollowPivot.x += vPivotMove.x;
		m_vFollowPivot.z += vPivotMove.z;
	}

	_float fDesiredPivotY = m_vFollowPivot.y;
	if (vPlayerFocus.y > m_vFollowPivot.y + m_fVerticalDeadZoneHalfHeight)
	{
		fDesiredPivotY = vPlayerFocus.y - m_fVerticalDeadZoneHalfHeight;
	}
	else if (vPlayerFocus.y < m_vFollowPivot.y - m_fVerticalDeadZoneHalfHeight)
	{
		fDesiredPivotY = vPlayerFocus.y + m_fVerticalDeadZoneHalfHeight;
	}

	const _float fVerticalFollowRatio = 1.f - std::exp(-m_fVerticalFollowSpeed * std::max(fTimeDelta, 0.f));
	m_vFollowPivot.y = std::lerp(m_vFollowPivot.y, fDesiredPivotY, fVerticalFollowRatio);

	const _float fYawRadian = XMConvertToRadians(m_fYaw);
	const _float fPitchRadian = XMConvertToRadians(m_fPitch);
	const _float fHorizontalDistance = std::cos(fPitchRadian) * m_fDistance;
	const _float3 vForward{ std::sin(fYawRadian), 0.f, std::cos(fYawRadian) };
	const _float3 vRight{std::cos(fYawRadian), 0.f, -std::sin(fYawRadian) };
	const _float3 vCompositionPivot{m_vFollowPivot.x + vRight.x * m_fShoulderOffset, m_vFollowPivot.y, m_vFollowPivot.z + vRight.z * m_fShoulderOffset };
	const _float3 vDesiredPosition{vCompositionPivot.x - vForward.x * fHorizontalDistance, vCompositionPivot.y + std::sin(fPitchRadian) * m_fDistance, vCompositionPivot.z - vForward.z * fHorizontalDistance };


	_float3 finalPosition{};
	PlayerToCameraSphereSweep(m_vFollowPivot, vDesiredPosition, CAMERA_COLLISION_RADIUS, finalPosition);

	auto& CameraTransform = GetTransform();
	CameraTransform.SetPosition(finalPosition);
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
