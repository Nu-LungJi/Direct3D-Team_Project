#include "pch.h"
#include "BTComposite.h"
#include "BTSelector.h"
#include "BTSecqunce.h"
#include "BTActionNode.h"
CBTComposite::CBTComposite()
{

}
CBTComposite::~CBTComposite()
{

}

HRESULT CBTComposite::Add_Node(void* pArg, UPtr<CBTRoot> pNode)
{
	auto pDesc = static_cast<CBTRoot::BTROOT_DESC*>(pArg);
	
	int32_t iIndex = {};

	if (pDesc->m_GuiNode.eMyType == BEHAVIOR::SELECTOR)
	{
		auto pSelector = CBTSelector::Create(pDesc);

		if (nullptr == pSelector) return E_FAIL;
		iIndex = pSelector->Get_GuiNodeInfo().iID;
		m_Actions.push_back(std::move(pSelector));
	}
	else if (pDesc->m_GuiNode.eMyType == BEHAVIOR::SECQUNCE)
	{
		auto pSecqunce = CBTSecqunce::Create(pDesc);

		if (nullptr == pSecqunce) return E_FAIL;
		m_Actions.push_back(std::move(pSecqunce));
	}
	else if (pDesc->m_GuiNode.eMyType == BEHAVIOR::ACTION)
	{
		if (nullptr == pNode) return E_FAIL;

		m_Actions.push_back(std::move(pNode));
	}
	else
		return E_FAIL;

	return S_OK;
}


HRESULT CBTComposite::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	m_Actions.resize(m_GuiLink.SlotEnd.size());
	return S_OK;
}
