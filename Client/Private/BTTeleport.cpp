#include "pch.h"
#include "BTTeleport.h"
#include "ComTransform.h" 
#include "ComCharacterMoveIntent.h"
#include "Monster.h"
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
HRESULT CBTTeleport::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTTeleport";
	return S_OK;
}
HRESULT CBTTeleport::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}


EVALUATE CBTTeleport::Evaluate(_float fTimeDelta)
{
	auto pMoveIntent = Get_Component<CComCharacterMoveIntent>(m_Handle, "ComCharacterMoveIntent");
	if (auto pBT = Get_ComBT())
	{
		if (auto pOwner = static_cast<CMonster*>(pBT->GetGameObject()))
		{
			if (auto pTarget = pOwner->Get_Target())
			{
				if (!pTarget || !pMoveIntent)
					return m_eDebug = EVALUATE::FAILED;

				pMoveIntent->RequestWarp(pTarget->GetTransform().GetPosition());
			}
		}
	}
	return m_eDebug = EVALUATE::SUCCESS;
}
void CBTTeleport::Update_Gui()
{
}
E::UPtr<CBTTeleport> CBTTeleport::Create()
{
	auto pInstance = E::ToUPtr(new CBTTeleport{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTTeleport");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTTeleport::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTTeleport{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTTeleport");
		return nullptr;
	}

	return pInstance;
}
