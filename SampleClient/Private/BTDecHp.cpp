#include "pch.h"
#include "BTDecHp.h"
#include "ComBeHavior.h"
NS_USING(Client)

CBTDecHp::CBTDecHp()
{

}
CBTDecHp::CBTDecHp(const CBTDecHp& rhs) : CBTDecorator(rhs)
{

}

CBTDecHp::~CBTDecHp()
{
}
HRESULT CBTDecHp::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecHp";
	return S_OK;
}
HRESULT CBTDecHp::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	
	return S_OK;
}


EVALUATE CBTDecHp::Evaluate(_float fTimeDelta)
{

	if(m_CurrentHp <= m_MaxHp / m_GuiNode.fValue)
		return __super::Evaluate(fTimeDelta);
	
	return EVALUATE::FAILED;
}
void CBTDecHp::Update_Gui()
{
}
E::UPtr<CBTDecHp> CBTDecHp::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecHp{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecHp");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecHp::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecHp{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecHp");
		return nullptr;
	}

	return pInstance;
}
