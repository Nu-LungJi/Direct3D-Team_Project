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
	//auto pTransform = Cast<CComTransform>(Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	//if (pTransform == nullptr)
	//	return EVALUATE::FAILED;
	//if()조건을 만족하면
	//__super::Evaluate(fTimeDelta);
	m_fTick += fTimeDelta;
	
	if (m_fTick > m_fTimeTickCnt)
	{
		m_fTick = 0.f;
		++m_iCurrentTimeCnt;
	}
	if (m_iCurrentTimeCnt > m_iMaxTimeCnt)
	{
		__super::Evaluate(fTimeDelta);
		m_iCurrentTimeCnt = 0;
		return EVALUATE::SUCCESS;
	}
		
	return EVALUATE::FAILED;
}
nlohmann::json CBTDecTimer::Save_Node()
{
	nlohmann::json j;
	j = __super::Save_Node();
	SaveJsonValue(j, "MaxTimeTickCnt", m_fTimeTickCnt);
	SaveJsonValue(j, "MaxTimeCnt", m_iMaxTimeCnt);
	return j;
}
HRESULT CBTDecTimer::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	if (!LoadJsonValue(j, "MaxTimeTickCnt", m_fTimeTickCnt))
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
