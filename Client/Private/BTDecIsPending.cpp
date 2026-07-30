#include "pch.h"
#include "BTDecIsPending.h"
#include "ComBeHavior.h"
#include "Monster.h"
NS_USING(Client)

CBTDecIsPending::CBTDecIsPending()
{

}
CBTDecIsPending::CBTDecIsPending(const CBTDecIsPending& rhs) : CBTDecorator(rhs)
{

}

CBTDecIsPending::~CBTDecIsPending()
{
}
HRESULT CBTDecIsPending::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecIsPending";
	return S_OK;
}
HRESULT CBTDecIsPending::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDecIsPending::Evaluate(_float fTimeDelta)
{
	if (auto pBT = Get_ComBT())
	{
		if (auto pSrc = static_cast<CMonster*>(pBT->GetGameObject()))
		{
			if (!pSrc->Is_PendingHit() || !pSrc->Is_ActiveHit())
			{
				return m_eDebug = EVALUATE::FAILED;
			}
			else
				return m_eDebug = __super::Evaluate(fTimeDelta);
		}
	}
	
	return m_eDebug = EVALUATE::FAILED;;
}
void CBTDecIsPending::Update_Gui()
{
}
E::UPtr<CBTDecIsPending> CBTDecIsPending::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecIsPending{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecIsPending");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecIsPending::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecIsPending{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecIsPending");
		return nullptr;
	}

	return pInstance;
}
