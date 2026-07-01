#include "pch.h"
#include "BTActionNode.h"

CBTActionNode::CBTActionNode()
{
}


CBTActionNode::~CBTActionNode()
{
}


HRESULT CBTActionNode::Initalize(void* pArg)
{
    auto pDesc = static_cast<ACTION_NODE_DESC*>(pArg);

    __super::Initalize(pArg);
    
    return S_OK;
}

