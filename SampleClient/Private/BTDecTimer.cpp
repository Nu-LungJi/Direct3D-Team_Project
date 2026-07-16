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

EVALUATE CBTDecTimer::PAUSE(_float fTimeDelta)
{
	m_fTick += fTimeDelta;
	EVALUATE result{EVALUATE::END};
	if (m_fWaitTime > m_fTick)
		return result = m_bRun == true ? EVALUATE::RUN : EVALUATE::FAILED;

	result = __super::Evaluate(fTimeDelta);
	if (result != EVALUATE::RUN)
		m_fTick = 0.f;

	return result;
}

EVALUATE CBTDecTimer::NEXT(_float fTimeDelta)
{
	m_fTick += fTimeDelta;
	if (m_fWaitTime > m_fTick)
	{
		__super::Evaluate(fTimeDelta);
		return EVALUATE::RUN;
	}

	m_fTick = 0.f;
	return EVALUATE::SUCCESS;
}

EVALUATE CBTDecTimer::TimeOut(_float fTimeDelta)
{
	EVALUATE result{ EVALUATE::END };
	m_fTick += fTimeDelta;
	result = __super::Evaluate(fTimeDelta);
	if (result != EVALUATE::SUCCESS)
	{
		if (m_fWaitTime < m_fTick)
		{
			m_fTick = 0.f;
			return EVALUATE::FAILED;
		}
		else
			return EVALUATE::RUN;
	}
	
	m_fTick = 0.f;
	return result;
}

EVALUATE CBTDecTimer::TimeInSuccess(_float fTimeDelta)
{
	EVALUATE result = EVALUATE::FAILED;
	if (!m_bRun)
	{
		m_fTick += fTimeDelta;
		if (m_fWaitTime > m_fTick)
		{
			return EVALUATE::SUCCESS;
		}
		else
		{
			m_bRun = true;
			m_fTick = 0.f;
			return EVALUATE::FAILED;
		}
	}
	if (m_bRun)
	{
		result = __super::Evaluate(fTimeDelta);
		if (EVALUATE::SUCCESS == __super::Evaluate(fTimeDelta))
			m_bRun = false;
	}

	return result;
}

EVALUATE CBTDecTimer::Evaluate(_float fTimeDelta)
{
	if (Check_Flag(ETOUI(BTFLAG::ATTACK)))
		return EVALUATE::FAILED;
	EVALUATE result{ EVALUATE::END };
	//m_bRun 이 true 일떄만 해당노드 재진입
	if (m_eTimer == TIMER::PAUSE) //타이머 지나기 전까지 하위노드 실행 안됨 RUN 또는 FAILED 반환
		result = PAUSE(fTimeDelta);
	else if (m_eTimer == TIMER::NEXT)
		result = NEXT(fTimeDelta); // 타이머 지나는 동안 하위노드 실행
	else if (m_eTimer == TIMER::TIMEOUT)
		result = TimeOut(fTimeDelta);
	else if (m_eTimer == TIMER::TIMEIN_SUCCESS)
		result = TimeInSuccess(fTimeDelta);

	return result;
}
void CBTDecTimer::Abort()
{
	if (auto pBT = Get_ComBT())
	{
		if (Check_Flag(ETOUI(BTFLAG::HIT)))
		{
			m_bRun = true;
		}
	}
		
}
nlohmann::json CBTDecTimer::Save_Node()
{
	nlohmann::json j= __super::Save_Node();
	SaveJsonValue(j, "WaitTime", m_fWaitTime);
	SaveJsonEnum(j, "TimerType", m_eTimer);
	return j;
}
HRESULT CBTDecTimer::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	if (!LoadJsonValue(j, "WaitTime", m_fWaitTime))
		MSG_BOX("Failed Load MaxTimeTickCnt : BTDecTimer");

	LoadJsonEnum(j, "TimerType", m_eTimer);
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

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 0,0,0,1 });
	const _char* pName[] = { MagicEnumToStringView(TIMER::PAUSE).data(), MagicEnumToStringView(TIMER::NEXT).data(),
							 MagicEnumToStringView(TIMER::TIMEOUT).data(),MagicEnumToStringView(TIMER::TIMEIN_SUCCESS).data() };
	if (ImGui::TreeNode("Timer Type"))
	{
		ImGui::Text("TYPE : "); ImGui::SameLine(70.f);
		
		if (ImGui::BeginCombo("##Timer Type", pName[ETOUI(m_eTimer)]))
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1,0,0,1 });
			for (int i = 0; i < IM_ARRAYSIZE(pName); i++)
			{
				// 3. 선택 가능한 항목 생성
				const bool is_selected = (ETOUI(m_eTimer) == i);
				if (ImGui::Selectable(pName[i], is_selected))
					m_eTimer = static_cast<TIMER>(i); // 클릭 시 인덱스 업데이트

				// 선택된 항목에 포커스 자동 이동
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::PopStyleColor();
			ImGui::EndCombo();
		}
		ImGui::TreePop();
	}
	ImGui::PopStyleColor();
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
