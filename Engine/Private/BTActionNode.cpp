#include "pch.h"
#include "BTActionNode.h"

CBTActionNode::CBTActionNode()
{
}

CBTActionNode::CBTActionNode(const CBTActionNode& pPrototype) : CBTRoot(pPrototype)
{
}


CBTActionNode::~CBTActionNode()
{
}


HRESULT CBTActionNode::InitalizePrototype(void* pArg)
{
	__super::InitalizePrototype(pArg);

	return S_OK;
}

HRESULT CBTActionNode::Initalize(void* pArg)
{
    auto pDesc = static_cast<ACTION_NODE_DESC*>(pArg);
	if(nullptr != pDesc)
		m_Value = pDesc->Value;
    __super::Initalize(pArg);
    
    return S_OK;
}

void CBTActionNode::Update_Gui()
{
}



nlohmann::json CBTActionNode::Save_Node()
{
	nlohmann::json j;
	j = __super::Save_Node();
	j["Action_Value"] = m_Value;
	return j;
}

HRESULT CBTActionNode::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	j["Action_Value"].get_to<ACTION_VALUE>(m_Value);

	return S_OK;
}

