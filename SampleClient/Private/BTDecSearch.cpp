#include "pch.h"
#include "BTDecSearch.h" 
NS_USING(Client)

CBTDecSearch::CBTDecSearch()
{

}

CBTDecSearch::CBTDecSearch(const CBTDecSearch& rhs) : CBTDecorator(rhs)
{

}
CBTDecSearch::~CBTDecSearch()
{
}
HRESULT CBTDecSearch::InitalizePrototype(void* pArg)
{
	__super::InitalizePrototype(pArg);
	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecSearch";
	return S_OK;
}
HRESULT CBTDecSearch::Initalize(void* pArg)
{
	
	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDecSearch::Evaluate(_float fTimeDelta)
{
	//auto pTransform = Cast<CComTransform>(Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	//if (pTransform == nullptr)
	//	return EVALUATE::FAILED;
	//if()조건을 만족하면
	//__super::Evaluate(fTimeDelta);
	__super::Evaluate(fTimeDelta);
	return EVALUATE::FAILED;
}
void		CBTDecSearch::Update_Gui()
{

}
E::UPtr<CBTDecSearch> CBTDecSearch::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecSearch{});
	if (FAILED(pInstance->InitalizePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecSearch");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CBTRoot> CBTDecSearch::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecSearch{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecSearch");
		return nullptr;
	}

	return pInstance;
}
