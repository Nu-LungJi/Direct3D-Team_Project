#include "pch.h"
#include "ComBeHavior.h"
#include "BTSelector.h"
#include "BTSecqunce.h"
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

HRESULT CComBeHavior::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;
    
    CBTRoot::BTROOT_DESC BtRoot{};
    BtRoot.Handle    = GetGameObject()->GetHandle();
    BtRoot.NodeName = "Main_Selector";
    BtRoot.m_GuiNode = (GUINODE(BEHAVIOR::SELECTOR, m_iNodeID, "Main_Selector", _float2(40, 50), 0.5f, _float4(255, 100, 100, 1)));
    BtRoot.m_GuiLink = (GUINODE_LINK(2));
   
    m_Root = std::move(CBTSelector::Create(&BtRoot));
    m_NodeMap[m_iNodeID++] = m_Root.get();
    return S_OK;
}
CBTRoot* CComBeHavior::Find_Node(const uint32_t& iNode)
{
    auto iter = m_NodeMap.find(iNode);
   
    if (iter != m_NodeMap.end())
        return iter->second;

    return nullptr;
}

CBTSelector* CComBeHavior::Get_Selector()
{
    return m_Root.get(); 
}


void CComBeHavior::RegistNode(uint32_t iIndex, CBTRoot* pNode)
{
    auto iter = m_NodeMap.find(iIndex);

    if (iter == m_NodeMap.end())
        m_NodeMap[iIndex] = pNode;
}

void CComBeHavior::UnRegistNode(uint32_t iIndex)
{
    auto iter = m_NodeMap.find(iIndex);

    if (iter != m_NodeMap.end())
        m_NodeMap.erase(iIndex);
}

void CComBeHavior::Update(_float fTimeDelta)
{
    m_Root->Update(fTimeDelta);
}
void CComBeHavior::UpdateGUI()
{
    CComponent::UpdateGUI();

    
    if (ImGui::Button("Open BTEditor"))
    {
        
        CGameInstance::Get().OpenBeHavior(GetGameObject()->GetHandle());
    }
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
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Created : CComBeHavior_Clone");
        return nullptr;
    }
    return pInstance;
}