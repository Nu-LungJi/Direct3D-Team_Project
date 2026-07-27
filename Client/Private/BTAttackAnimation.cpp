#include "pch.h"
#include "BTAttackAnimation.h"
#include "ComAnimator.h" 
#include "ComCharacterMoveIntent.h"
#include "Monster.h"
NS_USING(Client)

CBTAttackAnimation::CBTAttackAnimation()
{

}
CBTAttackAnimation::CBTAttackAnimation(const CBTAttackAnimation& rhs) : CBTAnimRoot(rhs)
{

}

CBTAttackAnimation::~CBTAttackAnimation()
{

}
HRESULT CBTAttackAnimation::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
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
	
		Active_Skill();
	
		_bool bFinished = pAnimator->GetFinish();

		_float fAnimRatio = pAnimator->GetPlayAnimRatio();
		EventFlagToRatio(fAnimRatio);

		//애니매이션 진행시간에 맞춰서 이동량 제어하기 m_bRatio true일 경우에만
		if (m_bRatio && m_fRatio.x <= fAnimRatio && m_fRatio.y >= fAnimRatio)
		{
			if (m_bStart)
			{
				m_fDis = XMVectorGetX(XMVector3Length(vDestPos - vSrcPos));
				m_bStart = false;
			}

			m_fTime += fTimeDelta;

			_float tt = (fAnimRatio - m_fRatio.x) / (m_fRatio.y - m_fRatio.x);
			if (tt < 0.f)
				tt = 0.f;
			if (tt > 1.f)
				tt = 1.f;
			if (auto pBT = Get_ComBT())
			{
				if (auto pSrc = pBT->GetGameObject())
				{
					_float fEmissive = std::lerp(0.f, 0.5f, tt);
					static_cast<CMonster*>(pSrc)->Set_Emissive(fEmissive);
				}
			}
			_float fAnimRange = m_fRatio.y - m_fRatio.x;
			_float t = (m_fDis * fAnimRatio) / (m_fRatio.y - m_fRatio.x);
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
			m_bStart = true;
			m_fTime = 0.f;
			Reset_CheckFlag();
			return m_eDebug = EVALUATE::SUCCESS;
		}
	}
	return m_eDebug = EVALUATE::RUN;
}
void CBTAttackAnimation::Update_Gui()
{
	__super::Update_Gui();
	ImGui::Text("Move Speed");
	ImGui::DragFloat("##Move Speed", &m_Value.fSpeed);

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

}
void CBTAttackAnimation::Abort()
{
	__super::Abort();
	m_fTime = 0.f;
}
nlohmann::json CBTAttackAnimation::Save_Node()
{
	nlohmann::json j = __super::Save_Node();

	SaveJsonValue(j, "MoveSpeed", m_Value.fSpeed);
	SaveJsonEnum(j, "MOVE", m_eMove);
	return j;
}
HRESULT CBTAttackAnimation::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "MoveSpeed", m_Value.fSpeed);
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
