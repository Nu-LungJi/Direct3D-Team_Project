#include "pch.h"
#include "BTDecInvert.h" 
NS_USING(Client)

CBTDecInvert::CBTDecInvert()
{

}

CBTDecInvert::CBTDecInvert(const CBTDecInvert& rhs) : CBTDecorator(rhs)
{

}
CBTDecInvert::~CBTDecInvert()
{
}
HRESULT CBTDecInvert::InitalizePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecInvert";
	return S_OK;
}
HRESULT CBTDecInvert::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDecInvert::Evaluate(_float fTimeDelta)
{
	EVALUATE eType = __super::Evaluate(fTimeDelta);
	if (eType == EVALUATE::SUCCESS)
		eType = EVALUATE::FAILED;
	else if (eType == EVALUATE::FAILED)
		eType = EVALUATE::SUCCESS;

	return m_eDebug = eType;
}

void		CBTDecInvert::Update_Gui()
{
}
E::UPtr<CBTDecInvert> CBTDecInvert::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecInvert{});
	if (FAILED(pInstance->InitalizePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecInvert");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecInvert::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecInvert{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecInvert");
		return nullptr;
	}

	return pInstance;
}
