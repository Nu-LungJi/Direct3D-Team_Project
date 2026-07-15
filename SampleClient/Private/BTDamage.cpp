#include "pch.h"
#include "BTDamage.h"
#include "ComTransform.h" 
NS_USING(Client)

CBTDamage::CBTDamage()
{

}
CBTDamage::CBTDamage(const CBTDamage& rhs) : CBTActionNode(rhs)
{

}

CBTDamage::~CBTDamage()
{
}
HRESULT CBTDamage::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTDamage";
	return S_OK;
}
HRESULT CBTDamage::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

nlohmann::json CBTDamage::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonValue(j, "Attack", m_GuiNode.fValue);

	return j;
}

HRESULT CBTDamage::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "Attack", m_GuiNode.fValue);

	return S_OK;
}


EVALUATE CBTDamage::Evaluate(_float fTimeDelta)
{
	auto pTransform = (Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	if (pTransform == nullptr)
		return m_eDebug = EVALUATE::FAILED;
	//데미지 이후 true시 피격판정 애니매이션인데 슈퍼아머중에는 진입 못하게 막기
	if (Check_Flag(ETOUI(BTFLAG::SUPERARMOR)))
		return m_eDebug = EVALUATE::FAILED;

	return m_eDebug = EVALUATE::SUCCESS;
}
void CBTDamage::Update_Gui()
{
	ImGui::Text("Attack");
	ImGui::DragFloat("##Attack", &m_GuiNode.fValue, 0, 100);

}
E::UPtr<CBTDamage> CBTDamage::Create()
{
	auto pInstance = E::ToUPtr(new CBTDamage{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDamage");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDamage::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDamage{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDamage");
		return nullptr;
	}

	return pInstance;
}
