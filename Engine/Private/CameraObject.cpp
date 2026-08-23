#include "pch.h"

#include "CameraObject.h"
#include "CollFrustum.h"
#include "CollOrientedBox.h"
#include "GameInstance.h"
#include "ffx_fsr2.h"

NS_USING(Engine)


CCameraObject::CCameraObject()
{
}

CCameraObject::CCameraObject(const CCameraObject& Prototype)
	: CGameObject{ Prototype }
{
}

CCameraObject::~CCameraObject()
{
}

std::pair<_float3, _float3> CCameraObject::GetRayFromScreenPixel(
	const _float2& vScreenPixel,
	const _float2& vViewportSize) const
{
	if (vViewportSize.x <= 0.f || vViewportSize.y <= 0.f)
		return GetRay();

	const _matrix matView = GetView();
	const _matrix matProj = GetProj();
	const _matrix matWorld = XMMatrixIdentity();

	const _vector vScreenNear = XMVectorSet(
		vScreenPixel.x,
		vScreenPixel.y,
		0.f,
		1.f);
	const _vector vScreenFar = XMVectorSet(
		vScreenPixel.x,
		vScreenPixel.y,
		1.f,
		1.f);

	const _vector vWorldNear = XMVector3Unproject(
		vScreenNear,
		0.f,
		0.f,
		vViewportSize.x,
		vViewportSize.y,
		0.f,
		1.f,
		matProj,
		matView,
		matWorld);
	const _vector vWorldFar = XMVector3Unproject(
		vScreenFar,
		0.f,
		0.f,
		vViewportSize.x,
		vViewportSize.y,
		0.f,
		1.f,
		matProj,
		matView,
		matWorld);

	_vector vRayOrigin = vWorldNear;
	if (m_cameraDesc.eProj == PROJ::PERSPECTIVE)
	{
		const _matrix matInverseView = XMMatrixInverse(nullptr, matView);
		vRayOrigin = XMVector3TransformCoord(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			matInverseView);
	}

	const _vector vRayDirection = XMVector3Normalize(vWorldFar - vRayOrigin);

	_float3 vOrigin{};
	_float3 vDirection{};
	XMStoreFloat3(&vOrigin, vRayOrigin);
	XMStoreFloat3(&vDirection, vRayDirection);
	return { vOrigin, vDirection };
}

void CCameraObject::FSRCameraJitter()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();
	auto displaySize = CGameInstance::Get().GetDisplayScreenSize();
	auto frameIndex = CGameInstance::Get().GetFrameCnt();
	auto fsrIndex = static_cast<int32_t>(frameIndex % INT32_MAX);
	const int32_t jitterPhaseCount = ffxFsr2GetJitterPhaseCount(clientSize.x, displaySize.x);
	float jitterX = 0.0f, jitterY = 0.0f;
	ffxFsr2GetJitterOffset(&jitterX, &jitterY, fsrIndex, jitterPhaseCount);
	m_pairCameraJitter.first = jitterX;
	m_pairCameraJitter.second = jitterY;
}

HRESULT CCameraObject::Initialize(void* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	};

	XMStoreFloat4x4(&m_matView, XMMatrixIdentity());
	XMStoreFloat4x4(&m_matProj, XMMatrixIdentity());

	CAMERA_DESC* cameraDesc = static_cast<CAMERA_DESC*>(pArg);
	m_cameraDesc = *cameraDesc;

	auto pTransform = GetComponent<CComTransform>("Com_Transform");
	_fvector vPos = XMVectorSet(m_cameraDesc.vEye.x, m_cameraDesc.vEye.y, m_cameraDesc.vEye.z, 1.f);
	//pTransform->SetState(STATE::POSITION, vPos);
	pTransform->SetPosition(vPos);
	_fvector vAt = XMVectorSet(m_cameraDesc.vAt.x, m_cameraDesc.vAt.y, m_cameraDesc.vAt.z, 1.f);
	pTransform->LookAt(vAt, XMLoadFloat3(&cameraDesc->vUp));

	if (FAILED(UpdateProjMatrix()))
	{
		return E_FAIL;
	}

	if (FAILED(UpdateViewMatrix()))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CCameraObject::SetFovY(_float fFovY)
{
	if (m_cameraDesc.eProj != PROJ::PERSPECTIVE ||
		!std::isfinite(fFovY) || fFovY <= 0.f || fFovY >= 180.f)
	{
		return E_INVALIDARG;
	}

	if (std::abs(m_cameraDesc.fFovY - fFovY) <= FLT_EPSILON)
		return S_OK;

	const _float fPreviousFovY = m_cameraDesc.fFovY;
	m_cameraDesc.fFovY = fFovY;
	const HRESULT hr = UpdateProjMatrix();
	if (FAILED(hr))
	{
		m_cameraDesc.fFovY = fPreviousFovY;
		UpdateProjMatrix();
	}
	return hr;
}

HRESULT CCameraObject::UpdateViewMatrix()
{
	auto& pTransform = GetTransform();
	auto vEye = pTransform.GetLoadedPostion();
	auto vLook = pTransform.GetState(STATE::LOOK);
	E::_float3 tmp;
	XMStoreFloat3(&tmp, vLook);
	E::_float3 tmp2;
	XMStoreFloat3(&tmp2, vEye);
	auto vAt = vLook + vEye;

	XMStoreFloat4x4(&m_matView,
		XMMatrixLookAtLH(vEye, vAt, XMLoadFloat3(&m_cameraDesc.vUp)));

	return UpdateViewVolume();
}

HRESULT CCameraObject::UpdateProjMatrix()
{
	if (m_cameraDesc.eProj == PROJ::ORTHOGRAPHIC)
	{
		XMStoreFloat4x4(&m_matProj,
			XMMatrixOrthographicLH(m_cameraDesc.fWidth, m_cameraDesc.fHeight, m_cameraDesc.fNear, m_cameraDesc.fFar)
		);
	}
	else if (m_cameraDesc.eProj == PROJ::PERSPECTIVE)
	{
		XMStoreFloat4x4(&m_matProj,
			XMMatrixPerspectiveFovLH(
				XMConvertToRadians(m_cameraDesc.fFovY),
				m_cameraDesc.fAspect,
				m_cameraDesc.fNear,
				m_cameraDesc.fFar)
		);

		// FOV가 런타임에 바뀌어도 절두체 원거리 코너가 투영행렬과 일치해야 한다.
		const _float fHalfHeight = m_cameraDesc.fFar *
			tanf(0.5f * XMConvertToRadians(m_cameraDesc.fFovY));
		const _float fHalfWidth = m_cameraDesc.fAspect * fHalfHeight;
		m_FrustumFarCorner[0] = {
			-fHalfWidth, -fHalfHeight, m_cameraDesc.fFar, 0.f };
		m_FrustumFarCorner[1] = {
			-fHalfWidth, +fHalfHeight, m_cameraDesc.fFar, 0.f };
		m_FrustumFarCorner[2] = {
			+fHalfWidth, +fHalfHeight, m_cameraDesc.fFar, 0.f };
		m_FrustumFarCorner[3] = {
			+fHalfWidth, -fHalfHeight, m_cameraDesc.fFar, 0.f };
	}
	else
	{
		return E_FAIL;
	}

	if (m_cameraDesc.eProj == PROJ::PERSPECTIVE)
	{
		if (!m_pViewVolumeCollider || m_pViewVolumeCollider->GetCollType() != CollType::Frustum)
		{
			m_pViewVolumeCollider = CCollFrustum::Create(XMLoadFloat4x4(&m_matProj));
		}
		else
		{
			static_cast<CCollFrustum*>(m_pViewVolumeCollider.get())
				->SetLocalFrustum(XMLoadFloat4x4(&m_matProj));
		}
	}
	else
	{
		const _float fDepth = m_cameraDesc.fFar - m_cameraDesc.fNear;
		const _float3 vCenter{ 0.f, 0.f, m_cameraDesc.fNear + fDepth * 0.5f };
		const _float3 vExtents{
			m_cameraDesc.fWidth * 0.5f,
			m_cameraDesc.fHeight * 0.5f,
			fDepth * 0.5f };
		const _float4 vOrientation{ 0.f, 0.f, 0.f, 1.f };

		if (!m_pViewVolumeCollider || m_pViewVolumeCollider->GetCollType() != CollType::OrientedBox)
		{
			m_pViewVolumeCollider = CCollOrientedBox::Create(vCenter, vExtents, vOrientation);
		}
		else
		{
			static_cast<CCollOrientedBox*>(m_pViewVolumeCollider.get())
				->SetLocalBoundingOrientedBox(vCenter, vExtents, vOrientation);
		}
	}

	if (!m_pViewVolumeCollider)
		return E_FAIL;

	return UpdateViewVolume();
}

HRESULT CCameraObject::UpdateViewVolume()
{
	if (!m_pViewVolumeCollider)
		return E_FAIL;

	const _matrix matView = XMLoadFloat4x4(&m_matView);
	_vector vDeterminant = XMMatrixDeterminant(matView);
	m_pViewVolumeCollider->Transform(XMMatrixInverse(&vDeterminant, matView));
	return S_OK;
}

ContainmentType CCameraObject::ContainsViewVolume(const BoundingBox& bounds) const
{
	if (!m_pViewVolumeCollider)
		return DirectX::DISJOINT;

	switch (m_pViewVolumeCollider->GetCollType())
	{
	case CollType::Frustum:
		return static_cast<const CCollFrustum*>(m_pViewVolumeCollider.get())
			->GetBoundingFrustum().Contains(bounds);
	case CollType::OrientedBox:
		return static_cast<const CCollOrientedBox*>(m_pViewVolumeCollider.get())
			->GetBoundingOrientedBox().Contains(bounds);
	default:
		return DirectX::DISJOINT;
	}
}

_bool CCameraObject::IntersectsViewVolume(const BoundingBox& bounds) const
{
	return ContainsViewVolume(bounds) != DirectX::DISJOINT;
}

const CCollFrustum* CCameraObject::GetFrustumCollider() const
{
	if (!m_pViewVolumeCollider || m_pViewVolumeCollider->GetCollType() != CollType::Frustum)
		return nullptr;

	return static_cast<const CCollFrustum*>(m_pViewVolumeCollider.get());
}

const CCollOrientedBox* CCameraObject::GetOrientedBoxCollider() const
{
	if (!m_pViewVolumeCollider || m_pViewVolumeCollider->GetCollType() != CollType::OrientedBox)
		return nullptr;

	return static_cast<const CCollOrientedBox*>(m_pViewVolumeCollider.get());
}
