#include "pch.h"
#include "BTMonAttType.h"
#include"Monster.h"
#include "ComTransform.h" 
NS_USING(Client)

CBTMonAttType::CBTMonAttType()
{

}
CBTMonAttType::CBTMonAttType(const CBTMonAttType& rhs) : CBTActionNode(rhs)
{

}

CBTMonAttType::~CBTMonAttType()
{
}
HRESULT CBTMonAttType::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTMonAttType";
	return S_OK;
}
HRESULT CBTMonAttType::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	return S_OK;
}


EVALUATE CBTMonAttType::Evaluate(_float fTimeDelta)
{
	if (auto pBT = Get_ComBT())
		if (auto pSrc = pBT->GetGameObject())
			//static_cast<CMonster*>(pSrc)->Set_AttTable(m_eAttType);

	return m_eDebug = EVALUATE::SUCCESS;
}
void CBTMonAttType::Update_Gui()
{
	ImGui::Text("ATTTYPE : "); ImGui::SameLine();
	
	if (ImGui::BeginCombo("##AttType", MagicEnumToStringView(m_eAttType).data()))
	{
		for (uint32_t i = 0; i < ETOUI(ATTMON::END); ++i)
		{	
			_bool bSelect = m_eAttType == static_cast<ATTMON>(i);
			if (ImGui::Selectable(MagicEnumToStringView(static_cast<ATTMON>(i)).data(), bSelect))
				m_eAttType = static_cast<ATTMON>(i);

			if (bSelect)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}
}
nlohmann::json CBTMonAttType::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonEnum(j, "ATTType", m_eAttType);

	return j;
}
HRESULT CBTMonAttType::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonEnum(j, "ATTType", m_eAttType);
	return S_OK;
}
E::UPtr<CBTMonAttType> CBTMonAttType::Create()
{
	auto pInstance = E::ToUPtr(new CBTMonAttType{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTMonAttType");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTMonAttType::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTMonAttType{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTMonAttType");
		return nullptr;
	}

	return pInstance;
}
