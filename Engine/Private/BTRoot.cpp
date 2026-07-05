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

	m_Handle		= pDesc->Handle;
	m_GuiNode = std::move(pDesc->m_GuiNode);
	m_GuiLink = std::move(pDesc->m_GuiLink);
	return S_OK;
}



