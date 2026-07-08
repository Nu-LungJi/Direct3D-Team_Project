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

HRESULT CBTSelector::InitalizePrototype(void* pArg)
{
    __super::InitalizePrototype(pArg);
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
    int32_t iIndex = 0;
    if (m_NodeValue.bCur)
        iIndex = m_NodeValue.iPreSecquenceIndex;

    for (size_t i = iIndex; i < m_Actions.size(); ++i)
    {
        if (nullptr == m_Actions[i])
            continue;

        EVALUATE eValuate = m_Actions[i]->Evaluate(fTimeDelta);
        if (eValuate == EVALUATE::SUCCESS)
        {
        }
        else if (eValuate == EVALUATE::RUN)
        {
            m_NodeValue.bCur = true;
            m_NodeValue.iPreSecquenceIndex = i;
            return  EVALUATE::RUN;
        }
        else if (eValuate == EVALUATE::FAILED)
        {
            m_NodeValue.bCur = false;
            return EVALUATE::FAILED;
        }

    }
	return EVALUATE::SUCCESS;
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
    if (FAILED(pInstance->InitalizePrototype(pArg)))
    {
        MSG_BOX("Failed to Created : CBTSelector");
        return nullptr;
    }
    return pInstance;
}

E::UPtr<E::CBTRoot> CBTSelector::Clone(void* pArg)
{
    auto	pInstance = E::ToUPtr(new CBTSelector{ *this });
    if (FAILED(pInstance->Initalize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CBTSelector");
        return nullptr;
    }

    return pInstance;
}

