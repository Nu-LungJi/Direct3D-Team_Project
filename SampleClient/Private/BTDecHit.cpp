#include "pch.h"
#include "BTDecHit.h"
#include "ComBeHavior.h"
NS_USING(Client)

CBTDecHit::CBTDecHit()
{

}
CBTDecHit::CBTDecHit(const CBTDecHit& rhs) : CBTDecorator(rhs)
{

}

CBTDecHit::~CBTDecHit()
{
}
HRESULT CBTDecHit::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecHit";
	return S_OK;
}
HRESULT CBTDecHit::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDecHit::Evaluate(_float fTimeDelta)
{
	if (Check_Flag(ETOUI(BTFLAG::HIT)))
			return __super::Evaluate(fTimeDelta);
	
	return m_eDebug = EVALUATE::FAILED;
}
void CBTDecHit::Update_Gui()
{
}
E::UPtr<CBTDecHit> CBTDecHit::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecHit{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecHit");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecHit::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecHit{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecHit");
		return nullptr;
	}

	return pInstance;
}
