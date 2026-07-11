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
HRESULT CBTDecTimer::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecTimer";
	return S_OK;
}
HRESULT CBTDecTimer::Initalize(void* pArg)
{
	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDecTimer::Evaluate(_float fTimeDelta)
{
	if (Check_Flag(ETOUI(BTFLAG::ATTACK)))
		return EVALUATE::FAILED;
	m_fTick += fTimeDelta;
	EVALUATE result{ EVALUATE::END };
	//m_bRun 이 true 일떄만 해당노드 재진입
	if (m_fWaitTime > m_fTick)
		return result = m_bRun == true ? EVALUATE::RUN : EVALUATE::FAILED;

	result = __super::Evaluate(fTimeDelta);
	if (result != EVALUATE::RUN)
		m_fTick = 0.f;

	return result;
}
void CBTDecTimer::Abort()
{
	if (auto pBT = Get_ComBT())
	{
		if(Check_Flag(ETOUI(BTFLAG::HIT)))
			m_fTick = 0;
	}
		
}
nlohmann::json CBTDecTimer::Save_Node()
{
	nlohmann::json j= __super::Save_Node();
	SaveJsonValue(j, "WaitTime", m_fWaitTime);
	SaveJsonValue(j, "Run", m_bRun);
	return j;
}
HRESULT CBTDecTimer::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	if (!LoadJsonValue(j, "WaitTime", m_fWaitTime))
		MSG_BOX("Failed Load MaxTimeTickCnt : BTDecTimer");

	LoadJsonValue(j, "Run", m_bRun);
	return S_OK;
}
void		CBTDecTimer::Update_Gui()
{
	ImGui::Text("TimerTick Cnt");
	ImGui::DragFloat("##Timer1", &m_fWaitTime, 0, 100);

	ImGui::Text("Current Tick %2.f : ", m_fTick);

	if (ImGui::Button("Run : "))
		m_bRun = !m_bRun;
	ImGui::SameLine(50.f);  m_bRun == true ? ImGui::Text("TRUE") : ImGui::Text("FALSE");

}
E::UPtr<CBTDecTimer> CBTDecTimer::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecTimer{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecTimer");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecTimer::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecTimer{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecTimer");
		return nullptr;
	}

	return pInstance;
}
