#include "pch.h"

#include "CameraObject.h"

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


	// build frustum far corner
	{
		float halfHegith = cameraDesc->fFar * tanf(0.5f * XMConvertToRadians(m_cameraDesc.fFovY));
		float halfWidth = cameraDesc->fAspect * halfHegith;

		m_FrustumFarCorner[0] = _float4{ -halfWidth, -halfHegith, cameraDesc->fFar, 0.f };
		m_FrustumFarCorner[1] = _float4{ -halfWidth, +halfHegith, cameraDesc->fFar, 0.f };
		m_FrustumFarCorner[2] = _float4{ +halfWidth, +halfHegith, cameraDesc->fFar, 0.f };
		m_FrustumFarCorner[3] = _float4{ +halfWidth, -halfHegith, cameraDesc->fFar, 0.f };
	}

	return S_OK;
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
	return S_OK;
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
	}
	else
	{
		return E_FAIL;
	}
	return S_OK;
}
