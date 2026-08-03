#include "pch.h"
#include "BTMonResetTable.h"
#include"Monster.h"
#include "ComTransform.h" 
NS_USING(Client)

CBTMonResetTable::CBTMonResetTable()
{

}
CBTMonResetTable::CBTMonResetTable(const CBTMonResetTable& rhs) : CBTActionNode(rhs)
{

}

CBTMonResetTable::~CBTMonResetTable()
{
}
HRESULT CBTMonResetTable::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTMonResetTable";
	return S_OK;
}
HRESULT CBTMonResetTable::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	return S_OK;
}


EVALUATE CBTMonResetTable::Evaluate(_float fTimeDelta)
{
	if (auto pBT = Get_ComBT())
	{
		if (auto pMonster = static_cast<CMonster*>(pBT->GetGameObject()))
		{
			if (m_bHardReset)
			{
				pMonster->ReActiveTable();
				return m_eDebug = EVALUATE::SUCCESS;
			}
				
			if (!pMonster->Is_ActiveHit())
				return m_eDebug = EVALUATE::FAILED;

			pMonster->ReActiveTable();
			return m_eDebug = EVALUATE::SUCCESS;
		}
	}

	return m_eDebug= EVALUATE::FAILED;
}
void CBTMonResetTable::Update_Gui()
{
	ImGui::Text("HardReset : %s" , m_bHardReset == true ? "TRUE" : "FALSE");
	if (ImGui::Button("Active"))
		m_bHardReset = !m_bHardReset;
}
nlohmann::json CBTMonResetTable::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonValue(j, "HardReset", m_bHardReset);
	return j;
}
HRESULT CBTMonResetTable::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "HardReset", m_bHardReset);
	return S_OK;
}
E::UPtr<CBTMonResetTable> CBTMonResetTable::Create()
{
	auto pInstance = E::ToUPtr(new CBTMonResetTable{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTMonResetTable");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTMonResetTable::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTMonResetTable{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTMonResetTable");
		return nullptr;
	}

	return pInstance;
}
