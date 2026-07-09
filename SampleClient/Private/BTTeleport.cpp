#include "pch.h"
#include "BTTeleport.h"
#include "ComTransform.h" 
NS_USING(Client)

CBTTeleport::CBTTeleport()
{

}
CBTTeleport::CBTTeleport(const CBTTeleport& rhs) : CBTActionNode(rhs)
{

}

CBTTeleport::~CBTTeleport()
{
}
HRESULT CBTTeleport::InitalizePrototype(void* pArg)
{
	__super::InitalizePrototype(pArg);

	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTOnlyTrue";
	return S_OK;
}
HRESULT CBTTeleport::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}


EVALUATE CBTTeleport::Evaluate(_float fTimeDelta)
{
	auto& vDest = CGameInstance::Get().GetActiveCamera()->GetTransform();
	auto pTransform = Cast<CComTransform>(Get_Component<CComTransform>(m_Handle, "Com_Transform"));

	_vector vDestPos = XMLoadFloat3(&vDest.GetPosition());
	pTransform->SetPosition(XMVectorSetW(vDestPos,1.f));
	
	return EVALUATE::SUCCESS;
}
void CBTTeleport::Update_Gui()
{
}
E::UPtr<CBTTeleport> CBTTeleport::Create()
{
	auto pInstance = E::ToUPtr(new CBTTeleport{});
	if (FAILED(pInstance->InitalizePrototype()))
	{
		MSG_BOX("Failed to Created : CBTTeleport");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CBTRoot> CBTTeleport::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTTeleport{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTTeleport");
		return nullptr;
	}

	return pInstance;
}
