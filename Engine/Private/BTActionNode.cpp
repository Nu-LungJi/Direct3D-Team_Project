#include "pch.h"
#include "BTActionNode.h"

CBTActionNode::CBTActionNode()
{
}

CBTActionNode::CBTActionNode(const CBTActionNode& Prototype) : CBTRoot(Prototype)
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

