#include "pch.h"
#include "BTchase.h"
#include "ComTransform.h" 
NS_USING(Client)

CBTchase::CBTchase()
{

}


CBTchase::~CBTchase()
{
}
HRESULT CBTchase::InitializePrototype()
{

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
E::UPtr<CBTchase> CBTchase::Create()
{
	auto pInstance = E::ToUPtr(new CBTchase{});
	if (FAILED(pInstance->InitializePrototype()))
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
