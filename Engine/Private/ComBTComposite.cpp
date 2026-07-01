#include "pch.h"
#include "ComBTComposite.h"


HRESULT ComBTComposite::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	return S_OK;
}

HRESULT ComBTComposite::Add_Node(const StringID& svGroupTag, const StringID& svPrototypetag, void* pArg)
{
	auto tmp = ToSPtr(CGameInstance::Get().ClonePrototype(svGroupTag, svPrototypetag, pArg).get());
	;
	if (!tmp->IsA(ComBTRoot::StaticType))
	{
		return E_FAIL;
	}
	m_Actions.push_back(std::static_pointer_cast<ComBTRoot>(tmp));
}
