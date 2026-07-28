#include "pch.h"
#include "BTDecIsGround.h"
#include "ComBeHavior.h"
#include "Monster.h"
NS_USING(Client)

CBTDecIsGround::CBTDecIsGround()
{

}
CBTDecIsGround::CBTDecIsGround(const CBTDecIsGround& rhs) : CBTDecorator(rhs)
{

}

CBTDecIsGround::~CBTDecIsGround()
{
}
HRESULT CBTDecIsGround::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecIsGround";
	return S_OK;
}
HRESULT CBTDecIsGround::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDecIsGround::Evaluate(_float fTimeDelta)
{
	if (auto pBT = Get_ComBT())
	{
		if (auto pSrc = static_cast<CMonster*>(pBT->GetGameObject()))
		{
			if(pSrc->Is_Grounded())
				return  m_eDebug = EVALUATE::SUCCESS;
			else
			{
				__super::Evaluate(fTimeDelta);
				m_eDebug = EVALUATE::RUN;
			}
		}
	}

	return m_eDebug = EVALUATE::FAILED;
}
E::UPtr<CBTDecIsGround> CBTDecIsGround::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecIsGround{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecIsGround");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecIsGround::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecIsGround{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecIsGround");
		return nullptr;
	}

	return pInstance;
}
