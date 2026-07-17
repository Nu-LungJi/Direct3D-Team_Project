#include "pch.h"
#include "BTRandSelector.h"
#include "BTSecqunce.h"
CBTRandSelector::CBTRandSelector()
{
}

CBTRandSelector::CBTRandSelector(const CBTRandSelector& rhs) : CBTComposite(rhs)
{

}
CBTRandSelector::~CBTRandSelector()
{
}

HRESULT CBTRandSelector::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_MasterName = "BTRandSelector";
	m_eGroup = NODEGROUP::RAND_SELECTOR;
	return S_OK;
}

HRESULT CBTRandSelector::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	NODEGROUP eGroup = m_eGroup;
	
	m_GuiNode.vColor = _float4(0.5294f, 0.9843f, 1.f, 1.f);
	return S_OK;
}

EVALUATE CBTRandSelector::Evaluate(_float fTimeDelta)
{
	uint32_t iRand = rand() % m_Actions.size();

	if (m_NodeValue.bCur)
		iRand = m_NodeValue.iPreSecquenceIndex;

	if (iRand >= m_Actions.size())
		return EVALUATE::FAILED;

	if (m_Actions.empty() || nullptr == m_Actions[iRand])
		return EVALUATE::FAILED;

	EVALUATE eValuate = m_Actions[iRand]->Evaluate(fTimeDelta);
	if (eValuate == EVALUATE::SUCCESS)
	{
		m_NodeValue.bCur = false;
		return m_eDebug = EVALUATE::SUCCESS;
	}
	else if (eValuate == EVALUATE::RUN)
	{
		m_NodeValue.bCur = true;
		m_NodeValue.iPreSecquenceIndex = iRand;
		return m_eDebug = EVALUATE::RUN;
	}
	m_NodeValue.bCur = false;
	return m_eDebug = EVALUATE::FAILED;
}

void CBTRandSelector::Abort()
{
	for (size_t i = 0; i < m_Actions.size(); ++i)
	{
		if (nullptr != m_Actions[i])
			m_Actions[i]->Abort();
	}
}

nlohmann::json CBTRandSelector::Save_Node()
{
	return __super::Save_Node();
}

HRESULT CBTRandSelector::Load_json(const nlohmann::json& j)
{
	return __super::Load_json(j);
}

UPtr<CBTRandSelector> CBTRandSelector::Create(void* pArg)
{
	auto pInstance = ToUPtr(new CBTRandSelector());
	if (FAILED(pInstance->InitializePrototype(pArg)))
	{
		MSG_BOX("Failed to Created : CBTRandSelector");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CBTRandSelector::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTRandSelector{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTRandSelector");
		return nullptr;
	}

	return pInstance;
}

