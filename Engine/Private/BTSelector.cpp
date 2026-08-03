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
	m_GuiNode.vColor = _float4(0.5294f, 0.9843f, 1.f, 1.f);
	return S_OK;
}

void CBTSelector::OnEnter()
{
}

void CBTSelector::OnExit(EVALUATE eResult)
{
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

        EVALUATE eValuate = m_Actions[i]->Execute(fTimeDelta);
        if (eValuate == EVALUATE::SUCCESS)
        {
			m_NodeValue.bCur = false;
			m_NodeValue.iPreSecquenceIndex = 0;
			return m_eDebug  = EVALUATE::SUCCESS;
        }
        else if (eValuate == EVALUATE::RUN)
		{
			m_NodeValue.bCur = true;
			m_NodeValue.iPreSecquenceIndex = i;
            return m_eDebug =  EVALUATE::RUN;
        }
 

    }

	m_NodeValue.bCur = false;
	m_NodeValue.iPreSecquenceIndex = 0;
	return m_eDebug =  EVALUATE::FAILED;
}

void CBTSelector::Abort()
{

	if (m_NodeValue.iPreSecquenceIndex < m_Actions.size() &&
		m_Actions[m_NodeValue.iPreSecquenceIndex])
	{
		m_Actions[m_NodeValue.iPreSecquenceIndex]->AbortExecute();
	}

	m_NodeValue.bCur = false;
	m_NodeValue.iPreSecquenceIndex = 0;
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

