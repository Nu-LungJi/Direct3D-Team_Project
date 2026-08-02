#include "pch.h"
#include "PlayerThirdPersonCamera.h"

#include "GameInstance.h"
#include "Client_Defines.h"

NS_USING(Client)

CPlayerThirdPersonCamera::CPlayerThirdPersonCamera() = default;

CPlayerThirdPersonCamera::CPlayerThirdPersonCamera(const CPlayerThirdPersonCamera& rhs)
	: CCameraObject{ rhs }
{
}

CPlayerThirdPersonCamera::~CPlayerThirdPersonCamera() = default;

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

	const _float3 vTargetPosition = pTarget->GetTransform().GetPosition();
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
