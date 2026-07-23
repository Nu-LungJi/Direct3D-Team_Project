#include "pch.h"
#include "BTAnimRoot.h"
#include "ComAnimator.h" 
NS_USING(Client)

CBTAnimRoot::CBTAnimRoot()
{

}
CBTAnimRoot::CBTAnimRoot(const CBTAnimRoot& rhs) : CBTActionNode(rhs)
{

}

CBTAnimRoot::~CBTAnimRoot()
{
}
HRESULT CBTAnimRoot::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_eGroup = NODEGROUP::ANIMATION;
	return S_OK;
}
HRESULT CBTAnimRoot::Initalize(void* pArg)
{
	__super::Initalize(pArg);

	return S_OK;
}

void CBTAnimRoot::Update_Gui()
{
	if (ImGui::TreeNode("AnimRoot"))
	{

		if (ImGui::Button("Enable Ratio : "))
			m_bRatio = !m_bRatio;
			ImGui::SameLine(110.f);
			m_bRatio == true ? ImGui::Text("TRUE") : ImGui::Text("FALSE");

			if (ImGui::Button("Loop Change"))
				m_bLoop = !m_bLoop;
				ImGui::Text("Loop : %s", m_bLoop ? "TRUE" : "FALSE");

				ImGui::Text("StartRatio");
		ImGui::DragFloat("##SRaito", &m_fRatio.x, 0.f, 1.f);
		ImGui::Text("EndRatio");
		ImGui::DragFloat("##ERaito", &m_fRatio.y, 0.f, 1.f);

		ImGui::Text("SkillRatio");
		ImGui::DragFloat2("##SKRaito", reinterpret_cast<_float*>(&m_fSkillRatio), 0.f, 1.f);

		ImGui::Text("AttMon Type");
		if (ImGui::BeginCombo("##AttMon Typer", MagicEnumToStringView(m_eSkillType).data()))
		{
			for (uint32_t i = 0; i < ETOUI(ATTMON::END) + 1; ++i)
			{
				_bool bSelect = static_cast<int32_t>(m_eSkillType) == i;

				if (ImGui::Selectable(MagicEnumToStringView(static_cast<ATTMON>(i)).data()))
					m_eSkillType = static_cast<ATTMON>(i);

				if (bSelect)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}

		ImGui::TreePop();
	}
}
void CBTAnimRoot::Abort()
{
	m_bStart = true;
	m_iLoopCnt = 0;
}
nlohmann::json CBTAnimRoot::Save_Node()
{
	nlohmann::json j = __super::Save_Node();

	SaveJsonValue(j, "EnableRatio", m_bRatio);
	SaveJsonValue(j, "Loop", m_bLoop);
	SaveJsonValue(j, "EndFlag", m_iEndFlag);
	SaveJsonEnum(j, "SkillType", m_eSkillType);
	JsonSaveLoadManager::SaveJsonTypeFloat2(j, "SkillRatio", m_fSkillRatio);
	JsonSaveLoadManager::SaveJsonTypeFloat2(j, "Ratio_TypeF2", m_fRatio);
	return j;
}
HRESULT CBTAnimRoot::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);

	LoadJsonValue(j, "EnableRatio", m_bRatio);
	LoadJsonValue(j, "Loop", m_bLoop);
	LoadJsonValue(j, "EndFlag", m_iEndFlag);
	LoadJsonEnum(j, "SkillType", m_eSkillType);

	JsonSaveLoadManager::LoadJsonTypeFloat2(j, "SkillRatio", m_fSkillRatio);
	JsonSaveLoadManager::LoadJsonTypeFloat2(j, "Ratio_TypeF2", m_fRatio);
	return S_OK;
}
void CBTAnimRoot::Active_Skill()
{
	
	if (auto pBT = Get_ComBT())
	{
		if (pBT->Check_Flag(ETOUI(BTFLAG::ATTACK)))
			return;

		if (m_eSkillType != ATTMON::END)
		{
			if (auto pSrc = static_cast<CMonster*>(pBT->GetGameObject()))
			{
				pSrc->Set_AttTable(m_eSkillType, m_fSkillRatio);
				pBT->Set_Flag(ETOUI(BTFLAG::ATTACK), FLAGTYPE::ADD);
			}
		}
	}
}
E::UPtr<CBTAnimRoot> CBTAnimRoot::Create()
{
	auto pInstance = E::ToUPtr(new CBTAnimRoot{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTAnimRoot");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTAnimRoot::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTAnimRoot{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTAnimRoot");
		return nullptr;
	}

	return pInstance;
}
