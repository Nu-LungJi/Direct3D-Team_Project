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
		if (!m_bLoop)
			Set_Flag(m_iStartFlag, FLAGTYPE::ADD);

		pAnimator->Play_Anim(m_Value.iAnimIndex, m_bLoop);
		_bool bFinished = pAnimator->GetFinish();

		//애니매이션 진행시간에 맞춰서 이동량 제어하기 m_bRatio true일 경우에만
		if (m_bRatio && m_fRatio.x <= pAnimator->GetPlayAnimRatio() && m_fRatio.y >= pAnimator->GetPlayAnimRatio())
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
			//Hit 종료는 애니매이션 끝나면
			//Attack도 애니매이션 끝나면
			Set_Flag(m_iEndFlag, FLAGTYPE::DEL);
			if (!m_bLoop) //루프 한번만 도는거 초기화용
				++m_iLoopCnt;
			if (m_iLoopCnt >= 2)
			{
				m_iLoopCnt = 0;
				return m_eDebug = EVALUATE::FAILED;
			}
			
			return m_eDebug = EVALUATE::SUCCESS;
		}
	}
	return m_eDebug = EVALUATE::RUN;
}
void CBTAttackAnimation::Update_Gui()
{
	ImGui::Text("Move Speed");
	ImGui::DragFloat("##Move Speed", &m_Value.fSpeed);

	ImGui::Text("StartRatio");
	ImGui::DragFloat("##SRaito", &m_fRatio.x, 0, 1);
	ImGui::Text("EndRatio");
	ImGui::DragFloat("##ERaito", &m_fRatio.y, 0, 1);

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
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
		if (CGameInstance::Get().MouseDown(MOUSEKEYSTATE::RB))
			m_bPopup = false;
		int32_t iIndex = CGameInstance::Get().GetAnimIndex(m_Handle);

		if (-1 != iIndex)
		{
			m_bPopup = false;
			m_Value.iAnimIndex = iIndex;
		}
		ImGui::PopStyleColor();
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
	//	NONE = 0x0000000, HIT = 0x0000001, ATTACK = 0x0000002, ABORT = 0x0000004, SUPERARMOR = 0x0000008, THROW = 0x0000010

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 0,0,0,1 });
	ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.f, 0.f, 0.f, 1.f));
	ImGui::Text("StartFlag");
	uint32_t iStart = { m_iStartFlag };
	const _char* Flag[] = {"HIT","ATTACK","ABORT","SUPERARMOR","THORW" };
	for (uint32_t i = 0; i < std::size(Flag); ++i)
	{
		uint32_t iFlag = 1u << i;
		
		bool bChecked = (iStart & iFlag) != 0;

		if (ImGui::Checkbox((std::string(Flag[i]) + "##Start").c_str(), &bChecked))
		{
			if (bChecked)
				iStart |= iFlag;
			else
				iStart &= ~iFlag;
		}
	}
	m_iStartFlag = iStart;

	ImGui::Text("EndFlag");
	uint32_t iEndFlag = { m_iEndFlag };
	for (uint32_t i = 0; i < std::size(Flag); ++i)
	{
		uint32_t iEnd = 1u << i;

		bool bChecked = (iEndFlag & iEnd) != 0;

		if (ImGui::Checkbox((std::string(Flag[i]) + "##End").c_str(), &bChecked))
		{
			if (bChecked)
				iEndFlag |= iEnd;
			else
				iEndFlag &= ~iEnd;
		}
	}
	m_iEndFlag = iEndFlag;

	ImGui::PopStyleColor(2);
}
nlohmann::json CBTAttackAnimation::Save_Node()
{
	nlohmann::json j = __super::Save_Node();

	SaveJsonValue(j, "MoveSpeed", m_Value.fSpeed);
	SaveJsonValue(j, "Loop", m_bLoop);
	SaveJsonValue(j, "EnableRatio", m_bRatio);
	SaveJsonEnum(j, "MOVE", m_eMove);

	SaveJsonValue(j, "StartFlag", m_iStartFlag);
	SaveJsonValue(j, "EndFlag", m_iEndFlag);
	JsonSaveLoadManager::SaveJsonTypeFloat2(j, "Ratio_TypeF2", m_fRatio);
	return j;
}
HRESULT CBTAttackAnimation::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "MoveSpeed", m_Value.fSpeed);
	LoadJsonValue(j, "Loop", m_bLoop);
	LoadJsonValue(j, "EnableRatio", m_bRatio);
	LoadJsonEnum(j, "MOVE", m_eMove);
	LoadJsonValue(j, "StartFlag", m_iStartFlag);
	LoadJsonValue(j, "EndFlag", m_iEndFlag);
	JsonSaveLoadManager::LoadJsonTypeFloat2(j, "Ratio_TypeF2", m_fRatio);
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
