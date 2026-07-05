#include "pch.h"
#include "BTSecqunce.h"

CBTSecqunce::CBTSecqunce()
{
}


CBTSecqunce ::~CBTSecqunce()
{
}


HRESULT CBTSecqunce::Initalize(void* pArg)
{
    if (FAILED(__super::Initalize(pArg)))
        return E_FAIL;

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

    for (size_t i = iIndex; i < m_Actions.size();++i)
    {
        if (nullptr == m_Actions[i])
            continue;

        EVALUATE eValuate = m_Actions[i]->Evaluate(fTimeDelta);
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
            
        }

    }
    return S_OK;
}

HRESULT CBTSecqunce::Late_Update(_float fTimeDelta)
{

   
    return S_OK;
}

EVALUATE CBTSecqunce::Evaluate(_float fTimeDelta)
{
    EVALUATE e{};
	return e;
}

nlohmann::json CBTSecqunce::Save_Node()
{
    return nlohmann::json();
}

HRESULT CBTSecqunce::Load_json(nlohmann::json& j)
{
    return E_NOTIMPL;
}

UPtr<CBTSecqunce> CBTSecqunce::Create(void* pArg)
{
    auto pInstance =ToUPtr(new CBTSecqunce());
    if (FAILED(pInstance->Initalize(pArg)))
    {
        MSG_BOX("Failed to Created : CBTSecqunce");
        return nullptr;
    }
    return pInstance;
}

