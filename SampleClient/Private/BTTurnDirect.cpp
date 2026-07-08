#include "pch.h"
#include "BTTurnDirect.h"
#include "ComTransform.h" 
NS_USING(Client)

CBTTurnDirect::CBTTurnDirect()
{

}

CBTTurnDirect::CBTTurnDirect(const CBTTurnDirect& rhs) : CBTActionNode(rhs)
{

}
CBTTurnDirect::~CBTTurnDirect()
{
}
HRESULT CBTTurnDirect::InitalizePrototype(void* pArg)
{
	__super::InitalizePrototype(pArg);
	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTTurnDirect";
	return S_OK;
}
HRESULT CBTTurnDirect::Initalize(void* pArg)
{
	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTTurnDirect::Evaluate(_float fTimeDelta)
{
	auto pTransform = Cast<CComTransform>(Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	auto& vDest = CGameInstance::Get().GetActiveCamera()->GetTransform();
	if (pTransform == nullptr)
		return EVALUATE::FAILED;

	_float3 fScale = pTransform->GetScale();

	_vector vLook = XMVector3Normalize(pTransform->GetState(STATE::POSITION))-(vDest.GetState(STATE::POSITION));
	_vector vRight = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), vLook));
	_vector vUp = XMVector3Cross(vLook, vRight);
	vLook = XMVector3Cross(vRight, vUp);

	pTransform->SetState(STATE::RIGHT, vRight * fScale.x);
	pTransform->SetState(STATE::UP, vUp * fScale.y);
	pTransform->SetState(STATE::LOOK, vLook * fScale.z);

	return EVALUATE::SUCCESS;
}
void CBTTurnDirect::Update_Gui()
{
	ImGui::Text("TickTime %2.f : ");
	ImGui::DragFloat("##Tick", &m_Value.fTime, 0, 100);
}
E::UPtr<CBTTurnDirect> CBTTurnDirect::Create()
{
	auto pInstance = E::ToUPtr(new CBTTurnDirect{});
	if (FAILED(pInstance->InitalizePrototype()))
	{
		MSG_BOX("Failed to Created : CBTTurnDirect");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CBTRoot> CBTTurnDirect::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTTurnDirect{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTTurnDirect");
		return nullptr;
	}

	return pInstance;
}
