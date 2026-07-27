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

	if (ImGui::TreeNode("AnimRoot_Flag"))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1,0,0,1 });
		Combo("Flag", m_AddFlag.iFlag);
		ImGui::DragFloat("RatioSetting", &m_AddFlag.fRatio, 0.f, 1.f);
		Combo2("FlagType", m_AddFlag.eType);

		ImGui::PopStyleColor();
		ImGui::TreePop();
	}
	if (m_AddFlag.iFlag != 0)
	{
		if (ImGui::Button("Add To Start"))
		{
			m_StartFlags.push_back(m_AddFlag);
			m_AddFlag.fRatio = 0;
			m_AddFlag.iFlag = 0;
		}
		ImGui::SameLine(150.f);
		if (ImGui::Button("Add To End"))
		{
			m_EndFlags.push_back(m_AddFlag);
			m_AddFlag.fRatio = 0;
			m_AddFlag.iFlag = 0;
		}
	}

	if (ImGui::TreeNode("Show Start Flag"))
	{
		uint32_t iNumIndex{ 0 };
		for (auto StartFlag = m_StartFlags.begin(); StartFlag != m_StartFlags.end(); ++StartFlag)
		{
			ImGui::PushID(iNumIndex);
			_string TypeName  = "SType" + std::to_string(iNumIndex) + " : ";
			_string RatioName = "SRatio" + std::to_string(iNumIndex) + " : ";
			_string FlagName  = "SFlag" + std::to_string(iNumIndex) + " : ";
			ImGui::Text(RatioName.c_str()); ImGui::SameLine();
			ImGui::DragFloat("##Ratio", &(*StartFlag).fRatio, 0.f, 1.f); ImGui::SameLine();

			ImGui::Text(FlagName.c_str()); ImGui::SameLine();
			Combo("##Flag", (*StartFlag).iFlag); ImGui::SameLine();

			ImGui::Text(TypeName.c_str()); ImGui::SameLine();
			Combo2("##Type", (*StartFlag).eType); ImGui::SameLine();

			if (ImGui::Button("Del"))
			{
				m_StartFlags.erase(StartFlag);
				ImGui::PopID();
				break;
			}
			ImGui::PopID();
			++iNumIndex;
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Show End Flag"))
	{
		uint32_t iNumIndex{ 0 };
		for (auto EndFlag = m_EndFlags.begin(); EndFlag != m_EndFlags.end(); ++EndFlag)
		{
			ImGui::PushID(iNumIndex);
			_string TypeName =  "EType" + std::to_string(iNumIndex) + " : ";
			_string RatioName = "ERatio" + std::to_string(iNumIndex) + " : ";
			_string FlagName =  "EFlag" + std::to_string(iNumIndex) + " : ";

			ImGui::Text(RatioName.c_str()); ImGui::SameLine();
			ImGui::DragFloat("##Ratio", &(*EndFlag).fRatio, 0.01f,0.f, 1.f); ImGui::SameLine();

			ImGui::Text(FlagName.c_str()); ImGui::SameLine();
			Combo("##Flag", (*EndFlag).iFlag);  ImGui::SameLine();

			ImGui::Text(TypeName.c_str()); ImGui::SameLine();
			Combo2("##Type", (*EndFlag).eType);  ImGui::SameLine();

			if (ImGui::Button("Del"))
			{
				m_EndFlags.erase(EndFlag);
				ImGui::PopID();
				break;
			}
			ImGui::PopID();
			++iNumIndex;

		}
		ImGui::TreePop();
	}


}
void CBTAnimRoot::Abort()
{
	m_bStart = true;
	m_iLoopCnt = 0;
	Reset_CheckFlag();
}
nlohmann::json CBTAnimRoot::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonValue(j, "EnableRatio", m_bRatio);
	SaveJsonValue(j, "Loop", m_bLoop);
	SaveJsonEnum(j, "SkillType", m_eSkillType);
	JsonSaveLoadManager::SaveJsonTypeFloat2(j, "SkillRatio", m_fSkillRatio);
	JsonSaveLoadManager::SaveJsonTypeFloat2(j, "Ratio_TypeF2", m_fRatio);

	size_t iMaxSize = m_StartFlags.size();
	if (!m_StartFlags.empty())
	{
		SaveJsonValue(j, "StartRatioArraySize", iMaxSize);
		for (size_t i = 0; i < m_StartFlags.size(); ++i)
		{
			_string FlagTypeName = "EventStartFlagType" + std::to_string(i);
			_string FlagRatioName = "EventStartFlagRatio" + std::to_string(i);
			_string FlagValue = "EventStartFlagValue" + std::to_string(i);

			SaveJsonEnum(j, FlagTypeName, m_StartFlags[i].eType);
			SaveJsonValue(j, FlagRatioName, m_StartFlags[i].fRatio);
			SaveJsonValue(j, FlagValue, m_StartFlags[i].iFlag);
		}
	}

	iMaxSize = m_EndFlags.size();
	if (!m_EndFlags.empty())
	{
		SaveJsonValue(j, "EndRatioArraySize", iMaxSize);
		for (size_t i = 0; i < m_EndFlags.size(); ++i)
		{
			_string FlagTypeName  = "EventEndFlagType"  + std::to_string(i);
			_string FlagRatioName = "EventEndFlagRatio" + std::to_string(i);
			_string FlagValue     = "EventEndFlagValue" + std::to_string(i);

			SaveJsonEnum(j,  FlagTypeName,  m_EndFlags[i].eType);
			SaveJsonValue(j, FlagRatioName, m_EndFlags[i].fRatio);
			SaveJsonValue(j, FlagValue,     m_EndFlags[i].iFlag);
		}
	}

	return j;
}
HRESULT CBTAnimRoot::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);

	LoadJsonValue(j, "EnableRatio", m_bRatio);
	LoadJsonValue(j, "Loop", m_bLoop);
	LoadJsonEnum(j, "SkillType", m_eSkillType);

	JsonSaveLoadManager::LoadJsonTypeFloat2(j, "SkillRatio", m_fSkillRatio);
	JsonSaveLoadManager::LoadJsonTypeFloat2(j, "Ratio_TypeF2", m_fRatio);
	

	size_t iMaxSize = {};
	if (LoadJsonValue(j, "StartRatioArraySize", iMaxSize))
	{
		m_StartFlags.resize(iMaxSize);
		for (size_t i = 0; i < iMaxSize; ++i)
		{
			_string FlagTypeName = "EventStartFlagType" + std::to_string(i);
			_string FlagRatioName = "EventStartFlagRatio" + std::to_string(i);
			_string FlagValue = "EventStartFlagValue" + std::to_string(i);

			LoadJsonEnum(j, FlagTypeName, m_StartFlags[i].eType);
			LoadJsonValue(j, FlagRatioName, m_StartFlags[i].fRatio);
			LoadJsonValue(j, FlagValue, m_StartFlags[i].iFlag);
		}
	}

	if (LoadJsonValue(j, "EndRatioArraySize", iMaxSize))
	{
		m_EndFlags.resize(iMaxSize);
		for (size_t i = 0; i < iMaxSize; ++i)
		{
			_string FlagTypeName = "EventEndFlagType" + std::to_string(i);
			_string FlagRatioName = "EventEndFlagRatio" + std::to_string(i);
			_string FlagValue = "EventEndFlagValue" + std::to_string(i);

			LoadJsonEnum(j, FlagTypeName, m_EndFlags[i].eType);
			LoadJsonValue(j, FlagRatioName, m_EndFlags[i].fRatio);
			LoadJsonValue(j, FlagValue, m_EndFlags[i].iFlag);
		}
	}
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
			}
		}
	}
}
void CBTAnimRoot::EventFlagToRatio(_float fRatio)
{
	if (auto pBT = Get_ComBT())
	{
		for (auto& startFlag : m_StartFlags)
		{
			if (startFlag.bFlag == true)
				continue;

			if (fRatio >= startFlag.fRatio)
			{
				pBT->Set_Flag(startFlag.iFlag, startFlag.eType);
				startFlag.bFlag = true;
			}
		}
		for (auto& endFlag : m_EndFlags)
		{
			if (endFlag.bFlag == true)
				continue;

			if (fRatio >= endFlag.fRatio)
			{
				pBT->Set_Flag(endFlag.iFlag, endFlag.eType);
				endFlag.bFlag = true;
			}
		}
	}
}
void CBTAnimRoot::Reset_CheckFlag()
{
	for (auto& startFlag : m_StartFlags)
		startFlag.bFlag = false;
	for (auto& endFlag : m_EndFlags)
		endFlag.bFlag = false;

}
void CBTAnimRoot::Combo(const _char* pName,uint32_t& iFlag)
{
	struct GuiView
	{
		uint32_t iValue{};
		const _char* pName{};
	};
#define X(name, value) value, #name,
	const GuiView Flags[] = { BTFLAG_M };
#undef X
	const _char* pPreview{};
	for (const auto& Flag : Flags)
	{
		if (Flag.iValue == iFlag)
		{
			pPreview = Flag.pName;
			break;
		}
	}
	if (ImGui::BeginCombo(pName, pPreview))
	{
		for (uint32_t i = 0; i < std::size(Flags); ++i)
		{
			uint32_t iFFlag = 1u << i;
			bool bSelect = iFlag == iFFlag;
			if (ImGui::Selectable(Flags[i].pName))
				iFlag = Flags[i].iValue;

			if (bSelect)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
}
void CBTAnimRoot::Combo2(const _char* pName, FLAGTYPE& eType)
{
	if (ImGui::BeginCombo(pName, MagicEnumToStringView(eType).data()))
	{
		for (uint32_t i = 0; i < ETOUI(FLAGTYPE::RESET) + 1; ++i)
		{
			_bool bSelect = eType == static_cast<FLAGTYPE>(i);
			if (ImGui::Selectable(MagicEnumToStringView(static_cast<FLAGTYPE>(i)).data()))
			{
				eType = static_cast<FLAGTYPE>(i);
			}
			if (bSelect)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
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
