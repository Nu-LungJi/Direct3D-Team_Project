#include "pch.h"
#include "BTDecNpcState.h"
#include "ComBeHavior.h"
#include "BTBlackBoard.h"
NS_USING(Client)

CBTDecNpcState::CBTDecNpcState()
{

}
CBTDecNpcState::CBTDecNpcState(const CBTDecNpcState& rhs) : CBTDecorator(rhs)
{
	m_eState = rhs.m_eState;
}

CBTDecNpcState::~CBTDecNpcState()
{
}
HRESULT CBTDecNpcState::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecNpcState";
	return S_OK;
}
HRESULT CBTDecNpcState::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDecNpcState::Evaluate(_float fTimeDelta)
{
	//--------------NullCheck---------------//
	auto pBT = Get_ComBT();
	if (!pBT) return m_eDebug = EVALUATE::FAILED;

	auto pBB = pBT->Get_Blackboard();
	if (!pBB) return m_eDebug = EVALUATE::FAILED;

	auto pState = pBB->Get_Value<AGENT_STATE>(NPC_KEY::STATE);
	if (!pState) return m_eDebug = EVALUATE::FAILED;
	//--------------------------------------//

	if (*pState != m_eState)
		return m_eDebug = EVALUATE::FAILED;

	return m_eDebug = __super::Evaluate(fTimeDelta);
}
void CBTDecNpcState::Update_Gui()
{
	ImGui::Text("NpcState");
	if (ImGui::BeginCombo("##NpcState", MagicEnumToStringView(m_eState).data()))
	{
		for (uint32_t i = 0; i < ETOUI(AGENT_STATE::END); ++i)
		{
			_bool bSelect = i == ETOUI(m_eState);

			if (ImGui::Selectable(MagicEnumToStringView(static_cast<AGENT_STATE>(i)).data(), bSelect))
			{
				m_eState = static_cast<AGENT_STATE>(i);
			}
			if (bSelect)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}


}
nlohmann::json CBTDecNpcState::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonEnum(j, "AGENT_STATE", m_eState);
	return  j;
}
HRESULT CBTDecNpcState::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonEnum(j, "AGENT_STATE", m_eState);
	return S_OK;
}
E::UPtr<CBTDecNpcState> CBTDecNpcState::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecNpcState{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecNpcState");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecNpcState::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecNpcState{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecNpcState");
		return nullptr;
	}

	return pInstance;
}
