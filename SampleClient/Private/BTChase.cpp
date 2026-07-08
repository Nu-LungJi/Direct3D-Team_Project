#include "pch.h"
#include "BTChase.h"
#include "ComTransform.h" 
NS_USING(Client)

CBTChase::CBTChase()
{

}
CBTChase::CBTChase(const CBTChase& rhs) : CBTActionNode(rhs)
{

}

CBTChase::~CBTChase()
{
}
HRESULT CBTChase::InitalizePrototype(void* pArg)
{
	__super::InitalizePrototype(pArg);

	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTChase";
	return S_OK;
}
HRESULT CBTChase::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

nlohmann::json CBTChase::Save_Node()
{
	nlohmann::json j = __super::Save_Node();

	SaveJsonEnum(j, "MOVE", m_eMove);

	return j;
}

HRESULT CBTChase::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	return S_OK;
}

EVALUATE CBTChase::Evaluate(_float fTimeDelta)
{
	auto pTransform = Cast<CComTransform>(Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	if (pTransform == nullptr)
		return EVALUATE::FAILED;

	return EVALUATE::SUCCESS;
}
void CBTChase::Update_Gui()
{
#define X(name)#name,
	const _char* pMoveType[] = { MOVE_M };
#undef X
	ImGui::Text("Current Move Type : "); ImGui::SameLine(140.f); ImGui::Text(pMoveType[ETOUI(m_eMove)]);

	for (uint32_t i = 0; i < 4; ++i)
	{
		if (ImGui::Button(pMoveType[i]))
			m_eMove = static_cast<MOVE>(i);
	}
}
E::UPtr<CBTChase> CBTChase::Create()
{
	auto pInstance = E::ToUPtr(new CBTChase{});
	if (FAILED(pInstance->InitalizePrototype()))
	{
		MSG_BOX("Failed to Created : CBTChase");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CBTRoot> CBTChase::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTChase{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTChase");
		return nullptr;
	}

	return pInstance;
}
