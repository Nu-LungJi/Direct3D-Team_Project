#include "pch.h"
#include "BTSecqunce.h"

CBTSecqunce::CBTSecqunce()
{
}


CBTSecqunce::CBTSecqunce(const CBTSecqunce& rhs) : CBTComposite(rhs)
{
}
CBTSecqunce ::~CBTSecqunce()
{
}

HRESULT CBTSecqunce::InitializePrototype(void* pArg)
{
    m_MasterName = "BTSequnce";
    m_eGroup = NODEGROUP::SEQUENCE;
    return S_OK;
}

HRESULT CBTSecqunce::Initalize(void* pArg)
{
    if (FAILED(__super::Initalize(pArg)))
        return E_FAIL;

	return S_OK;
}
EVALUATE CBTSecqunce::Evaluate(_float fTimeDelta)
{
    int32_t iIndex = 0;

    if (m_NodeValue.bCur)
        iIndex = m_NodeValue.iPreSecquenceIndex;

    for (size_t i = iIndex; i < m_Actions.size(); ++i)
    {
        if (nullptr == m_Actions[i])
            continue;

        EVALUATE eValuate = m_Actions[i]->Evaluate(fTimeDelta);
		if (eValuate == EVALUATE::RUN)
        {
            m_NodeValue.bCur = true;
            m_NodeValue.iPreSecquenceIndex = i;
			m_eDebug = EVALUATE::RUN;
            return EVALUATE::RUN;
        }
        else if (eValuate == EVALUATE::FAILED)
        {
			m_NodeValue.bCur = false;
			m_NodeValue.iPreSecquenceIndex = 0;
			m_eDebug = EVALUATE::FAILED;
			return EVALUATE::FAILED;
        }

    }
	m_NodeValue.bCur = false;
	m_NodeValue.iPreSecquenceIndex = 0;
	m_eDebug = EVALUATE::SUCCESS;
    return EVALUATE::SUCCESS;
}

nlohmann::json  CBTSecqunce::Save_Node()
{
    return __super::Save_Node();
}

HRESULT CBTSecqunce::Load_json(const nlohmann::json& j)
{
    return __super::Load_json(j);
}

UPtr<CBTSecqunce> CBTSecqunce::Create(void* pArg)
{
    auto pInstance =ToUPtr(new CBTSecqunce());
    if (FAILED(pInstance->InitializePrototype(pArg)))
    {
        MSG_BOX("Failed to Created : CBTSecqunce");
        return nullptr;
    }
    return pInstance;
}


E::UPtr<E::CPrototype> CBTSecqunce::Clone(void* pArg)
{
    auto	pInstance = E::ToUPtr(new CBTSecqunce{ *this });
    if (FAILED(pInstance->Initalize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CBTSecqunce");
        return nullptr;
    }

    return pInstance;
}
