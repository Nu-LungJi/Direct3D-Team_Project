#include "pch.h"
#include "ComBeHavior.h"
#include "BTSelector.h"

CComBeHavior::CComBeHavior()
{
}

CComBeHavior::CComBeHavior(const CComBeHavior& Prototype) : CComponent(Prototype)
{
}

CComBeHavior::~CComBeHavior()
{
}

HRESULT CComBeHavior::InitializePrototype(void* pArg)
{
    return S_OK;
}

HRESULT CComBeHavior::Initalize(void* pArg)
{
    m_Root = std::move(CBTSelector::Create(pArg));
    return S_OK;
}
int32_t CComBeHavior::Find_Secquence(const _string& strSecquenceName)
{
    auto iter = m_NodeHandles.find(strSecquenceName);

    if (iter == m_NodeHandles.end())
        return -1;

    return iter->second;
}
HRESULT CComBeHavior::Add_Secqunce(const _string& strSecquenceName)
{
    if (Find_Secquence(strSecquenceName) == -1)
        return E_FAIL;

    uint32_t iIndex = m_NodeHandles.size();

    if(FAILED(m_Root.get()->Add_Secqunce(strSecquenceName)))
        return E_FAIL;

    m_NodeHandles[strSecquenceName] = iIndex;

}
HRESULT CComBeHavior::Add_SecqunceToNode(const _string& strSequenceName, UPtr<CBTRoot> pActionNode)
{
    int32_t iIndex = Find_Secquence(strSequenceName);
    if (iIndex == 1) return E_FAIL;

    if(FAILED(m_Root.get()->Add_ActionNode(iIndex, std::move(pActionNode))))
        return E_FAIL;

    return S_OK;
}


void CComBeHavior::Update(_float fTimeDelta)
{
    m_Root->Update(fTimeDelta);
}
UPtr<CComBeHavior> CComBeHavior::Create()
{
    auto pInstance = ToUPtr(new CComBeHavior{});
    if (FAILED(pInstance->InitializePrototype()))
    {
        MSG_BOX("Failed to Created : CComBeHavior");
        return nullptr;
    }
    return pInstance;
}

UPtr<CPrototype> CComBeHavior::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CComBeHavior{});
    if (FAILED(pInstance->Initalize(pArg)))
    {
        MSG_BOX("Failed to Created : CComBeHavior_Clone");
        return nullptr;
    }
    return pInstance;
}