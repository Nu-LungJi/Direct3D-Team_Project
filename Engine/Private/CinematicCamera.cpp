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
