#include "pch.h"

#include "Component.h"
NS_USING(Engine)

CComponent::CComponent()
{
}

CComponent::CComponent(const CComponent& Prototype)
{
}

CComponent::~CComponent()
{
}

HRESULT CComponent::Initialize(void* pArg)
{
    m_pGameObject = static_cast<const DESC*>(pArg)->pGameObject;
    return S_OK;
}

void CComponent::Free()
{
	m_pGameObject = nullptr;
	CPrototype::Free();
}

void CComponent::UpdateGUI()
{
}
