#include "pch.h"
#include "BTSecqunce.h"

CBTSecqunce::CBTSecqunce()
{
}


CBTSecqunce ::~CBTSecqunce()
{
}


HRESULT CBTSecqunce::Initalize(const _string& strNodeName)
{
    m_NodeName = strNodeName;

	return S_OK;
}

HRESULT CBTSecqunce::Priority_Update(_float fTimeDelta)
{
 
    return S_OK;
}

HRESULT CBTSecqunce::Update(_float fTimeDelta)
{

    int32_t iIndex = 0;

    if (m_NodeValue.bCur)
        iIndex = m_NodeValue.iCurSecquenceIndex;

    for (size_t i = iIndex; i < m_Actions.size();)
    {
        EVALUATE eValuate = m_Actions[i]->Evaluate();
        if (eValuate == EVALUATE::SUCCESS)
            return S_OK;
        else if (eValuate == EVALUATE::RUN)
        {
            m_NodeValue.bCur = true;
            m_NodeValue.iPreSecquenceIndex = i;
            return S_OK;
        }
        else if (eValuate == EVALUATE::FAILED)
        {
            m_NodeValue.bCur = false;
            ++i;
        }

    }
    return S_OK;
}

HRESULT CBTSecqunce::Late_Update(_float fTimeDelta)
{

   
    return S_OK;
}

EVALUATE CBTSecqunce::Evaluate()
{
    EVALUATE e{};
	return e;
}


HRESULT CBTSecqunce::Add_ActioNode(UPtr<CBTRoot> pActionNode)
{
    _string tagName = pActionNode->Get_NodeName();
    if (Find_Node(tagName) == -1)
        return E_FAIL;

    int32_t iIndex = m_Actions.size();

    m_Actions.push_back(std::move(pActionNode));

    m_NodeHandles[tagName] = iIndex;

    return S_OK;
}

UPtr<CBTSecqunce> CBTSecqunce::Create(const _string& strNodeName)
{
    auto pInstance =ToUPtr(new CBTSecqunce());
    if (FAILED(pInstance->Initalize(strNodeName)))
    {
        MSG_BOX("Failed to Created : CBTSecqunce");
        return nullptr;
    }
    return pInstance;
}

