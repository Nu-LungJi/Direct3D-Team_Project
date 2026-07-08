#include "pch.h"
#include "BTDecLier.h" 
NS_USING(Client)

CBTDecLier::CBTDecLier()
{

}

CBTDecLier::CBTDecLier(const CBTDecLier& rhs) : CBTDecorator(rhs)
{

}
CBTDecLier::~CBTDecLier()
{
}
HRESULT CBTDecLier::InitalizePrototype(void* pArg)
{
	__super::InitalizePrototype(pArg);
	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecLier";
	return S_OK;
}
HRESULT CBTDecLier::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDecLier::Evaluate(_float fTimeDelta)
{
	if (m_bEnter)
		return EVALUATE::SUCCESS;

	EVALUATE eType = __super::Evaluate(fTimeDelta);
	if (eType == EVALUATE::SUCCESS)
		m_bEnter = true;
	
	return eType;
}

void		CBTDecLier::Update_Gui()
{
}
E::UPtr<CBTDecLier> CBTDecLier::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecLier{});
	if (FAILED(pInstance->InitalizePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecLier");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CBTRoot> CBTDecLier::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecLier{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecLier");
		return nullptr;
	}

	return pInstance;
}
