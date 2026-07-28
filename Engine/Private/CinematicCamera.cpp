#include "pch.h"
#include "CinematicCamera.h"

NS_USING(Engine)

CCinematicCamera::CCinematicCamera()
{
}

CCinematicCamera::CCinematicCamera(const CCinematicCamera& Prototype)
	: CCameraObject{ Prototype }
{
}

CCinematicCamera::~CCinematicCamera()
{
}

HRESULT CCinematicCamera::InitializePrototype(void* pArg)
{
	return CCameraObject::InitializePrototype(pArg);
}

HRESULT CCinematicCamera::Initialize(void* pArg)
{
	if (FAILED(CCameraObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

	return S_OK;
}

void CCinematicCamera::LateUpdate(_float fTimeDelta)
{
	GetTransform().Update();
	CCameraObject::UpdateViewMatrix();
}

HRESULT CCinematicCamera::ApplyPose(const _float3& vPosition, const _float4& vRotation, _float fFovY)
{
	if (!std::isfinite(vPosition.x) ||
		!std::isfinite(vPosition.y) ||
		!std::isfinite(vPosition.z) ||
		!std::isfinite(vRotation.x) ||
		!std::isfinite(vRotation.y) ||
		!std::isfinite(vRotation.z) ||
		!std::isfinite(vRotation.w) ||
		!std::isfinite(fFovY) ||
		fFovY <= 0.f ||
		fFovY >= 180.f)
	{
		return E_INVALIDARG;
	}

	_float fRotationLengthSq{};
	XMStoreFloat(&fRotationLengthSq, XMVector4LengthSq(XMLoadFloat4(&vRotation)));
	if (fRotationLengthSq <= FLT_EPSILON)
	{
		return E_INVALIDARG;
	}

	auto& Transform = GetTransform();
	Transform.SetPosition(vPosition);
	Transform.SetQuaternion(vRotation);

	if (std::abs(m_cameraDesc.fFovY - fFovY) > FLT_EPSILON)
	{
		m_cameraDesc.fFovY = fFovY;
		return UpdateProjMatrix();
	}

	return S_OK;
}

UPtr<CPrototype> CCinematicCamera::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CCinematicCamera{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned: CCinematicCamera");
		return nullptr;
	}

	return pInstance;
}

Engine::UPtr<CCinematicCamera> CCinematicCamera::Create()
{
	auto pInstance = ToUPtr(new CCinematicCamera{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: CCinematicCamera");
		return nullptr;
	}

	return pInstance;
}
