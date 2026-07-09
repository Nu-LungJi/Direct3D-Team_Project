#include "pch.h"
#include "BTMove.h"
#include "ComTransform.h" 
NS_USING(Client)

CBTMove::CBTMove()
{

}
CBTMove::CBTMove(const CBTMove& rhs) : CBTActionNode(rhs)
{

}

CBTMove::~CBTMove()
{
}
HRESULT CBTMove::InitalizePrototype(void* pArg)
{
	__super::InitalizePrototype(pArg);

	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTMove";
	return S_OK;
}
HRESULT CBTMove::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

nlohmann::json CBTMove::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	
	SaveJsonEnum(j, "MOVE", m_eMove);
	
	return j;
}

HRESULT CBTMove::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	return S_OK;
}

EVALUATE CBTMove::Evaluate(_float fTimeDelta)
{
	auto pTransform = Cast<CComTransform>(Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	if (pTransform == nullptr)
	{
		m_eDebug = EVALUATE::FAILED;

		return EVALUATE::FAILED;
	}
	

	//if (m_eMove ==MOVE::RIGHT&&CGameInstance::Get().KeyPressing(DIK_RIGHT))
	//{
	//	pTransform->GoRight(fTimeDelta);
	//	return EVALUATE::SUCCESS;
	//}
	//else if (m_eMove == MOVE::LEFT&&CGameInstance::Get().KeyPressing(DIK_LEFT))
	//{
	//	pTransform->GoLeft(fTimeDelta);
	//	return EVALUATE::SUCCESS;
	//}else if (m_eMove == MOVE::STRAIGHT && CGameInstance::Get().KeyPressing(DIK_UP))
	//{
		pTransform->GoStraight(10.f * fTimeDelta);

		m_eDebug = EVALUATE::SUCCESS;
		return EVALUATE::SUCCESS;
	//}
	//if (m_eMove == MOVE::STRAIGHT && CGameInstance::Get().KeyPressing(DIK_DOWN))
	//{
	//	pTransform->GoBackward(fTimeDelta);
	//	return EVALUATE::SUCCESS;
	//}
	//	
	//return EVALUATE::FAILED;
}
void CBTMove::Update_Gui()
{
//#define X(name)#name,
//	const _char* pMoveType[] = { MOVE_M };
//#undef X
//	ImGui::Text("Current Move Type : "); ImGui::SameLine(140.f); ImGui::Text(pMoveType[ETOUI(m_eMove)]);
//
//	for (uint32_t i = 0; i < 4; ++i)
//	{
//		if (ImGui::Button(pMoveType[i]))
//			m_eMove = static_cast<MOVE>(i);
//	}
}
E::UPtr<CBTMove> CBTMove::Create()
{
	auto pInstance = E::ToUPtr(new CBTMove{});
	if (FAILED(pInstance->InitalizePrototype()))
	{
		MSG_BOX("Failed to Created : CBTMove");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CBTRoot> CBTMove::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTMove{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTMove");
		return nullptr;
	}

	return pInstance;
}
