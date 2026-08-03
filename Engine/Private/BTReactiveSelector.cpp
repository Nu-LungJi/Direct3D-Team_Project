#include "pch.h"
#include "BTReactiveSelector.h"
#include "BTSecqunce.h"
CBTReactiveSelector::CBTReactiveSelector()
{
}

CBTReactiveSelector::CBTReactiveSelector(const CBTReactiveSelector& rhs) : CBTComposite(rhs)
{

}
CBTReactiveSelector::~CBTReactiveSelector()
{
}

HRESULT CBTReactiveSelector::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_MasterName = "BTReactiveSelector";
	m_eGroup = NODEGROUP::SELECTOR;


	return S_OK;
}

HRESULT CBTReactiveSelector::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	NODEGROUP eGroup = m_eGroup;

	m_GuiNode.vColor = _float4(0.5294f, 0.9843f, 1.f, 1.f);
	return S_OK;
}

void CBTReactiveSelector::OnEnter()
{
}

void CBTReactiveSelector::OnExit(EVALUATE eResult)
{
}

EVALUATE CBTReactiveSelector::Evaluate(_float fTimeDelta)
{
	int32_t iIndex = 0;

	for (size_t i = iIndex; i < m_Actions.size(); ++i)
	{
		if (nullptr == m_Actions[i])
			continue;

		EVALUATE eValuate = m_Actions[i]->Execute(fTimeDelta);
		if (eValuate == EVALUATE::SUCCESS)
		{
			if (m_iRunningIndex != -1 &&
				m_iRunningIndex != static_cast<int32_t>(i))
			{
				m_Actions[m_iRunningIndex]->AbortExecute();
			}

			m_iRunningIndex = -1;
			return m_eDebug = EVALUATE::SUCCESS;
		}
		else if (eValuate == EVALUATE::RUN)
		{
			if (m_iRunningIndex != -1 && m_iRunningIndex != static_cast<int32_t>(i))
			{
				m_Actions[m_iRunningIndex]->AbortExecute();
			}
			m_iRunningIndex = static_cast<int32_t>(i);
			return m_eDebug = EVALUATE::RUN;
		}

		if (m_iRunningIndex == static_cast<int32_t>(i))
			m_iRunningIndex = -1;
	}
	m_iRunningIndex= - 1;
	return m_eDebug = EVALUATE::FAILED;
}

void CBTReactiveSelector::Abort()
{
	if (m_iRunningIndex >= 0 &&static_cast<size_t>(m_iRunningIndex) < m_Actions.size() &&m_Actions[m_iRunningIndex])
		m_Actions[m_iRunningIndex]->AbortExecute();

	m_iRunningIndex = -1;

}

nlohmann::json CBTReactiveSelector::Save_Node()
{
	return __super::Save_Node();
}

HRESULT CBTReactiveSelector::Load_json(const nlohmann::json& j)
{
	return __super::Load_json(j);
}

UPtr<CBTReactiveSelector> CBTReactiveSelector::Create(void* pArg)
{
	auto pInstance = ToUPtr(new CBTReactiveSelector());
	if (FAILED(pInstance->InitializePrototype(pArg)))
	{
		MSG_BOX("Failed to Created : CBTReactiveSelector");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CBTReactiveSelector::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTReactiveSelector{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTReactiveSelector");
		return nullptr;
	}

	return pInstance;
}

