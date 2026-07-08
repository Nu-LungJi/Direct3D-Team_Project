#include "pch.h"
#include "BTDecTimer.h" 
NS_USING(Client)

CBTDecTimer::CBTDecTimer()
{

}

CBTDecTimer::CBTDecTimer(const CBTDecTimer& rhs) : CBTDecorator(rhs)
{

}
CBTDecTimer::~CBTDecTimer()
{
}
HRESULT CBTDecTimer::InitalizePrototype(void* pArg)
{
	__super::InitalizePrototype(pArg);
	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecSearch";
	return S_OK;
}
HRESULT CBTDecTimer::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDecTimer::Evaluate(_float fTimeDelta)
{
	m_fTick += fTimeDelta;
	
	if (m_fWaitTime < m_fTick)
		return EVALUATE::SUCCESS;

	EVALUATE result = __super::Evaluate(fTimeDelta);

	if (result != EVALUATE::RUN)
		m_fTick = 0.f;

	return result;
}
nlohmann::json CBTDecTimer::Save_Node()
{
	nlohmann::json j;
	j = __super::Save_Node();
	SaveJsonValue(j, "WaitTime", m_fWaitTime);
	SaveJsonValue(j, "MaxTimeCnt", m_iMaxTimeCnt);
	return j;
}
HRESULT CBTDecTimer::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	if (!LoadJsonValue(j, "WaitTime", m_fWaitTime))
		MSG_BOX("Failed Load MaxTimeTickCnt : BTDecTimer");

	if (!LoadJsonValue(j, "MaxTimeCnt", m_iMaxTimeCnt))
		MSG_BOX("Failed Load MaxTimeCnt : BTDecTimer");
	return S_OK;
}
void		CBTDecTimer::Update_Gui()
{
	ImGui::Text("TimerTick Cnt %2.f: ");
	ImGui::DragFloat("##Timer", &m_fTimeTickCnt, 0, 100);

	ImGui::Text("Current Tick %2.f : ", &m_fTick);

	ImGui::Text("TimerMaxdCnt %d: ");
	ImGui::DragInt("##Timer", &m_iMaxTimeCnt, 0, 100);
}
E::UPtr<CBTDecTimer> CBTDecTimer::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecTimer{});
	if (FAILED(pInstance->InitalizePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecTimer");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CBTRoot> CBTDecTimer::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecTimer{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecTimer");
		return nullptr;
	}

	return pInstance;
}
