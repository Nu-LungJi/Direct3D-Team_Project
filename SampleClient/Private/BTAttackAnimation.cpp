#include "pch.h"
#include "BTAttackAnimation.h"
#include "ComAnimator.h" 
NS_USING(Client)

CBTAttackAnimation::CBTAttackAnimation()
{

}
CBTAttackAnimation::CBTAttackAnimation(const CBTAttackAnimation& rhs) : CBTActionNode(rhs)
{

}

CBTAttackAnimation::~CBTAttackAnimation()
{
	
}
HRESULT CBTAttackAnimation::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_eGroup = NODEGROUP::ANIMATION;
	m_MasterName = "BTAttackAnimation";
	return S_OK;
}
HRESULT CBTAttackAnimation::Initalize(void* pArg)
{
	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTAttackAnimation::Evaluate(_float fTimeDelta)
{
	if (auto pBT = Get_ComBT())
	{

		auto pAnimator = (Get_Component<CComAnimator>(m_Handle, "ComCModelAnimator"));
		auto pTransform = (Get_Component<CComTransform>(m_Handle, "Com_Transform"));

		if (pTransform == nullptr || pAnimator == nullptr || -1 == m_Value.iAnimIndex)
			return m_eDebug = EVALUATE::FAILED;
		if (m_bStart)
			pAnimator->SetPlay(true);

		pAnimator->Play_Anim(m_Value.iAnimIndex, m_bLoop);
		_bool bFinished = pAnimator->GetFinish();


		if (m_bRatio && m_fRatio > pAnimator->GetPlayAnimRatio())
		{
			if (m_eMove == MOVE::RIGHT)
				pTransform->GoRight(m_Value.fSpeed * fTimeDelta);
			else if (m_eMove == MOVE::LEFT)
				pTransform->GoLeft(m_Value.fSpeed * fTimeDelta);
			else if (m_eMove == MOVE::STRAIGHT)
				pTransform->GoStraight(m_Value.fSpeed * fTimeDelta);
			else if (m_eMove == MOVE::BACKWARD)
				pTransform->GoBackward(m_Value.fSpeed * fTimeDelta);
		}

		if (m_bLoop || bFinished)
		{
		
			if (!m_bLoop)
				++m_iLoopCnt;
			if (m_iLoopCnt >= 2)
			{
				m_iLoopCnt = 0;
				return m_eDebug = EVALUATE::FAILED;
			}

			pBT->Set_Hit(false);
			return m_eDebug = EVALUATE::SUCCESS;
		}
	}
	return m_eDebug = EVALUATE::RUN;
}
void CBTAttackAnimation::Update_Gui()
{
	ImGui::Text("Move Speed");
	ImGui::DragFloat("##Move Speed", &m_Value.fSpeed);

	ImGui::Text("Raito");
	ImGui::DragFloat("##Raito", &m_fRatio, 0, 1);

	if (ImGui::Button("Enable Ratio : "))
		m_bRatio = !m_bRatio;
	ImGui::SameLine(110.f);
	m_bRatio == true ? ImGui::Text("TRUE") : ImGui::Text("FALSE");

	if (ImGui::Button("Loop Change"))
		m_bLoop = !m_bLoop;
	ImGui::Text("Loop : "); ImGui::SameLine(50.f);
	m_bLoop == true ? ImGui::Text("TRUE") : ImGui::Text("FALSE");

	if (ImGui::Button("Animation"))
		m_bPopup = true;
	if (m_bPopup)
	{
		if (CGameInstance::Get().MouseDown(MOUSEKEYSTATE::RB))
			m_bPopup = false;
		int32_t iIndex = CGameInstance::Get().GetAnimIndex(m_Handle);

		if (-1 != iIndex)
		{
			m_bPopup = false;
			m_Value.iAnimIndex = iIndex;
		}
	}


#define X(name)#name,
	const _char* pMoveType[] = { MOVE_M };
#undef X
	ImGui::Text("Move Selector");
	if (ImGui::BeginCombo("##Move Seletor", pMoveType[(ETOUI(m_eMove))]))
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
nlohmann::json CBTAttackAnimation::Save_Node()
{
	nlohmann::json j = __super::Save_Node();

	SaveJsonValue(j, "MoveSpeed", m_Value.fSpeed);
	SaveJsonValue(j, "Loop", m_bLoop);
	SaveJsonValue(j, "EnableRatio", m_bRatio);
	SaveJsonValue(j, "Ratio", m_fRatio);
	SaveJsonEnum(j, "MOVE", m_eMove);
	return j;
}
HRESULT CBTAttackAnimation::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "MoveSpeed", m_Value.fSpeed);
	LoadJsonValue(j, "Loop", m_bLoop);
	LoadJsonValue(j, "EnableRatio", m_bRatio);
	LoadJsonValue(j, "Ratio", m_fRatio);
	LoadJsonEnum(j, "MOVE", m_eMove);
	return S_OK;
}
E::UPtr<CBTAttackAnimation> CBTAttackAnimation::Create()
{
	auto pInstance = E::ToUPtr(new CBTAttackAnimation{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTAttackAnimation");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTAttackAnimation::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTAttackAnimation{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTAttackAnimation");
		return nullptr;
	}

	return pInstance;
}
