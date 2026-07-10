#include "pch.h"
#include "ComBeHavior.h"
#include "BTDecorator.h"
#include "BTComposite.h"
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
    BtRoot.NodeName = "Root";
    BtRoot.m_GuiNode = (GUINODE(BEHAVIOR::SELECTOR, m_iNodeID, BtRoot.NodeName.c_str(), _float2(40, 50), 0.5f, _float4(0.5f, 0.5f, 0.5f, 1)));
    BtRoot.m_GuiLink = (GUINODE_LINK(2));
   
    m_Root = std::move(CBTComposite::Create(&BtRoot));
    m_NodeMap[m_iNodeID++] = m_Root.get();
    return S_OK;
}
void CComBeHavior::Set_NodeInfo(CBTRoot* pNode)
{
    BEHAVIOR eType = pNode->Get_GuiNodeInfo().eMyType;
    pNode->Set_Handle(GetGameObject()->GetHandle());
	uint32_t iMax = 0;
	m_iNodeID = std::max(m_iNodeID,pNode->Get_GuiNodeInfo().iID);
    RegistNode(pNode->Get_GuiNodeInfo().iID, pNode);

    if (eType == BEHAVIOR::SECQUNCE || eType == BEHAVIOR::SELECTOR)
    {
        auto& pSrc = (*static_cast<CBTComposite*>(pNode)->Get_Nodes());
        for (auto& iter : pSrc)
        {
			if(iter != nullptr)
            Set_NodeInfo(iter.get());
        }
    }
    else if (eType == BEHAVIOR::DECORATOR)
    {
        auto pSrc = static_cast<CBTDecorator*>(pNode)->Get_Child().get();
		if(pSrc != nullptr)
        Set_NodeInfo(pSrc);
    }
}
void CComBeHavior::Save_Data(const _string& filePath)
{
    nlohmann::json j;

    j = m_Root->Save_Node();

    std::ofstream path(filePath);
    path << j.dump(4);
    path.close();

    return;
}
HRESULT CComBeHavior::Load_Data(const _string& filePath)
{
	m_Root->Get_Nodes()->clear();
	m_iNodeID = 0;
	m_NodeMap.clear();
    nlohmann::json j;
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        MSG_BOX("Node Data Load Failed");
        return E_FAIL;
    }
    file >> j;
    m_Root->Load_json(j);
    file.close();

    Set_NodeInfo(m_Root.get());
	++m_iNodeID;
    return S_OK;
}
CBTRoot* CComBeHavior::Find_Node(const uint32_t& iNode)
{
    auto iter = m_NodeMap.find(iNode);
   
    if (iter != m_NodeMap.end())
        return iter->second;

    return nullptr;
}

CBTComposite* CComBeHavior::Get_Selector()
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
	if (nullptr != m_Root)
	{
		m_Root->ResetDebug();
		m_Root->Tick(fTimeDelta);
	}

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
