#include "pch.h"
#include "PlayerThirdPersonCamera.h"

#include "GameInstance.h"

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
	if (!pDesc || pDesc->fDistance <= 0.f || pDesc->fMinPitch > pDesc->fMaxPitch)
		return E_FAIL;

	if (FAILED(CCameraObject::Initialize(pArg)))
		return E_FAIL;

	m_hTarget = pDesc->hTarget;
	m_fPitch = std::clamp(pDesc->fPitch, pDesc->fMinPitch, pDesc->fMaxPitch);
	m_fDistance = pDesc->fDistance;
	m_fTargetHeight = pDesc->fTargetHeight;
	m_fMinPitch = pDesc->fMinPitch;
	m_fMaxPitch = pDesc->fMaxPitch;
	m_fMouseSensitivity = pDesc->fMouseSensitivity;
	m_fShoulderOffset = pDesc->fShoulderOffset;
	m_fLookSideOffset = pDesc->fLookSideOffset;
	m_fLookHeightOffset = pDesc->fLookHeightOffset;
	m_fPositionSmoothSpeed = pDesc->fPositionSmoothSpeed;
	m_fLookSmoothSpeed = pDesc->fLookSmoothSpeed;
	return S_OK;
}

void CPlayerThirdPersonCamera::PriorityUpdate(_float fTimeDelta)
{
	if (CGameInstance::Get().GetActiveCamera() != this ||
		!CGameInstance::Get().GetMouseFix())
	{
		return;
	}

	m_fYaw += CGameInstance::Get().MouseMove(MOUSEMOVESTATE::X) *
		m_fMouseSensitivity * fTimeDelta;
	m_fPitch += CGameInstance::Get().MouseMove(MOUSEMOVESTATE::Y) *
		m_fMouseSensitivity * fTimeDelta;
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
	const _float3 vOrbitCenter{
		vTargetPosition.x,
		vTargetPosition.y + m_fTargetHeight,
		vTargetPosition.z };
	_float3 vLookTarget = vOrbitCenter;

	const _float fYawRadian = XMConvertToRadians(m_fYaw);
	const _float fPitchRadian = XMConvertToRadians(m_fPitch);
	const _float fHorizontalDistance = std::cos(fPitchRadian) * m_fDistance;
	const _float3 vForward{ std::sin(fYawRadian), 0.f, std::cos(fYawRadian) };
	const _float3 vCameraRight{ vForward.z, 0.f, -vForward.x };


	vLookTarget.x += vCameraRight.x * m_fLookSideOffset;
	vLookTarget.y += m_fLookHeightOffset;
	vLookTarget.z += vCameraRight.z * m_fLookSideOffset;

	const _float3 vDesiredPosition{
		vOrbitCenter.x - vForward.x * fHorizontalDistance +
			vCameraRight.x * m_fShoulderOffset,
		vOrbitCenter.y + std::sin(fPitchRadian) * m_fDistance,
		vOrbitCenter.z - vForward.z * fHorizontalDistance +
			vCameraRight.z * m_fShoulderOffset };

	if (!m_bFollowInitialized)
	{
		m_vSmoothedPosition = vDesiredPosition;
		m_vSmoothedLookTarget = vLookTarget;
		m_bFollowInitialized = true;
	}
	else
	{
		// 지형/물리 업데이트의 계단식 위치 변화를 프레임 독립적으로 감쇠한다.
		const _float fPositionAlpha =
			1.f - std::exp(-m_fPositionSmoothSpeed * fTimeDelta);
		const _float fLookAlpha =
			1.f - std::exp(-m_fLookSmoothSpeed * fTimeDelta);

		XMStoreFloat3(
			&m_vSmoothedPosition,
			XMVectorLerp(
				XMLoadFloat3(&m_vSmoothedPosition),
				XMLoadFloat3(&vDesiredPosition),
				fPositionAlpha));
		XMStoreFloat3(
			&m_vSmoothedLookTarget,
			XMVectorLerp(
				XMLoadFloat3(&m_vSmoothedLookTarget),
				XMLoadFloat3(&vLookTarget),
				fLookAlpha));
	}

	auto& CameraTransform = GetTransform();
	CameraTransform.SetPosition(m_vSmoothedPosition);
	CameraTransform.LookAt(XMLoadFloat3(&m_vSmoothedLookTarget));
	CameraTransform.Update();
	UpdateViewMatrix();
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
