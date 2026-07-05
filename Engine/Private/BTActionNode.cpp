#include "pch.h"
#include "BTActionNode.h"

CBTActionNode::CBTActionNode()
{
}

CBTActionNode::CBTActionNode(const CBTActionNode& pPrototype)
{
}


CBTActionNode::~CBTActionNode()
{
}


HRESULT CBTActionNode::Initalize(void* pArg)
{
    auto pDesc = static_cast<ACTION_NODE_DESC*>(pArg);
    m_Value = pDesc->Value;
    __super::Initalize(pArg);
    
    return S_OK;
}

HRESULT CBTActionNode::Priority_Update(_float fTimeDelta)
{
    return S_OK;
}

HRESULT CBTActionNode::Update(_float fTimeDelta)
{
    return S_OK;
}

HRESULT CBTActionNode::Late_Update(_float fTimeDelta)
{
    return S_OK;
}

void CBTActionNode::Update_Gui()
{
	if (m_Value.eNodeType == NODE_ACTION::MOVE)
	{

	}
	else if (m_Value.eNodeType == NODE_ACTION::ANIMATION)
	{
		if (ImGui::Button("Animation"))
			m_bPopup = true;
		if (m_bPopup)
		{
			if (CGameInstance::Get().MouseDown(MOUSEKEYSTATE::RB))
				m_bPopup = false;
			int32_t iIndex = CGameInstance::Get().GetAnimIndex(m_Handle);

			if (-1 != iIndex)
			{
				m_bPopup = false;
				m_Value.iAnimIndex = iIndex;
			}
		}
	}

}

nlohmann::json CBTActionNode::Save_Node()
{
	return nlohmann::json();
}

HRESULT CBTActionNode::Load_json(nlohmann::json& j)
{
	return E_NOTIMPL;
}

