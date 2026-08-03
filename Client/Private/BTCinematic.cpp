#include "pch.h"
#include "BTCinematic.h"
#include"Monster.h"
#include "ComTransform.h" 
NS_USING(Client)

CBTCinematic::CBTCinematic()
{

}
CBTCinematic::CBTCinematic(const CBTCinematic& rhs) : CBTActionNode(rhs)
{

}

CBTCinematic::~CBTCinematic()
{
}
HRESULT CBTCinematic::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTCinematic";
	return S_OK;
}
HRESULT CBTCinematic::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	return S_OK;
}


EVALUATE CBTCinematic::Evaluate(_float fTimeDelta)
{
	FCinematicPlayOptions option{};
	option.eReturnMode = ECinematicReturnMode::Blend;
	option.fReturnBlendDuration = 1.5f;
	CGameInstance::Get().PlayCinematic("TombBossIntro", m_Handle, option);
	return m_eDebug = EVALUATE::SUCCESS;
}
void CBTCinematic::Update_Gui()
{
}
nlohmann::json CBTCinematic::Save_Node()
{
	nlohmann::json j = __super::Save_Node();

	return j;
}
HRESULT CBTCinematic::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	return S_OK;
}
void CBTCinematic::OnEnter()
{
}
void CBTCinematic::OnExit(EVALUATE eResult)
{
}
E::UPtr<CBTCinematic> CBTCinematic::Create()
{
	auto pInstance = E::ToUPtr(new CBTCinematic{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTCinematic");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTCinematic::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTCinematic{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTCinematic");
		return nullptr;
	}

	return pInstance;
}
