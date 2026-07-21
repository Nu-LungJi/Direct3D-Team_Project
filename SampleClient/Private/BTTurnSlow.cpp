#include "pch.h"
#include "BTTurnSlow.h"
#include "ComTransform.h" 
#include "ComCharacterMoveIntent.h"
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
HRESULT CBTTurnSlow::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
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
	auto pMoveIntent = Get_Component<CComCharacterMoveIntent>(m_Handle, "ComCharacterMoveIntent");
	auto* pTarget = CGameInstance::Get().GetActiveCamera();
	if (pTransform == nullptr || pMoveIntent == nullptr || pTarget == nullptr)
		return m_eDebug = EVALUATE::FAILED;

	_float3 vFacingDirection{};
	XMStoreFloat3(&vFacingDirection,
		pTarget->GetTransform().GetState(STATE::POSITION) -
		pTransform->GetState(STATE::POSITION));
	const _float fTurnTime = std::max(m_Value.fTime, 0.001f);
	pMoveIntent->SetFacingIntent(vFacingDirection, 180.f / fTurnTime);
	
	return m_eDebug = EVALUATE::SUCCESS;
}
void CBTTurnSlow::Update_Gui()
{
	ImGui::Text("TickTime : %2.f",&m_Value.fTick);
	ImGui::DragFloat("##Tick", &m_Value.fTime, 0, 100);
}
E::UPtr<CBTTurnSlow> CBTTurnSlow::Create()
{
	auto pInstance = E::ToUPtr(new CBTTurnSlow{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTTurnSlow");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTTurnSlow::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTTurnSlow{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTTurnSlow");
		return nullptr;
	}

	return pInstance;
}
