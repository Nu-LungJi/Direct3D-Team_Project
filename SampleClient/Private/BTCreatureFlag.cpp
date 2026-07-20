#include "pch.h"
#include "BTCreatureFlag.h"
#include "ComTransform.h" 
NS_USING(Client)

CBTCreatureFlag::CBTCreatureFlag()
{

}
CBTCreatureFlag::CBTCreatureFlag(const CBTCreatureFlag& rhs) : CBTActionNode(rhs)
{

}

CBTCreatureFlag::~CBTCreatureFlag()
{
}
HRESULT CBTCreatureFlag::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTCreatureFlag";
	return S_OK;
}
HRESULT CBTCreatureFlag::Initalize(void* pArg)
{
	__super::Initalize(pArg);

	return S_OK;
}


EVALUATE CBTCreatureFlag::Evaluate(_float fTimeDelta)
{
	if (m_eType == FLAGTYPE::ADD)
		Set_Flag(m_iFlag, m_eType);
	else if (m_eType == FLAGTYPE::DEL)
		Set_Flag(m_iFlag, m_eType);
	else if(m_eType == FLAGTYPE::RESET)
		Set_Flag(0x0000000, m_eType);
	
	return  m_eDebug = EVALUATE::SUCCESS;
}
void CBTCreatureFlag::Update_Gui()
{
	FLAGTYPE eType[3] = { FLAGTYPE::ADD,FLAGTYPE::DEL, FLAGTYPE::RESET };
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
	ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1, 0, 0, 1));
	if(ImGui::TreeNode("FlagType"))
	{
		for (uint32_t i = 0; i < 3; ++i)
		{
			uint32_t iFlag = 1u << i;

			bool bChecked = i == ETOUI(m_eType);

			if (ImGui::Checkbox(MagicEnumToStringView(eType[i]).data(), &bChecked))
				m_eType = eType[i];
		}

		ImGui::TreePop();
	}
	uint32_t iStart = { m_iFlag };
#define X(name)#name,
	const _char* Flag[] = { BTFLAG_M };
#undef X
	if (ImGui::TreeNode("FlagValue"))
	{
		for (uint32_t i = 0; i < std::size(Flag); ++i)
		{
			uint32_t iFlag = 1u << i;

			bool bChecked = (iStart & iFlag) != 0;

			if (ImGui::Checkbox((std::string(Flag[i]) + "##Start").c_str(), &bChecked))
			{
				if (bChecked)
					iStart |= iFlag;
				else
					iStart &= ~iFlag;
			}
		}
		m_iFlag = iStart;
		ImGui::TreePop();
	}
	ImGui::PopStyleColor(2);
}
nlohmann::json CBTCreatureFlag::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonValue(j, "FlagType", m_iFlag);
	SaveJsonEnum(j, "FlagValue", m_eType);

	return j;
}
HRESULT CBTCreatureFlag::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "FlagType", m_iFlag);
	LoadJsonEnum(j, "FlagValue", m_eType);

	return E_NOTIMPL;
}
E::UPtr<CBTCreatureFlag> CBTCreatureFlag::Create()
{
	auto pInstance = E::ToUPtr(new CBTCreatureFlag{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTCreatureFlag");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTCreatureFlag::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTCreatureFlag{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTCreatureFlag");
		return nullptr;
	}

	return pInstance;
}
