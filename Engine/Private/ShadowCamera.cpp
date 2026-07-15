#include "pch.h"

#include "ShadowCamera.h"
#include "GameInstance.h"

CShadowCamera::CShadowCamera()
{
}

CShadowCamera::CShadowCamera(const CShadowCamera& Prototype)
	: CCameraObject{ Prototype }
{
}

CShadowCamera::~CShadowCamera()
{
}

void CShadowCamera::UpdateGUI()
{
	CCameraObject::UpdateGUI();
}

HRESULT CShadowCamera::Initialize(void* pArg)
{
	if (FAILED(CCameraObject::Initialize(pArg)))	return E_FAIL;

	return S_OK;
}

void CShadowCamera::PriorityUpdate(E::_float fTimeDelta)
{
}

void CShadowCamera::Update(E::_float fTimeDelta)
{
}

void CShadowCamera::LateUpdate(E::_float fTimeDelta)
{
	m_pComTransform->Update();
	CCameraObject::UpdateViewMatrix();
}

Engine::UPtr<CShadowCamera> CShadowCamera::Create() {
	auto pInstance = ToUPtr(new CShadowCamera{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: CShadowCamera");
		return nullptr;
	}

	return pInstance;
}
Engine::UPtr<CPrototype>	CShadowCamera::Clone(void* pArg) {
	auto pInstance = ToUPtr(new CShadowCamera{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned: CShadowCamera");
		return nullptr;
	}

	return pInstance;
}
