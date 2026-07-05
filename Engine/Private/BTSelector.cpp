#include "pch.h"
#include "BTSelector.h"
#include "BTSecqunce.h"
CBTSelector::CBTSelector()
{
}


CBTSelector::~CBTSelector()
{
}


HRESULT CBTSelector::Initalize(void* pArg)
{
    __super::Initalize(pArg);

	return S_OK;
}

HRESULT CBTSelector::Priority_Update(_float fTimeDelta)
{
   
    return S_OK;
}

HRESULT CBTSelector::Update(_float fTimeDelta)
{
    int32_t iIndex = 0;
    
    if (m_NodeValue.bCur)
        iIndex = m_NodeValue.iCurSecquenceIndex;

    for (size_t i = iIndex ; i < m_Actions.size();++i)
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
            return S_OK;
        }else if (eValuate == EVALUATE::FAILED)
        {
            m_NodeValue.bCur = false;
          
        }
         
    }
    return S_OK;
}

HRESULT CBTSelector::Late_Update(_float fTimeDelta)
{
   
    return S_OK;
}

EVALUATE CBTSelector::Evaluate(_float fTimeDelta)
{
	return EVALUATE();
}

nlohmann::json CBTSelector::Save_Node()
{
    return nlohmann::json();
}

HRESULT CBTSelector::Load_json(nlohmann::json& j)
{
    return E_NOTIMPL;
}

UPtr<CBTSelector> CBTSelector::Create(void* pArg)
{
    auto pInstance = ToUPtr(new CBTSelector()) ;
    if (FAILED(pInstance->Initalize(pArg)))
    {
        MSG_BOX("Failed to Created : CBTSelector");
        return nullptr;
    }
    return pInstance;
}

