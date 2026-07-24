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

void CPlayerThirdPersonCamera::UpdateFollow()
{
	if (CGameInstance::Get().GetActiveCamera() != this)
		return;

	auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(m_hTarget);
	if (!pTarget)
		return;

	const _float3 vTargetPosition = pTarget->GetTransform().GetPosition();
	const _float3 vTarget{
		vTargetPosition.x,
		vTargetPosition.y + m_fTargetHeight,
		vTargetPosition.z };

	const _float fYawRadian = XMConvertToRadians(m_fYaw);
	const _float fPitchRadian = XMConvertToRadians(m_fPitch);
	const _float fHorizontalDistance = std::cos(fPitchRadian) * m_fDistance;
	const _float3 vForward{ std::sin(fYawRadian), 0.f, std::cos(fYawRadian) };
	const _float3 vDesiredPosition{
		vTarget.x - vForward.x * fHorizontalDistance,
		vTarget.y + std::sin(fPitchRadian) * m_fDistance,
		vTarget.z - vForward.z * fHorizontalDistance };

	auto& CameraTransform = GetTransform();
	CameraTransform.SetPosition(vDesiredPosition);
	CameraTransform.LookAt(XMLoadFloat3(&vTarget));
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
