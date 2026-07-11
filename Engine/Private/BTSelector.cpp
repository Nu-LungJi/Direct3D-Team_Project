#include "pch.h"
#include "BTSelector.h"
#include "BTSecqunce.h"
CBTSelector::CBTSelector()
{
}

CBTSelector::CBTSelector(const CBTSelector& rhs) : CBTComposite(rhs)
{

}
CBTSelector::~CBTSelector()
{
}

HRESULT CBTSelector::InitializePrototype(void* pArg)
{
    __super::InitializePrototype(pArg);
    m_MasterName = "BTSelector";
    m_eGroup = NODEGROUP::SELECTOR;
    return S_OK;
}

HRESULT CBTSelector::Initalize(void* pArg)
{
    __super::Initalize(pArg);
    NODEGROUP eGroup = m_eGroup;
	return S_OK;
}

EVALUATE CBTSelector::Evaluate(_float fTimeDelta)
{
    for (size_t i = 0; i < m_Actions.size(); ++i)
    {
        if (nullptr == m_Actions[i])
            continue;

        EVALUATE eValuate = m_Actions[i]->Evaluate(fTimeDelta);
        if (eValuate == EVALUATE::SUCCESS)
        {
			m_eDebug = EVALUATE::SUCCESS;
			return EVALUATE::SUCCESS;
        }
        else if (eValuate == EVALUATE::RUN)
		{
			m_NodeValue.iPreSecquenceIndex = i;
            return  EVALUATE::RUN;
        }
 

    }
	m_eDebug = EVALUATE::FAILED;
	return EVALUATE::FAILED;
}

void CBTSelector::Abort()
{
	for(size_t i=0; i< m_Actions.size(); ++i)
		m_Actions[i]->Abort();
}

nlohmann::json CBTSelector::Save_Node()
{
    return __super::Save_Node();
}

HRESULT CBTSelector::Load_json(const nlohmann::json& j)
{
    return __super::Load_json(j);
}

UPtr<CBTSelector> CBTSelector::Create(void* pArg)
{
    auto pInstance = ToUPtr(new CBTSelector()) ;
    if (FAILED(pInstance->InitializePrototype(pArg)))
    {
        MSG_BOX("Failed to Created : CBTSelector");
        return nullptr;
    }
    return pInstance;
}

E::UPtr<E::CPrototype> CBTSelector::Clone(void* pArg)
{
    auto	pInstance = E::ToUPtr(new CBTSelector{ *this });
    if (FAILED(pInstance->Initalize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CBTSelector");
        return nullptr;
    }

    return pInstance;
}

