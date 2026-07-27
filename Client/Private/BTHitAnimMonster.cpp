#include "pch.h"
#include "BTHitAnimMonster.h"
#include "ComAnimator.h" 
#include "ComCharacterMoveIntent.h"
NS_USING(Client)

CBTHitAnimMonster::CBTHitAnimMonster()
{

}
CBTHitAnimMonster::CBTHitAnimMonster(const CBTHitAnimMonster& rhs) : CBTAnimRoot(rhs)
{

}

CBTHitAnimMonster::~CBTHitAnimMonster()
{
	
}
HRESULT CBTHitAnimMonster::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
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
		auto pAnimator   = (Get_Component<CComAnimator>(m_Handle, "ComCModelAnimator"));
		auto pTransform  = (Get_Component<CComTransform>(m_Handle, "Com_Transform"));
		auto pMoveIntent =  Get_Component<CComCharacterMoveIntent>(m_Handle, "ComCharacterMoveIntent");

		if (pTransform == nullptr || pAnimator == nullptr || pMoveIntent == nullptr)
			return m_eDebug = EVALUATE::FAILED;
		if (m_bStart)
		{
			//type 없으면 그냥 넘어가기
			if (false == HitType())
				return m_eDebug = EVALUATE::SUCCESS;
			m_bStart = false;
		}
		
		pAnimator->SetPlay(true);
		pAnimator->Play_Anim(m_Value.iAnimIndex, m_bLoop);
		Active_Skill();
		_bool bFinished = pAnimator->GetFinish();

		//애니매이션 진행시간에 맞춰서 이동량 제어하기 m_bRatio true일 경우에만
		if (m_bRatio && m_fRatio.x <= pAnimator->GetPlayAnimRatio() && m_fRatio.y >= pAnimator->GetPlayAnimRatio())
		{
			_vector vMoveDirection{};
			if (m_eMove == MOVE::RIGHT)
				vMoveDirection = pTransform->GetState(STATE::RIGHT);
			else if (m_eMove == MOVE::LEFT)
				vMoveDirection = -pTransform->GetState(STATE::RIGHT);
			else if (m_eMove == MOVE::STRAIGHT)
				vMoveDirection = pTransform->GetState(STATE::LOOK);
			else if (m_eMove == MOVE::BACKWARD)
				vMoveDirection = -pTransform->GetState(STATE::LOOK);

			if (m_eMove != MOVE::END)
			{
				_float3 vDirection{};
				XMStoreFloat3(&vDirection, vMoveDirection);
				pMoveIntent->SetMoveIntent(vDirection, m_Value.fSpeed);
			}
		}
		EventFlagToRatio(pAnimator->GetPlayAnimRatio());

		if (m_bLoop || bFinished)
		{
			m_bStart = true;
			return m_eDebug = EVALUATE::SUCCESS;
		}
	}
	return m_eDebug = EVALUATE::RUN;
}
void CBTHitAnimMonster::Update_Gui()
{
	__super::Update_Gui();
	ImGui::Text("Move Speed");
	ImGui::DragFloat("##Move Speed", &m_Value.fSpeed);

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
	
	if (!m_bPopup)
	{
		for (size_t i = 0; i < ETOUI((PLAYER_SKILL_TYPE::END)); ++i)
		{

			_string Name = _string("Animation : ");
			if (ImGui::Button(Name.c_str()))
			{
				m_Value.iAnimIndex = i;
				m_bPopup = true;
				break;
			}
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1.f,0.f,0.f,1.f });
			_string AttName = _string("##AttType : ") + MagicEnumToStringView(static_cast<PLAYER_SKILL_TYPE>(i)).data();
			if (ImGui::BeginCombo(AttName.c_str(), MagicEnumToStringView(m_HitTable[i].eAttType).data()))
			{
				for (uint32_t j = 0; j < ETOUI(ATTMON::END) +1; ++j)
				{
					_bool	bSelect = m_HitTable[i].eAttType == static_cast<ATTMON>(j);
					ImGui::PushID(MagicEnumToStringView(static_cast<ATTMON>(j)).data());
					if (ImGui::Selectable(MagicEnumToStringView(static_cast<ATTMON>(j)).data(), bSelect))
					{
						m_HitTable[i].eAttType = static_cast<ATTMON>(j);
					}
					if (bSelect)
						ImGui::SetItemDefaultFocus();

					ImGui::PopID();
				}

				ImGui::EndCombo();
			}
			ImGui::SameLine();

			_string HittName = _string("##HitType : ") + MagicEnumToStringView(static_cast<PLAYER_SKILL_TYPE>(i)).data();
			if (ImGui::BeginCombo(HittName.c_str(), MagicEnumToStringView(m_HitTable[i].eHitType).data()))
			{
				for (uint32_t j = 0; j < ETOUI(PLAYER_SKILL_TYPE::END); ++j)
				{
					_bool	bSelect = m_HitTable[i].eHitType == static_cast<PLAYER_SKILL_TYPE>(j);
					ImGui::PushID(MagicEnumToStringView(static_cast<PLAYER_SKILL_TYPE>(j)).data());
					if (ImGui::Selectable(MagicEnumToStringView(static_cast<PLAYER_SKILL_TYPE>(j)).data(), bSelect))
					{
						m_HitTable[i].eHitType = static_cast<PLAYER_SKILL_TYPE>(j);
					}
					if (bSelect)
						ImGui::SetItemDefaultFocus();
					ImGui::PopID();
				}

				ImGui::EndCombo();
			}
			ImGui::PopStyleColor();
		}
	}

	if (m_bPopup && m_Value.iAnimIndex != -1)
	{
		ImGui::Text("Select Animation : "); ImGui::SameLine(150.f);
		ImGui::Text(MagicEnumToStringView(static_cast<PLAYER_SKILL_TYPE>(m_Value.iAnimIndex)).data());
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
	__super::Abort();
}
nlohmann::json CBTHitAnimMonster::Save_Node()
{
	nlohmann::json j = __super::Save_Node();

	SaveJsonValue(j, "MoveSpeed", m_Value.fSpeed);
	SaveJsonEnum(j, "MOVE", m_eMove);

	for (uint32_t i = 0; i < ETOUI(PLAYER_SKILL_TYPE::END); ++i)
	{
		_string Name = "AnimIndex" + std::to_string(i);
		_string HitName = "HITType" + std::to_string(i);
		SaveJsonValue(j, Name, m_iHitAnim[i]);
		SaveJsonEnum(j, HitName, m_HitTable[i].eHitType);
	}
	for (uint32_t i = 0; i < ETOUI(ATTMON::END); ++i)
	{
		_string Name = "ATTType" + std::to_string(i);
		SaveJsonEnum(j, Name, m_HitTable[i].eAttType);
	}
	return j;
}
HRESULT CBTHitAnimMonster::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "MoveSpeed", m_Value.fSpeed);
	LoadJsonEnum(j, "MOVE", m_eMove);

	for (uint32_t i = 0; i < ETOUI(PLAYER_SKILL_TYPE::END); ++i)
	{
		_string Name = "AnimIndex" + std::to_string(i);
		_string HitName = "HITType" + std::to_string(i);
		LoadJsonValue(j, Name, m_iHitAnim[i]);
		LoadJsonEnum(j, HitName, m_HitTable[i].eHitType);
	}
	for (uint32_t i = 0; i < ETOUI(ATTMON::END); ++i)
	{
		_string Name = "ATTType" + std::to_string(i);
		LoadJsonEnum(j, Name, m_HitTable[i].eAttType);
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
			
			for (uint32_t i = 0; i < ETOUI(PLAYER_SKILL_TYPE::END); ++i)
			{
				if (m_HitTable[i] == static_cast<CMonster*>(pSrc)->Get_HitTable())
				{
					m_Value.iAnimIndex = m_iHitAnim[i];
					return true;
				}
			}
		}
	}
	return false;
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
