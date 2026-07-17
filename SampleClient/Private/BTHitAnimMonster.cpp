#include "pch.h"
#include "BTHitAnimMonster.h"
#include "ComAnimator.h" 
NS_USING(Client)

CBTHitAnimMonster::CBTHitAnimMonster()
{

}
CBTHitAnimMonster::CBTHitAnimMonster(const CBTHitAnimMonster& rhs) : CBTActionNode(rhs)
{

}

CBTHitAnimMonster::~CBTHitAnimMonster()
{

}
HRESULT CBTHitAnimMonster::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_eGroup = NODEGROUP::ANIMATION;
	m_MasterName = "BTHitAnimMonster";
	return S_OK;
}
HRESULT CBTHitAnimMonster::Initalize(void* pArg)
{
	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTHitAnimMonster::Evaluate(_float fTimeDelta)
{

	if (auto pBT = Get_ComBT())
	{
		auto pAnimator = (Get_Component<CComAnimator>(m_Handle, "ComCModelAnimator"));
		auto pTransform = (Get_Component<CComTransform>(m_Handle, "Com_Transform"));

		if (pTransform == nullptr || pAnimator == nullptr)
			return m_eDebug = EVALUATE::FAILED;
		if (m_bStart)
		{
			if (false == HitType())
				return EVALUATE::FAILED;
			Set_Flag(m_iStartFlag, FLAGTYPE::ADD);
			m_bStart = false;
		}
		
		pAnimator->SetPlay(true);
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
			Set_Flag(m_iEndFlag, FLAGTYPE::DEL);
			m_bStart = true;
			return m_eDebug = EVALUATE::SUCCESS;

		}
	}
	return m_eDebug = EVALUATE::RUN;
}
void CBTHitAnimMonster::Update_Gui()
{
	ImGui::Text("Move Speed");
	ImGui::DragFloat("##Move Speed", &m_Value.fSpeed);

	ImGui::Text("StartRatio");
	ImGui::DragFloat("##SRaito", &m_fRatio.x, 0.f, 1.f);
	ImGui::Text("EndRatio");
	ImGui::DragFloat("##ERaito", &m_fRatio.y, 0.f, 1.f);

	if (ImGui::Button("Enable Ratio : "))
		m_bRatio = !m_bRatio;
	ImGui::SameLine(110.f);
	m_bRatio == true ? ImGui::Text("TRUE") : ImGui::Text("FALSE");

	if (ImGui::Button("Loop Change"))
		m_bLoop = !m_bLoop;
	ImGui::Text("Loop : "); ImGui::SameLine(50.f);
	m_bLoop == true ? ImGui::Text("TRUE") : ImGui::Text("FALSE");

#define X(name)#name,
	const _char* pMoveType[] = { MOVE_M "NONE" };
#undef X
	ImGui::Text("Move Selector");
	if (ImGui::BeginCombo("##Move Seletor", pMoveType[(ETOUI(m_eMove))]))
	{
		for (uint32_t i = 0; i < ETOUI(MOVE::END) + 1; ++i)
		{
			_bool bSelect = static_cast<int32_t>(m_eMove) == i;

			if (ImGui::Selectable(pMoveType[i]))
				m_eMove = static_cast<MOVE>(i);

			if (bSelect)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 0.f,0.f,0.f,1.f });
	ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.f, 0.f, 0.f, 1.f));
	uint32_t iStart = { m_iStartFlag };
	//NONE = 0x0000000, HIT = 0x0000001, ATTACK = 0x0000002, ABORT = 0x0000004, SUPERARMOR = 0x0000008, THROW = 0x0000010, DEAD = 0x0000020

//, EMISSIVE = 0x0000040
	const _char* Flag[] = { "HIT","ATTACK","ABORT","SUPERARMOR","THORW" ,"DEAD" ,"EMISSIVE" };
	if (ImGui::TreeNode("StartFlag"))
	{
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
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("EndFlag"))
	{

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

		ImGui::TreePop();
	}
	if (!m_bPopup)
	{
		for (size_t i = 0; i < ETOUI(HITMON::END); ++i)
		{
			_string Name = _string("Animation : ") + MagicEnumToStringView(static_cast<HITMON>(i)).data();
			if (ImGui::Button(Name.c_str()))
			{
				m_Value.iAnimIndex = i;
				m_bPopup = true;
				break;
			}
		}
	}

	if (m_bPopup && m_Value.iAnimIndex != -1)
	{
		ImGui::Text("Select Animation : "); ImGui::SameLine(150.f);
		ImGui::Text(MagicEnumToStringView(static_cast<HITMON>(m_Value.iAnimIndex)).data());
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1.f,1.f,1.f,1.f });
		if (CGameInstance::Get().MouseDown(MOUSEKEYSTATE::RB))
			m_bPopup = false;
		int32_t iIndex = CGameInstance::Get().GetAnimIndex(m_Handle);

		if (-1 != iIndex)
		{
			m_bPopup = false;
			m_iHitAnim[m_Value.iAnimIndex] = iIndex;
			m_Value.iAnimIndex = -1;
		}
		ImGui::PopStyleColor();
	}
	ImGui::PopStyleColor(2);
}
void CBTHitAnimMonster::Abort()
{
	m_bStart = true;
	m_iLoopCnt = 0;
}
nlohmann::json CBTHitAnimMonster::Save_Node()
{
	nlohmann::json j = __super::Save_Node();

	SaveJsonValue(j, "MoveSpeed", m_Value.fSpeed);
	SaveJsonValue(j, "Loop", m_bLoop);
	SaveJsonValue(j, "EnableRatio", m_bRatio);
	SaveJsonEnum(j, "MOVE", m_eMove);

	SaveJsonValue(j, "StartFlag", m_iStartFlag);
	SaveJsonValue(j, "EndFlag", m_iEndFlag);
	JsonSaveLoadManager::SaveJsonTypeFloat2(j, "Ratio_TypeF2", m_fRatio);

	for (uint32_t i = 0; i < ETOUI(TURN::END); ++i)
	{
		_string Name = "AnimIndex" + std::to_string(i);
		SaveJsonValue(j, Name, m_iHitAnim[i]);
	}
	return j;
}
HRESULT CBTHitAnimMonster::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "MoveSpeed", m_Value.fSpeed);
	LoadJsonValue(j, "Loop", m_bLoop);
	LoadJsonValue(j, "EnableRatio", m_bRatio);
	LoadJsonEnum(j, "MOVE", m_eMove);

	LoadJsonValue(j, "StartFlag", m_iStartFlag);
	LoadJsonValue(j, "EndFlag", m_iEndFlag);
	JsonSaveLoadManager::LoadJsonTypeFloat2(j, "Ratio_TypeF2", m_fRatio);
	for (uint32_t i = 0; i < ETOUI(TURN::END); ++i)
	{
		_string Name = "AnimIndex" + std::to_string(i);
		LoadJsonValue(j, Name, m_iHitAnim[i]);
	}
	return S_OK;
}
_bool CBTHitAnimMonster::HitType()
{
	//플레이어 공격에 따른..
	
	if (auto pBT = Get_ComBT())
	{
		if (auto pSrc = pBT->GetGameObject())
		{
			HITMON eType = static_cast<CTestGob*>(pSrc)->Get_HitMon();
			if (eType == HITMON::END)
				return false;
			m_Value.iAnimIndex = m_iHitAnim[ETOUI(eType)];
		}
	}
	return true;
}
E::UPtr<CBTHitAnimMonster> CBTHitAnimMonster::Create()
{
	auto pInstance = E::ToUPtr(new CBTHitAnimMonster{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTHitAnimMonster");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTHitAnimMonster::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTHitAnimMonster{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTHitAnimMonster");
		return nullptr;
	}

	return pInstance;
}
