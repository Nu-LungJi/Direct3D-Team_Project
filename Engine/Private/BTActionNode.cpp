#include "pch.h"
#include "BTActionNode.h"
#include "BTDecorator.h"

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
	SaveJsonValue(j,"ActionSpeed",m_Value.fSpeed);
	SaveJsonValue(j,"ActionTimeTick",m_Value.fTick);
	SaveJsonValue(j,"ActionMaxTime",m_Value.fTime);
	SaveJsonValue(j,"ActionAnimIndex",m_Value.iAnimIndex);
	return j;
}

HRESULT CBTActionNode::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "ActionSpeed", m_Value.fSpeed);
	LoadJsonValue(j, "ActionTimeTick", m_Value.fTick);
	LoadJsonValue(j, "ActionMaxTime", m_Value.fTime);
	LoadJsonValue(j, "ActionAnimIndex", m_Value.iAnimIndex);
	return S_OK;
}

