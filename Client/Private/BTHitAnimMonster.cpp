#include "pch.h"
#include "BTHitAnimMonster.h"
#include "ComAnimator.h" 
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
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
		auto pAnimator = (Get_Component<CComAnimator>(m_Handle, "ComCModelAnimator"));
		auto pTransform = (Get_Component<CComTransform>(m_Handle, "Com_Transform"));
		auto pMoveIntent = Get_Component<CComCharacterMoveIntent>(m_Handle, "ComCharacterMoveIntent");

		if (pTransform == nullptr || pAnimator == nullptr || pMoveIntent == nullptr)
			return m_eDebug = EVALUATE::FAILED;


		if (auto pSrc = static_cast<CMonster*>(pBT->GetGameObject()))
		{
			if (m_bStart)
			{
				m_iHitCnt = pSrc->GetHitCnt();
				//type 없으면 그냥 넘어가기
				if (false == HitType())
					return m_eDebug = EVALUATE::FAILED;
				m_bStart = false;

				if (m_bResetAnimTime)
					pAnimator->GetCurAnimState().fTrackPosition = 0.f;
			}

			if (m_iHitCnt != pSrc->GetHitCnt())
			{
				m_bStart = true;
				Reset_CheckFlag();
				return m_eDebug = EVALUATE::FAILED;
			}

		}
		Gravity();
		pAnimator->SetPlay(true);
		//현재 애니매이션 유지할건지
		if (!m_bUseCurAnim)
		{

			pAnimator->Play_Anim(m_Value.iAnimIndex, m_bLoop, m_fBlend);

		}
		
		_float fAnimRatio = pAnimator->GetPlayAnimRatio();
		_bool bFinished = pAnimator->GetFinish();

		//애니매이션 진행시간에 맞춰서 이동량 제어하기 m_bRatio true일 경우에만
		if (m_bRatio && m_fRatio.x <= fAnimRatio && m_fRatio.y >= fAnimRatio)
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
			else if (m_eMove == MOVE::UP)
				vMoveDirection = XMVectorSet(0, 1, 0, 0);
			else if (m_eMove == MOVE::DOWN)
				vMoveDirection = XMVectorSet(0, -1, 0, 0);

			if (m_eMove != MOVE::END)
			{
				_float3 vDirection{};
				XMStoreFloat3(&vDirection, vMoveDirection);
				pMoveIntent->SetMoveIntent(vDirection, m_Value.fSpeed);
			}
		}
		EventFlagToRatio(fAnimRatio);

		if (m_bEarly && m_fEarlyRatio <= fAnimRatio || bFinished)
		{
			Reset_CheckFlag();
			m_bStart = true;
			return m_eDebug = EVALUATE::SUCCESS;
		}

	}
	//if (m_bUseCurAnim)
	//{
	//	Reset_CheckFlag();
	//	return m_eDebug = EVALUATE::SUCCESS;
	//}
	return m_eDebug = EVALUATE::RUN;
}
void CBTHitAnimMonster::Update_Gui()
{
	__super::Update_Gui();
	DragFloat("Move Speed", m_Value.fSpeed);
	BoolButton("ResetAnimtime : ", m_bResetAnimTime);
	BoolButton("UseCurAnim : ", m_bUseCurAnim);
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

	if (ImGui::Button("AddTable"))
		m_HitTable.push_back(HITTABLE());

	if (ImGui::TreeNode("HitAnimTableList"))
	{
		uint32_t i = 0;
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1.f,0.f,0.f,1.f });
		for (auto iter  = m_HitTable.begin(); iter != m_HitTable.end();++iter)
		{
			ImGui::PushID(i);
			ImGui::Text("AttType : "); ImGui::SameLine();
			ComboAttMon("##ATT",(*iter).eAttType); ImGui::SameLine();
			
			ImGui::Text("HitType : "); ImGui::SameLine();
			ComboHit("##Hit", (*iter).eHitType); ImGui::SameLine();
		
			ComboAnim("Animation",(*iter).iAnimIndex,i); ImGui::SameLine();

			ImGui::DragFloat("Blend", &(*iter).fBlend, 0.1f, 0.f, 1.f);  ImGui::SameLine();
			
			if (ImGui::Button("Del"))
			{
				m_HitTable.erase(iter);
				ImGui::PopID();
				break;
			}
			ImGui::PopID();
			++i;
		}

		ImGui::PopStyleColor();
		ImGui::TreePop();
	}

}
void CBTHitAnimMonster::Abort()
{
	__super::Abort();
}
nlohmann::json CBTHitAnimMonster::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonValue(j, "UseCurAnim", m_bUseCurAnim);
	SaveJsonValue(j, "MoveSpeed", m_Value.fSpeed);

	SaveJsonValue(j, "AnimTime", m_bResetAnimTime);
	SaveJsonEnum(j, "MOVE", m_eMove);
	
	if (!m_HitTable.empty())
	{
		size_t iArray = m_HitTable.size();
		SaveJsonValue(j,"HitTableArrayCnt", iArray);

		for (size_t i=0; i < m_HitTable.size(); ++i)
		{
			_string AnimIndexName = "HitTableAnimIndex" + std::to_string(i);
			_string AttTypeName = "AttType" + std::to_string(i);
			_string HitTypeName = "HitType" + std::to_string(i);
			_string BlendName = "FBlend" + std::to_string(i);

			SaveJsonValue(j, AnimIndexName, m_HitTable[i].iAnimIndex);
			SaveJsonValue(j, BlendName, m_HitTable[i].fBlend);
			SaveJsonEnum(j, AttTypeName, m_HitTable[i].eAttType);
			SaveJsonEnum(j, HitTypeName, m_HitTable[i].eHitType);

		}
	}

	return j;
}
HRESULT CBTHitAnimMonster::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);

	LoadJsonValue(j, "UseCurAnim", m_bUseCurAnim);
	LoadJsonValue(j, "MoveSpeed", m_Value.fSpeed);
	LoadJsonEnum(j, "MOVE", m_eMove);
	LoadJsonValue(j, "AnimTime", m_bResetAnimTime);
	size_t iArray = 0;

	if(LoadJsonValue(j, "HitTableArrayCnt", iArray))
	{
		m_HitTable.resize(iArray);
		for (size_t i = 0; i < iArray; ++i)
		{
			_string AnimIndexName = "HitTableAnimIndex" + std::to_string(i);
			_string AttTypeName = "AttType" + std::to_string(i);
			_string HitTypeName = "HitType" + std::to_string(i);
			_string BlendName = "FBlend" + std::to_string(i);

			LoadJsonValue(j, BlendName, m_HitTable[i].fBlend);
			LoadJsonValue(j, AnimIndexName, m_HitTable[i].iAnimIndex);
			LoadJsonEnum(j,  AttTypeName, m_HitTable[i].eAttType);
			LoadJsonEnum(j,  HitTypeName, m_HitTable[i].eHitType);
		}
	}
	return S_OK;
}
_bool CBTHitAnimMonster::HitType()
{
	//플레이어 공격에 따른..
	
	if (auto pBT = Get_ComBT())
	{
		if (auto pSrc = static_cast<CMonster*>(pBT->GetGameObject()))
		{
			if (!pSrc->Is_ActiveHit())
				return false;

			MON_HIT_INFO info = pSrc->Get_ActiveHitInfo();

			auto iter = std::find_if(m_HitTable.begin(), m_HitTable.end(), [&](const HITTABLE& table)
			{
					return table.eAttType == info.eAttType && table.eHitType == info.eHitType;
			});
			 
			if (iter == m_HitTable.end())
			{
				iter = std::find_if(m_HitTable.begin(), m_HitTable.end(), [&](const HITTABLE& table)
					{
						return table.eAttType == ATTMON::SKIP && table.eHitType == info.eHitType;
					});

			}
			if (iter == m_HitTable.end())
				return false;

			if (iter != m_HitTable.end())
			{
				m_Value.iAnimIndex = (*iter).iAnimIndex;
				m_fBlend = (*iter).fBlend;
				
				return true;
			}
			else
				return false;
		}
	}
	return false;
}

void CBTHitAnimMonster::ComboAttMon(const _char* pName, ATTMON& eTye)
{
	if (auto pBT = Get_ComBT())
	{
		if (auto pSrc = static_cast<CMonster*>(pBT->GetGameObject()))
		{
			if (ImGui::BeginCombo(pName, MagicEnumToStringView(eTye).data()))
			{
				for (uint32_t j = 0; j < ETOUI(ATTMON::END) + 1; ++j)
				{
					_string SkillName = pSrc->Get_SkillName(static_cast<ATTMON>(j));
					if (SkillName == "")
						continue;
					_bool	bSelect = eTye == static_cast<ATTMON>(j);
					ImGui::PushID(SkillName.data());
					if (ImGui::Selectable(SkillName.data(), bSelect))
						eTye = static_cast<ATTMON>(j);

					if (bSelect)
						ImGui::SetItemDefaultFocus();

					ImGui::PopID();
				}
				ImGui::EndCombo();
			}
			
		}
	}
}
void CBTHitAnimMonster::ComboHit(const _char* pName, PLAYER_SKILL_TYPE& eTye)
{
	if (ImGui::BeginCombo(pName, MagicEnumToStringView(eTye).data()))
	{
		for (uint32_t j = 0; j < ETOUI(PLAYER_SKILL_TYPE::END); ++j)
		{
			_bool	bSelect = eTye == static_cast<PLAYER_SKILL_TYPE>(j);
			ImGui::PushID(MagicEnumToStringView(static_cast<PLAYER_SKILL_TYPE>(j)).data());
			if (ImGui::Selectable(MagicEnumToStringView(static_cast<PLAYER_SKILL_TYPE>(j)).data(), bSelect))
			{
				eTye = static_cast<PLAYER_SKILL_TYPE>(j);
			}
			if (bSelect)
				ImGui::SetItemDefaultFocus();
			ImGui::PopID();
		}

		ImGui::EndCombo();
	}
}
void CBTHitAnimMonster::ComboAnim(const _char* pName, int32_t& iAnimIndex, uint32_t iArrayIndex)
{
	if (!m_bPopup)
	{
		if (ImGui::Button(pName))
		{
			m_iArrayIndex= iArrayIndex;

			m_bPopup = true;
		}
	}

	if (m_bPopup && m_iArrayIndex == iArrayIndex)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1.f,1.f,1.f,1.f });
		if (CGameInstance::Get().MouseDown(MOUSEKEYSTATE::RB))
			m_bPopup = false;
		int32_t iIndex = CGameInstance::Get().GetAnimIndex(m_Handle);

		if (-1 != iIndex)
		{
			m_bPopup = false;
			iAnimIndex = iIndex;
			m_iArrayIndex = UINT_MAX;
		}
		ImGui::PopStyleColor();
	}
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
