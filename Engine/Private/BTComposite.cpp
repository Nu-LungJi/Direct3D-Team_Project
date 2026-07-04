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
	
	int32_t iIndex = m_Actions.size();

	if (pDesc->m_GuiNode.eMyType == BEHAVIOR::SELECTOR)
	{
		auto pSelector = CBTSelector::Create(pDesc);

		if (nullptr == pSelector) return E_FAIL;
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

    
    m_NodeHandles[pDesc->m_GuiNode.Name] = iIndex;
	return S_OK;
}

int32_t CBTComposite::Find_Node(const _string& tagSecqunce)
{
	auto iter = m_NodeHandles.find(tagSecqunce);

	if (iter == m_NodeHandles.end())
		return -1;


	return iter->second;
}

CBTRoot* CBTComposite::Find_AllNodePtr(const _string& strNodeName)
{
	CBTRoot* pSrc = nullptr;
	int32_t iIndex = Find_Node(strNodeName);
	if (-1 != iIndex)
		return Find_Src(iIndex);
	
	for (size_t i = 0; i < m_Actions.size(); ++i)
	{
		if (m_Actions[i]->Get_GuiNodeInfo().eMyType == BEHAVIOR::SELECTOR)
		{
			pSrc = static_cast<CBTSelector*>((m_Actions[i].get()))->Find_AllNodePtr(strNodeName);	
		}
		else if (m_Actions[i]->Get_GuiNodeInfo().eMyType == BEHAVIOR::SECQUNCE)
		{
			pSrc = static_cast<CBTSecqunce*>((m_Actions[i].get()))->Find_AllNodePtr(strNodeName);
		}
		if (pSrc != nullptr)
			return pSrc;
	}
	return nullptr;
}

HRESULT CBTComposite::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	return S_OK;
}
