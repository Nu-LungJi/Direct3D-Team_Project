#include "pch.h"
#include "BTComposite.h"

CBTComposite::CBTComposite()
{

}
CBTComposite::~CBTComposite()
{

}
HRESULT CBTComposite::Find_Node(const _string& tagSecqunce)
{
	auto iter = m_NodeHandles.find(tagSecqunce);

	if (iter == m_NodeHandles.end())
		return E_FAIL;

	return S_OK;
}
HRESULT CBTComposite::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	return S_OK;
}
