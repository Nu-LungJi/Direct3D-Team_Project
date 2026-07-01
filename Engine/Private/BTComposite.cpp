#include "pch.h"
#include "BTComposite.h"

CBTComposite::CBTComposite()
{

}
CBTComposite::~CBTComposite()
{

}

int32_t CBTComposite::Find_Node(const _string& tagSecqunce)
{
	auto iter = m_NodeHandles.find(tagSecqunce);

	if (iter == m_NodeHandles.end())
		return -1;


	return iter->second;
}
HRESULT CBTComposite::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	return S_OK;
}
