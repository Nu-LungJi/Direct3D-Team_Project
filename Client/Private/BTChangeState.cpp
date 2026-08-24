#include "pch.h"
#include "BTChangeState.h"
#include "ComBeHavior.h"
#include "BTBlackBoard.h"
NS_USING(Client)

CBTChangeState::CBTChangeState()
{

}
CBTChangeState::CBTChangeState(const CBTChangeState& rhs) : CBTActionNode(rhs)
{
	m_eState = rhs.m_eState;
}

CBTChangeState::~CBTChangeState()
{
}
HRESULT CBTChangeState::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTChangeState";
	return S_OK;
}
HRESULT CBTChangeState::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTChangeState::Evaluate(_float fTimeDelta)
{
	//--------------NullCheck---------------//
	auto pBT = Get_ComBT();
	if (!pBT) return m_eDebug = EVALUATE::FAILED;

	auto pBB = pBT->Get_Blackboard();
	if (!pBB) return m_eDebug = EVALUATE::FAILED;
	//--------------------------------------//

	auto* pState = pBB->Get_Value<NPC_STATE>(NPC_KEY::STATE);
	if (nullptr == pState) return m_eDebug = EVALUATE::FAILED;
	if (*pState == m_eState) return m_eDebug = EVALUATE::FAILED;

	pBB->Set_Value<NPC_STATE>(NPC_KEY::STATE, m_eState);
	return m_eDebug = EVALUATE::SUCCESS;
}
void CBTChangeState::Update_Gui()
{
	ImGui::Text("NpcState");
	if (ImGui::BeginCombo("##NpcState", MagicEnumToStringView(m_eState).data()))
	{
		for (uint32_t i = 0; i < ETOUI(NPC_STATE::END); ++i)
		{
			_bool bSelect = i == ETOUI(m_eState);

			if (ImGui::Selectable(MagicEnumToStringView(static_cast<NPC_STATE>(i)).data(), bSelect))
			{
				m_eState = static_cast<NPC_STATE>(i);
			}
			if (bSelect)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}


}
nlohmann::json CBTChangeState::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonEnum(j, "NPC_STATE", m_eState);
	return  j;
}
HRESULT CBTChangeState::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonEnum(j, "NPC_STATE", m_eState);
	return S_OK;
}
E::UPtr<CBTChangeState> CBTChangeState::Create()
{
	auto pInstance = E::ToUPtr(new CBTChangeState{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTChangeState");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTChangeState::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTChangeState{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTChangeState");
		return nullptr;
	}

	return pInstance;
}
