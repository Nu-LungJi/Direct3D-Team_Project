#include "pch.h"
#include "BTOnlyTrue.h"
#include "ComTransform.h" 
NS_USING(Client)

CBTOnlyTrue::CBTOnlyTrue()
{

}
CBTOnlyTrue::CBTOnlyTrue(const CBTOnlyTrue& rhs) : CBTActionNode(rhs)
{

}

CBTOnlyTrue::~CBTOnlyTrue()
{
}
HRESULT CBTOnlyTrue::InitalizePrototype(void* pArg)
{
	__super::InitalizePrototype(pArg);

	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTOnlyTrue";
	return S_OK;
}
HRESULT CBTOnlyTrue::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}


EVALUATE CBTOnlyTrue::Evaluate(_float fTimeDelta)
{
	return EVALUATE::SUCCESS;
}
void CBTOnlyTrue::Update_Gui()
{
}
E::UPtr<CBTOnlyTrue> CBTOnlyTrue::Create()
{
	auto pInstance = E::ToUPtr(new CBTOnlyTrue{});
	if (FAILED(pInstance->InitalizePrototype()))
	{
		MSG_BOX("Failed to Created : CBTOnlyTrue");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CBTRoot> CBTOnlyTrue::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTOnlyTrue{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTOnlyTrue");
		return nullptr;
	}

	return pInstance;
}
