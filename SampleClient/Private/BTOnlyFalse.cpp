#include "pch.h"
#include "BTOnlyFalse.h"
#include "ComTransform.h" 
NS_USING(Client)

CBTOnlyFalse::CBTOnlyFalse()
{

}
CBTOnlyFalse::CBTOnlyFalse(const CBTOnlyFalse& rhs) : CBTActionNode(rhs)
{

}

CBTOnlyFalse::~CBTOnlyFalse()
{
}
HRESULT CBTOnlyFalse::InitalizePrototype(void* pArg)
{
	__super::InitalizePrototype(pArg);

	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTOnlyFalse";
	return S_OK;
}
HRESULT CBTOnlyFalse::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}


EVALUATE CBTOnlyFalse::Evaluate(_float fTimeDelta)
{
	m_eDebug = EVALUATE::FAILED;
	return EVALUATE::FAILED;
}
void CBTOnlyFalse::Update_Gui()
{

}
E::UPtr<CBTOnlyFalse> CBTOnlyFalse::Create()
{
	auto pInstance = E::ToUPtr(new CBTOnlyFalse{});
	if (FAILED(pInstance->InitalizePrototype()))
	{
		MSG_BOX("Failed to Created : CBTOnlyFalse");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CBTRoot> CBTOnlyFalse::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTOnlyFalse{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTOnlyFalse");
		return nullptr;
	}

	return pInstance;
}
