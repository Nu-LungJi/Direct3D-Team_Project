#include "pch.h"
#include "BTMonAttReset.h"
#include "ComAnimator.h" 
#include "Monster.h"
#include "ComCharacterMoveIntent.h"
NS_USING(Client)

CBTMonAttReset::CBTMonAttReset()
{

}
CBTMonAttReset::CBTMonAttReset(const CBTMonAttReset& rhs) : CBTActionNode(rhs)
{

}

CBTMonAttReset::~CBTMonAttReset()
{

}
HRESULT CBTMonAttReset::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTMonAttReset";
	return S_OK;
}
HRESULT CBTMonAttReset::Initalize(void* pArg)
{
	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTMonAttReset::Evaluate(_float fTimeDelta)
{
	if (auto pBT = Get_ComBT())
	{
		if (auto pSrc = pBT->GetGameObject())
		{
			static_cast<CMonster*>(pSrc)->Set_AttTable(ATTMON::END);
		}
	}
	return m_eDebug = EVALUATE::SUCCESS;
}
void CBTMonAttReset::Update_Gui()
{
}
void CBTMonAttReset::Abort()
{
}

E::UPtr<CBTMonAttReset> CBTMonAttReset::Create()
{
	auto pInstance = E::ToUPtr(new CBTMonAttReset{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTMonAttReset");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTMonAttReset::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTMonAttReset{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTMonAttReset");
		return nullptr;
	}

	return pInstance;
}
