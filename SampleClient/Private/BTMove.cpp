#include "pch.h"
#include "BTMove.h"
#include "ComTransform.h" 
NS_USING(Client)

CBTMove::CBTMove()
{

}


CBTMove::~CBTMove()
{
}
HRESULT CBTMove::InitializePrototype()
{

	return S_OK;
}
HRESULT CBTMove::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTMove::Evaluate(_float fTimeDelta)
{
	auto pTransform = Cast<CComTransform>(Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	if (pTransform == nullptr)
		return EVALUATE::FAILED;

	if (CGameInstance::Get().KeyPressing(DIK_RIGHT))
	{
		pTransform->GoRight(fTimeDelta);
		return EVALUATE::SUCCESS;
	}
	else if (CGameInstance::Get().KeyPressing(DIK_LEFT))
	{
		pTransform->GoLeft(fTimeDelta);
		return EVALUATE::SUCCESS;
	}else if (CGameInstance::Get().KeyPressing(DIK_UP))
	{
		pTransform->GoStraight(fTimeDelta);
		return EVALUATE::SUCCESS;
	}
	if (CGameInstance::Get().KeyPressing(DIK_DOWN))
	{
		pTransform->GoBackward(fTimeDelta);
		return EVALUATE::SUCCESS;
	}
		
	return EVALUATE::FAILED;
}
E::UPtr<CBTMove> CBTMove::Create()
{
	auto pInstance = E::ToUPtr(new CBTMove{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTMove");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CBTRoot> CBTMove::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTMove{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTMove");
		return nullptr;
	}

	return pInstance;
}
