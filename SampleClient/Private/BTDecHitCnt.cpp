#include "pch.h"
#include "BTDecHitCnt.h"
#include "ComBeHavior.h"
#include "TestGob.h"
NS_USING(Client)

CBTDecHitCnt::CBTDecHitCnt()
{

}
CBTDecHitCnt::CBTDecHitCnt(const CBTDecHitCnt& rhs) : CBTDecorator(rhs)
{

}

CBTDecHitCnt::~CBTDecHitCnt()
{
}
HRESULT CBTDecHitCnt::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecHitCnt";
	return S_OK;
}
HRESULT CBTDecHitCnt::Initalize(void* pArg)
{

	__super::Initalize(pArg);
	return S_OK;
}
EVALUATE CBTDecHitCnt::Evaluate(_float fTimeDelta)
{
	auto pBT = Get_ComBT();
	if (nullptr == pBT) return m_eDebug = EVALUATE::FAILED;
	auto pObj = static_cast<CTestGob*>(pBT->GetGameObject());
	if (nullptr == pObj) return m_eDebug = EVALUATE::FAILED;

	if (!m_bDeadCheck)
	{
		if (pObj->Get_CurrentHp() <= pObj->Get_MaxHp() / m_fdivided)
			return __super::Evaluate(fTimeDelta);
	}
	else if (m_bDeadCheck)
	{
		if (pObj->Get_CurrentHp() <= 0)
			return __super::Evaluate(fTimeDelta);

	}

	return m_eDebug = EVALUATE::FAILED;
}


void CBTDecHitCnt::Update_Gui()
{
	

}
E::UPtr<CBTDecHitCnt> CBTDecHitCnt::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecHitCnt{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecHitCnt");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecHitCnt::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecHitCnt{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecHitCnt");
		return nullptr;
	}

	return pInstance;
}
