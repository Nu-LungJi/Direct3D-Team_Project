#include "pch.h"
#include "BTDecLier.h" 
NS_USING(Client)

CBTDecLier::CBTDecLier()
{

}

CBTDecLier::CBTDecLier(const CBTDecLier& rhs) : CBTDecorator(rhs)
{

}
CBTDecLier::~CBTDecLier()
{
}
HRESULT CBTDecLier::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecLier";
	return S_OK;
}
HRESULT CBTDecLier::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDecLier::Evaluate(_float fTimeDelta)
{
	if (m_bEnter)
		return m_eDebug = m_bLierInverter == true ? EVALUATE::SUCCESS : EVALUATE::FAILED;

	EVALUATE eType = __super::Evaluate(fTimeDelta);
	if (eType == EVALUATE::SUCCESS)
		m_bEnter = true;
	
	return m_eDebug = eType;
}

void		CBTDecLier::Update_Gui()
{
	if (ImGui::Button("Inverter : "))
		m_bLierInverter = !m_bLierInverter;
	ImGui::SameLine();
	ImGui::Text(m_bLierInverter == true ? "TRUE" : "FALSE");
}
nlohmann::json CBTDecLier::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonValue(j, "Inverter", m_bLierInverter);
	return j;
}
HRESULT CBTDecLier::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "Inverter", m_bLierInverter);
	return S_OK;
}
E::UPtr<CBTDecLier> CBTDecLier::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecLier{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecLier");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecLier::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecLier{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecLier");
		return nullptr;
	}

	return pInstance;
}
