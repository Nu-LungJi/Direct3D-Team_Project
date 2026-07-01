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

    m_NodeName = "Main_Selector";
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

    for (size_t i = iIndex ; i < m_Actions.size();)
    {
        EVALUATE eValuate = m_Actions[i]->Evaluate();
        if (eValuate == EVALUATE::SUCCESS)
            return S_OK;
        else if (eValuate == EVALUATE::RUN)
        {
            m_NodeValue.bCur = true;
            m_NodeValue.iPreSecquenceIndex = i;
            return S_OK;
        }else if (eValuate == EVALUATE::FAILED)
        {
            m_NodeValue.bCur = false;
            ++i;
        }
         
    }
    return S_OK;
}

HRESULT CBTSelector::Late_Update(_float fTimeDelta)
{
   
    return S_OK;
}

HRESULT		CBTSelector::Add_Secqunce(const _string& strSequenceName)
{
    if (Find_Node(strSequenceName) == -1)
        return E_FAIL;

    int32_t iIndex = m_Actions.size();

    auto pSecqunce = CBTSecqunce::Create(strSequenceName);
    
    m_NodeHandles[strSequenceName] = iIndex;
    return E_FAIL;
}

HRESULT CBTSelector::Add_ActionNode(int32_t iSequenceIndex, UPtr<CBTRoot> pActionNode)
{
    auto pSequence = Cast<CBTSecqunce>(m_Actions[iSequenceIndex].get());
    
    pSequence->Add_ActioNode(move(pActionNode));
    
    return S_OK;
}

HRESULT CBTSelector::Add_Selector(const _string& strSelectoreName)
{
    return S_OK;
}

EVALUATE CBTSelector::Evaluate()
{
	return EVALUATE();
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

