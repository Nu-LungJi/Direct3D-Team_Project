#include "pch.h"
#include "BTDecHitPlayer.h"
#include "ComBeHavior.h"
#include "Player.h"
NS_USING(Client)

CBTDecHitPlayer::CBTDecHitPlayer()
{

}
CBTDecHitPlayer::CBTDecHitPlayer(const CBTDecHitPlayer& rhs) : CBTDecorator(rhs)
{

}

CBTDecHitPlayer::~CBTDecHitPlayer()
{
}
HRESULT CBTDecHitPlayer::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecHitPlayer";
	return S_OK;
}
HRESULT CBTDecHitPlayer::Initalize(void* pArg)
{

	__super::Initalize(pArg);
	return S_OK;
}
_bool CBTDecHitPlayer::bHitCheckPlayer()
{
	auto* pBT = Get_ComBT();
	if (nullptr == pBT) return false;

	auto* pSrc = pBT->GetGameObject();
	if (nullptr == pSrc) return false;



	return true;
}
EVALUATE CBTDecHitPlayer::Evaluate(_float fTimeDelta)
{
	if (bHitCheckPlayer())
		return m_eDebug = __super::Evaluate(fTimeDelta);

	return m_eDebug = EVALUATE::FAILED;
}


void CBTDecHitPlayer::Update_Gui()
{
	ImGui::Text("MaxHitCnt :");
	ImGui::DragInt("##Hitcnt", &m_iMaxHitCnt, 0.1f, 0, 100);
}
void CBTDecHitPlayer::Abort()
{
	__super::Abort();
}
nlohmann::json CBTDecHitPlayer::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonValue(j, "HitCnt", m_iMaxHitCnt);


	return j;
}
HRESULT CBTDecHitPlayer::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "HitCnt", m_iMaxHitCnt);
	return S_OK;
}
E::UPtr<CBTDecHitPlayer> CBTDecHitPlayer::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecHitPlayer{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecHitPlayer");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecHitPlayer::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecHitPlayer{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecHitPlayer");
		return nullptr;
	}

	return pInstance;
}
