#include "pch.h"
#include "ShopNpc.h"
#include "UIManager.h"

NS_USING(Client)

CShopNpc::CShopNpc(const CShopNpc& prototype)
	: CInteractiveNpc(prototype)
{
}

HRESULT CShopNpc::Initialize(void* pArg)
{
	if (!pArg)
		return E_INVALIDARG;

	auto* pDesc = static_cast<DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	m_bWorldSpaceShop = pDesc->WorldSpaceShop;
	m_vShopPanelPositionOffset = pDesc->ShopPanelPositionOffset;
	m_vShopPanelRotationOffsetDegrees =
		pDesc->ShopPanelRotationOffsetDegrees;
	return S_OK;
}

void CShopNpc::OpenShop()
{
	auto* pUIManager = GET_SINGLE(UIManager);
	if (m_bWorldSpaceShop)
	{
		pUIManager->OpenWandShopWorld(
			GetHandle(),
			m_vShopPanelPositionOffset,
			m_vShopPanelRotationOffsetDegrees);
		return;
	}

	pUIManager->OpenWandShop();
}

E::UPtr<CShopNpc> CShopNpc::Create()
{
	auto pInstance = E::ToUPtr(new CShopNpc{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CShopNpc");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CShopNpc::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CShopNpc{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CShopNpc");
		return nullptr;
	}
	return pInstance;
}
