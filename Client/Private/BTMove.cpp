#include "pch.h"
#include "BTMove.h"
#include "ComTransform.h" 
#include "ComCharacterMoveIntent.h"
#include "Monster.h"
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
HRESULT CBTMove::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

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
	LoadJsonEnum(j, "MOVE", m_eMove);
	return S_OK;
}

EVALUATE CBTMove::Evaluate(_float fTimeDelta)
{
	auto pTransform =(Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	auto pMoveIntent = Get_Component<CComCharacterMoveIntent>(m_Handle, "ComCharacterMoveIntent");
	if (pTransform == nullptr || pMoveIntent == nullptr)
		return m_eDebug = EVALUATE::FAILED;
	if (auto pBT = Get_ComBT())
	{
		if (auto pOwner = static_cast<CMonster*>(pBT->GetGameObject()))
		{
			if (auto pTarget = pOwner->Get_Target())
			{
				_vector vDirection{};
				if (m_eMove == MOVE::RIGHT)
					vDirection = pTransform->GetState(STATE::RIGHT);
				else if (m_eMove == MOVE::LEFT)
					vDirection = -pTransform->GetState(STATE::RIGHT);
				else if (m_eMove == MOVE::STRAIGHT)
					vDirection = pTransform->GetState(STATE::LOOK);
				else if (m_eMove == MOVE::BACKWARD)
					vDirection = -pTransform->GetState(STATE::LOOK);
				else
					return m_eDebug = EVALUATE::FAILED;

				_float3 vMoveDirection{};
				XMStoreFloat3(&vMoveDirection, vDirection);
				pMoveIntent->SetMoveIntent(vMoveDirection, 2.f);
			}
		}
	}

	return m_eDebug = EVALUATE::SUCCESS;
}
void CBTMove::Update_Gui()
{
#define X(name)#name,
	const _char* pMoveType[] = { MOVE_M };
#undef X
	ImGui::Text("Move Selector");
	if(ImGui::BeginCombo("##Move Seletor", pMoveType[(ETOUI(m_eMove))]))
	{
		for (uint32_t i = 0; i < 4; ++i)
		{
			_bool bSelect = static_cast<int32_t>(m_eMove) == i;

			if (ImGui::Selectable(pMoveType[i]))
				m_eMove = static_cast<MOVE>(i);

			if (bSelect)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

}
E::UPtr<CBTMove> CBTMove::Create()
{
	auto pInstance = E::ToUPtr(new CBTMove{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTMove");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTMove::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTMove{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTMove");
		return nullptr;
	}

	return pInstance;
}
