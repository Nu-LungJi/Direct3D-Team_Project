#include "pch.h"

#include "UICamera.h"

NS_USING(Engine)

CUICamera::CUICamera()
{
}

CUICamera::CUICamera(const CUICamera& Prototype)
{
}

CUICamera::~CUICamera()
{
}

HRESULT CUICamera::Initialize(void* pArg)
{
    if (FAILED(CCameraObject::Initialize(pArg)))
    {
        return E_FAIL;
    }

    return S_OK;
}

void CUICamera::PriorityUpdate(E::_float fTimeDelta)
{
}

void CUICamera::Update(E::_float fTimeDelta)
{
}

void CUICamera::LateUpdate(E::_float fTimeDelta)
{
    m_pComTransform->Update();

    CCameraObject::UpdateViewMatrix();
}


Engine::UPtr<CUICamera> CUICamera::Create()
{
    auto pInstance = ToUPtr(new CUICamera{});
    if (FAILED(pInstance->InitializePrototype()))
    {
        MSG_BOX("Failed to Create: CUICamera");
        return nullptr;
    }

    return pInstance;
}

Engine::UPtr<CPrototype> CUICamera::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CUICamera{ *this });
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned: CUICamera");
        return nullptr;
    }

    return pInstance;
}
