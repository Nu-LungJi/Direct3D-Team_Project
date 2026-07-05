#include "pch.h"
#include "BTDecorator.h"

CBTDecorator::CBTDecorator()
{
}

CBTDecorator::CBTDecorator(const CBTDecorator& Prototype)
{
}

CBTDecorator ::~CBTDecorator()
{
}


HRESULT CBTDecorator::Initalize(void* pArg)
{
    if (FAILED(__super::Initalize(pArg)))
        return E_FAIL;

    return S_OK;
}

HRESULT CBTDecorator::Priority_Update(_float fTimeDelta)
{

    return S_OK;
}

HRESULT CBTDecorator::Update(_float fTimeDelta)
{

    return S_OK;
}

HRESULT CBTDecorator::Late_Update(_float fTimeDelta)
{


    return S_OK;
}

EVALUATE CBTDecorator::Evaluate(_float fTimeDelta)
{

    int32_t iIndex = 0;

    if (m_NodeValue.bCur)
        iIndex = m_NodeValue.iCurSecquenceIndex;

    for (size_t i = iIndex; i < m_Actions.size(); ++i)
    {
        if (nullptr == m_Actions[i])
            continue;

        EVALUATE eValuate = m_Actions[i]->Evaluate(fTimeDelta);
        if (eValuate == EVALUATE::SUCCESS)
            return EVALUATE::SUCCESS;
        else if (eValuate == EVALUATE::RUN)
        {
            m_NodeValue.bCur = true;
            m_NodeValue.iPreSecquenceIndex = i;
            return EVALUATE::RUN;
        }
        else if (eValuate == EVALUATE::FAILED)
        {
            m_NodeValue.bCur = false;
            return EVALUATE::FAILED;

        }

    }
    return EVALUATE::FAILED;
}

nlohmann::json CBTDecorator::Save_Node()
{
    return nlohmann::json();
}

HRESULT CBTDecorator::Load_json(nlohmann::json& j)
{
    return E_NOTIMPL;
}

