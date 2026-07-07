#include "pch.h"
#include "BTchase.h"
#include "ComTransform.h" 
NS_USING(Client)

CBTchase::CBTchase()
{

}

CBTchase::CBTchase(const CBTchase& rhs) : CBTActionNode(rhs)
{

}
CBTchase::~CBTchase()
{
}
HRESULT CBTchase::InitalizePrototype(void* pArg)
{
	__super::InitalizePrototype(pArg);
	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTchase";
	return S_OK;
}
HRESULT CBTchase::Initalize(void* pArg)
{
	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTchase::Evaluate(_float fTimeDelta)
{
	auto pTransform = Cast<CComTransform>(Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	if (pTransform == nullptr)
		return EVALUATE::FAILED;

	//pTransform->LookAt();

	return EVALUATE::FAILED;
}
void CBTchase::Update_Gui()
{
}
E::UPtr<CBTchase> CBTchase::Create()
{
	auto pInstance = E::ToUPtr(new CBTchase{});
	if (FAILED(pInstance->InitalizePrototype()))
	{
		MSG_BOX("Failed to Created : CBTchase");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CBTRoot> CBTchase::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTchase{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTchase");
		return nullptr;
	}

	return pInstance;
}
