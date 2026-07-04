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
    BtRoot.NodeName = "Main_Selector";
    BtRoot.m_GuiNode = (GUINODE(BEHAVIOR::SELECTOR, m_iNodeID++, "Main_Selector", _float2(40, 50), 0.5f, _float4(255, 100, 100, 1)));
    BtRoot.m_GuiLink = (GUINODE_LINK(2));
    m_Root = std::move(CBTSelector::Create(&BtRoot));
    return S_OK;
}
int32_t CComBeHavior::Find_Node(const _string& tagSecqunce)
{
    auto iter = m_NodeHandles.find(tagSecqunce);

    if (iter == m_NodeHandles.end())
        return -1;


    return iter->second;
}
int32_t CComBeHavior::Recursive_Call_Node(CBTRoot* pNode, int32_t* iIndex, const _string& NodeName)
{
    
    if (pNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::SELECTOR)
    {
        auto pSelector = static_cast<CBTSelector*>(pNode);
        
        for (size_t i = 0; (*pSelector->Get_Nodes()).size();++i)
        {
            if (-1 != (*pSelector).Find_Node(NodeName))
                return 0;

            Recursive_Call_Node(pSelector, iIndex, NodeName);
        }
    }else if (pNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::SECQUNCE)
    {
        auto pSequnce = static_cast<CBTSecqunce*>(pNode);

        for (size_t i = 0; (*pSequnce->Get_Nodes()).size(); ++i)
        {
            if (-1 != (*pSequnce).Find_Node(NodeName))
                return 0;
            Recursive_Call_Node(pSequnce, iIndex, NodeName);
        }
    }

    return -1;
}

HRESULT		CComBeHavior::Add_Node(void* pArg)
{
    auto pDesc = static_cast<CBTRoot::BTROOT_DESC*>(pArg);
    int32_t iIndex = Check_AllNode(pDesc->m_GuiNode.Name);
    if (iIndex == -1) return E_FAIL;

    if (FAILED(m_Root->Add_Node(pDesc)))
        return E_FAIL;

    m_NodeHandles[pDesc->m_GuiNode.Name] = iIndex;
}
int32_t CComBeHavior::Check_AllNode(const _string& NodeName)
{
    int32_t iIndex{-1};
    if (-1 != Find_Node(NodeName))
        return 0;

    auto Nodes = m_Root->Get_Nodes();
    if (Nodes->empty()) return 1;
    for (size_t i = 0; i < (*Nodes).size(); ++i)
    {
        if ((*Nodes)[i]->Get_GuiNodeInfo().eMyType == BEHAVIOR::SELECTOR)
        {
            auto pSecqunce = static_cast<CBTSelector*>((*Nodes)[i].get());
            if (-1 != pSecqunce->Find_Node(NodeName))
                return 0;
            Recursive_Call_Node(pSecqunce,&iIndex, NodeName);
        }
        else if ((*m_Root->Get_Nodes())[i]->Get_GuiNodeInfo().eMyType == BEHAVIOR::SECQUNCE)
        {
            auto pSecqunce = static_cast<CBTSecqunce*>((*Nodes)[i].get());
            if (-1 != pSecqunce->Find_Node(NodeName))
                return 0;

            Recursive_Call_Node(pSecqunce, &iIndex, NodeName);
        }
    }


    return -1;
}


CBTRoot* CComBeHavior::Get_Node(const _string& NodeName)
{
    CBTRoot* pSrc = nullptr;
    int32_t iIndex{ -1 };
    iIndex = Find_Node(NodeName);

    if (-1 != iIndex)
        return ((*m_Root->Get_Nodes())[iIndex]).get();
    
    for (size_t i = 0; i < (*m_Root->Get_Nodes()).size(); ++i)
    {

        if (((*m_Root->Get_Nodes())[i])->Get_GuiNodeInfo().eMyType == BEHAVIOR::SELECTOR)
        {
            pSrc = static_cast<CBTSelector*>((*m_Root->Get_Nodes())[i].get())->Find_AllNodePtr(NodeName);
            if (pSrc != nullptr)
                return pSrc;
        }else if(((*m_Root->Get_Nodes())[i])->Get_GuiNodeInfo().eMyType == BEHAVIOR::SECQUNCE)
        {
            pSrc = static_cast<CBTSecqunce*>((*m_Root->Get_Nodes())[i].get())->Find_AllNodePtr(NodeName);
            if (pSrc != nullptr)
                return pSrc;
        }
    
    }
    return nullptr;
}

CBTSelector* CComBeHavior::Get_Selector()
{
    return m_Root.get(); 
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