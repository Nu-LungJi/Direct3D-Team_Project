#include "pch.h"
#include "BTAttackAnimation.h"
#include "ComAnimator.h" 
#include "ComCharacterMoveIntent.h"
#include "TestGob.h"
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
		_vector vDestPos = CGameInstance::Get().GetActiveCamera()->GetTransform().GetState(STATE::POSITION);
		auto pAnimator = (Get_Component<CComAnimator>(m_Handle, "ComCModelAnimator"));
		auto pTransform = (Get_Component<CComTransform>(m_Handle, "Com_Transform"));
		auto pMoveIntent = Get_Component<CComCharacterMoveIntent>(m_Handle, "ComCharacterMoveIntent");
		
		if (pTransform == nullptr || pAnimator == nullptr || pMoveIntent == nullptr ||
			-1 == m_Value.iAnimIndex)
			return m_eDebug = EVALUATE::FAILED;
		_vector vSrcPos = pTransform->GetState(STATE::POSITION);
		pAnimator->SetPlay(true);
		pAnimator->Play_Anim(m_Value.iAnimIndex, m_bLoop);

		_bool bFinished = pAnimator->GetFinish();

		//애니매이션 진행시간에 맞춰서 이동량 제어하기 m_bRatio true일 경우에만
		if (m_bRatio && m_fRatio.x <= pAnimator->GetPlayAnimRatio() && m_fRatio.y >= pAnimator->GetPlayAnimRatio())
		{
			if (m_bStart)
			{
				m_fDis = XMVectorGetX(XMVector3Length(vDestPos - vSrcPos));
				Set_Flag(m_iStartFlag, FLAGTYPE::ADD);
				m_bStart = false;
			}

			m_fTime += fTimeDelta;
	
			
			_float tt = (pAnimator->GetPlayAnimRatio() - m_fRatio.x) / (m_fRatio.y - m_fRatio.x);
			if (tt < 0.f)
				tt = 0.f;
			if (tt > 1.f)
				tt = 1.f;
			if (auto pBT = Get_ComBT())
			{
				if (auto pSrc = pBT->GetGameObject())
				{
					_float fEmissive = std::lerp(0.f,0.5f, tt);
					static_cast<CTestGob*>(pSrc)->Set_Emissive(fEmissive);
				}
			}

			_float fAnimRange = m_fRatio.y - m_fRatio.x;
			_float t = (m_fDis * pAnimator->GetPlayAnimRatio()) / (m_fRatio.y - m_fRatio.x) ;
			const _float fMoveSpeed = t * fAnimRange * m_Value.fSpeed;
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
				pMoveIntent->SetMoveIntent(vDirection, fMoveSpeed);
			}
		}

		
		if (m_bLoop || bFinished)
		{
			//Hit 종료는 애니매이션 끝나면
			//Attack도 애니매이션 끝나면
			Set_Flag(m_iEndFlag, FLAGTYPE::DEL);
			m_bStart = true;
			m_fTime = 0.f;
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
	ImGui::DragFloat("##SRaito", &m_fRatio.x, 0.f ,1.f);
	ImGui::Text("EndRatio");
	ImGui::DragFloat("##ERaito", &m_fRatio.y, 0.f, 1.f);

	if (ImGui::Button("Enable Ratio : "))
		m_bRatio = !m_bRatio;
	ImGui::Text("Abort : %s", m_bRatio ? "TRUE" : "FALSE");

	if (ImGui::Button("Loop Change"))
		m_bLoop = !m_bLoop;
	ImGui::Text("Loop : %s", m_bLoop ? "TRUE" : "FALSE");

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
	const _char* pMoveType[] = { MOVE_M "NONE"};
#undef X
	ImGui::Text("Move Selector");
	if (ImGui::BeginCombo("##Move Seletor", pMoveType[(ETOUI(m_eMove))]))
	{
		for (uint32_t i = 0; i < ETOUI(MOVE::END)+1; ++i)
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
#define X(name)#name,
	const _char* Flag[] = { BTFLAG_M };
#undef X
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

	ImGui::PopStyleColor(2);
}
void CBTAttackAnimation::Abort()
{
	m_fTime = 0.f;
	m_iLoopCnt = 0;
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
