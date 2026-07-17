#include "pch.h"
#include "BTDecFlag.h"
#include "ComBeHavior.h"
NS_USING(Client)

CBTDecFlag::CBTDecFlag()
{

}
CBTDecFlag::CBTDecFlag(const CBTDecFlag& rhs) : CBTDecorator(rhs)
{

}

CBTDecFlag::~CBTDecFlag()
{
}
HRESULT CBTDecFlag::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecFlag";
	return S_OK;
}
HRESULT CBTDecFlag::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDecFlag::Evaluate(_float fTimeDelta)
{
	if (Check_Flag(ETOUI(m_iFlag)))
			return __super::Evaluate(fTimeDelta);
	
	return m_eDebug = EVALUATE::FAILED;
}
void CBTDecFlag::Update_Gui()
{
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 0,0,0,1 });
	ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.f, 0.f, 0.f, 1.f));
	uint32_t iStart = { m_iFlag };
	const _char* Flag[] = { "HIT","ATTACK","ABORT","SUPERARMOR","THORW" ,"DEAD" ,"EMISSIVE" };
	if (ImGui::TreeNode("StartFlag"))
	{

		for (uint32_t i = 0; i < std::size(Flag); ++i)
		{
			uint32_t iFlag = 1u << i;

			bool bChecked = (iStart & iFlag) != 0;

			if (ImGui::Checkbox((std::string(Flag[i]) + "##Start").c_str(), &bChecked))
			{
				if (bChecked)
					iStart = iFlag;
			}
		}
		m_iFlag = iStart;
		ImGui::TreePop();
	}
	ImGui::PopStyleColor(2);
}
nlohmann::json CBTDecFlag::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonValue(j, "Flag", m_iFlag);
	return  j;
}
HRESULT CBTDecFlag::Load_json(const nlohmann::json& j)
{

	__super::Load_json(j);
	LoadJsonValue(j, "Flag", m_iFlag);
	return S_OK;
}
E::UPtr<CBTDecFlag> CBTDecFlag::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecFlag{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : m_iFlag");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecFlag::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecFlag{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecFlag");
		return nullptr;
	}

	return pInstance;
}
