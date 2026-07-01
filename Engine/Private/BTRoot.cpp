#include "pch.h"
#include "BTRoot.h"

CBTRoot::CBTRoot()
{
}

CBTRoot::~CBTRoot()
{
}

HRESULT CBTRoot::Initalize(void* pArg)
{
	auto pDesc = static_cast<BTROOT_DESC*>(pArg);

	m_NodeName = pDesc->NodeName;

	return S_OK;
}


