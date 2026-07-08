#include "pch.h"
#include "BTTurnSlow.h"
#include "ComTransform.h" 
NS_USING(Client)

CBTTurnSlow::CBTTurnSlow()
{

}

CBTTurnSlow::CBTTurnSlow(const CBTTurnSlow& rhs) : CBTActionNode(rhs)
{
}
CBTTurnSlow::~CBTTurnSlow()
{
}
HRESULT CBTTurnSlow::InitalizePrototype(void* pArg)
{
	__super::InitalizePrototype(pArg);
	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTTurnSlow";
	return S_OK;
}
HRESULT CBTTurnSlow::Initalize(void* pArg)
{
	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTTurnSlow::Evaluate(_float fTimeDelta)
{
	auto pTransform = Cast<CComTransform>(Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	auto& vDest = CGameInstance::Get().GetActiveCamera()->GetTransform();
	if (pTransform == nullptr)
		return EVALUATE::FAILED;
	_float3 vSrc = pTransform->GetPosition();
	_float3 fScale = pTransform->GetScale();
	
	m_Value.fTick += fTimeDelta;
	_float t = m_Value.fTick / m_Value.fTime;
	_vector vLook  = XMVectorLerp(XMVector3Normalize(pTransform->GetState(STATE::LOOK)), 
								  XMVector3Normalize(vDest.GetState(STATE::LOOK)),t);
	_vector vRight = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), vLook));
	_vector vUp = XMVector3Cross(vLook, vRight);
	vLook = XMVector3Cross(vRight, vUp);

	pTransform->SetState(STATE::RIGHT, vRight * fScale.x);
	pTransform->SetState(STATE::UP, vUp* fScale.y);
	pTransform->SetState(STATE::LOOK, vLook * fScale.z);
	if (t <= 1.f)
		return EVALUATE::RUN;

	m_Value.fTick = 0.f;
	return EVALUATE::SUCCESS;
}
void CBTTurnSlow::Update_Gui()
{
	ImGui::Text("TickTime : %2.f",&m_Value.fTick);
	ImGui::DragFloat("##Tick", &m_Value.fTime, 0, 100);
}
E::UPtr<CBTTurnSlow> CBTTurnSlow::Create()
{
	auto pInstance = E::ToUPtr(new CBTTurnSlow{});
	if (FAILED(pInstance->InitalizePrototype()))
	{
		MSG_BOX("Failed to Created : CBTTurnSlow");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CBTRoot> CBTTurnSlow::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTTurnSlow{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTTurnSlow");
		return nullptr;
	}

	return pInstance;
}
