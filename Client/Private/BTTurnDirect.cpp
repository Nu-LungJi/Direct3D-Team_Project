#include "pch.h"
#include "BTTurnDirect.h"
#include "ComTransform.h" 
#include "ComCharacterMoveIntent.h"
#include "Monster.h"
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
HRESULT CBTTurnDirect::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
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
	auto pTransform (Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	auto pMoveIntent = Get_Component<CComCharacterMoveIntent>(m_Handle, "ComCharacterMoveIntent");
	
	if (auto pBT = Get_ComBT())
	{
		if (auto pOwner = static_cast<CMonster*>(pBT->GetGameObject()))
		{
			if (auto pTarget = pOwner->Get_Target())
			{
				if (pTransform == nullptr || pMoveIntent == nullptr || pTarget == nullptr)
						return m_eDebug = EVALUATE::FAILED;

				_float3 vFacingDirection{};
				XMStoreFloat3(&vFacingDirection,
					pTarget->GetTransform().GetState(STATE::POSITION) -
					pTransform->GetState(STATE::POSITION));
				pMoveIntent->SetFacingIntentImmediate(vFacingDirection);
			}

		}
	}
	return m_eDebug = EVALUATE::SUCCESS;
}
void CBTTurnDirect::Update_Gui()
{
	ImGui::Text("TickTime %2.f : ");
	ImGui::DragFloat("##Tick", &m_Value.fTime, 0, 100);
}
E::UPtr<CBTTurnDirect> CBTTurnDirect::Create()
{
	auto pInstance = E::ToUPtr(new CBTTurnDirect{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTTurnDirect");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTTurnDirect::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTTurnDirect{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTTurnDirect");
		return nullptr;
	}

	return pInstance;
}
