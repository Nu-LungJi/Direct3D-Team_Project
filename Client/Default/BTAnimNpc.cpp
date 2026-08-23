#include "pch.h"
#include "BTAnimNpc.h"
#include "ComAnimator.h" 
#include "ComCharacterMoveIntent.h"
#include "NpcMom.h"
#include "Player.h"
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
#include "ClientEvents.h" 
NS_USING(Client)

CBTAnimNpc::CBTAnimNpc()
{

}
CBTAnimNpc::CBTAnimNpc(const CBTAnimNpc& rhs) : CBTAnimRoot(rhs)
{

}

CBTAnimNpc::~CBTAnimNpc()
{

}
HRESULT CBTAnimNpc::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_MasterName = "BTAnimNpc";
	
	return S_OK;
}
HRESULT CBTAnimNpc::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	m_eUser = BT_USER::NPC;
	return S_OK;
}

EVALUATE CBTAnimNpc::Evaluate(_float fTimeDelta)
{
	auto pBT = Get_ComBT();
	if (nullptr == pBT) return m_eDebug = EVALUATE::FAILED;
	
	auto pOwner = static_cast<CNpcMom*>(pBT->GetGameObject());
	if(nullptr == pOwner) return m_eDebug = EVALUATE::FAILED;
		
	auto pTarget = pOwner->Get_Target();
	auto pBB = pOwner->Get_BlackBoard();
	if(nullptr == pTarget || nullptr == pBB) return m_eDebug = EVALUATE::FAILED;

	_vector vDestPos = pTarget->GetTransform().GetState(STATE::POSITION);
	auto pAnimator = (Get_Component<CComAnimator>(m_Handle, "ComCModelAnimator"));
	auto pTransform = (Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	auto pMoveIntent = Get_Component<CComCharacterMoveIntent>(m_Handle, "ComCharacterMoveIntent");
	
	if (pTransform == nullptr || pAnimator == nullptr || pMoveIntent == nullptr ||
		-1 == m_Value.iAnimIndex)
		return m_eDebug = EVALUATE::FAILED;
	
	_vector vSrcPos = pTransform->GetState(STATE::POSITION);
	pAnimator->SetPlay(true);
	pAnimator->Play_Anim(m_Value.iAnimIndex, m_bLoop, m_fBlend);
	if (m_bStart)
	{
		m_fDis = XMVectorGetX(XMVector3Length(vDestPos - vSrcPos));
		m_bStart = false;
	}
	
	_float fAnimRatio = pAnimator->GetPlayAnimRatio();

	Gravity();
	Play_Sound(fTimeDelta);
	_bool bFinished = pAnimator->GetFinish();
	EventFlagToRatio(fAnimRatio);
	Rotation(pTransform, pMoveIntent, pTarget, fTimeDelta, fAnimRatio);
	//애니매이션 진행시간에 맞춰서 이동량 제어하기 m_bRatio true일 경우에만
	if (m_bRatio && m_fRatio.x <= fAnimRatio && m_fRatio.y >= fAnimRatio)
	{
		m_fTime += fTimeDelta;
	
		_float tt = (fAnimRatio - m_fRatio.x) / (m_fRatio.y - m_fRatio.x);
		if (tt < 0.f)
			tt = 0.f;
		if (tt > 1.f)
			tt = 1.f;
	
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
		else if (m_eMove == MOVE::UP)
			vMoveDirection = XMVectorSet(0, 1, 0, 0);
	
		if (m_eMove != MOVE::END)
		{
			_float3 vDirection{};
			XMStoreFloat3(&vDirection, vMoveDirection);
			pMoveIntent->SetMoveIntent(vDirection, fMoveSpeed);
		}
	}
	if (m_bEarly && m_fEarlyRatio <= fAnimRatio)
		return m_eDebug = EVALUATE::SUCCESS;
	
	if (m_bLoop || bFinished)
		return m_eDebug = EVALUATE::SUCCESS;
	
	return m_eDebug = EVALUATE::RUN;

}
void CBTAnimNpc::Update_Gui()
{
	__super::Update_Gui();
	if (ImGui::TreeNode("Anim"))
	{
		BoolButton("AnimToBB", m_bBBAnim);
		DragFloat("Move Speed", m_Value.fSpeed);
		DragFloat("RotTime", m_Value.fTime);
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
		ImGui::TreePop();
	}

}
void CBTAnimNpc::Abort()
{
	__super::Abort();
	Reset_CheckFlag();

}
nlohmann::json CBTAnimNpc::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonValue(j, "MoveSpeed", m_Value.fSpeed);
	SaveJsonEnum(j, "MOVE", m_eMove);

	
	return j;
}
HRESULT CBTAnimNpc::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "MoveSpeed", m_Value.fSpeed);
	LoadJsonEnum(j, "MOVE", m_eMove);
	return S_OK;
}


void CBTAnimNpc::OnEnter()
{
	__super::OnEnter();
	m_fTime = 0.f;




}
void CBTAnimNpc::OnExit(EVALUATE eResult)
{
	__super::OnExit(eResult);
}
E::UPtr<CBTAnimNpc> CBTAnimNpc::Create()
{
	auto pInstance = E::ToUPtr(new CBTAnimNpc{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTAnimNpc");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTAnimNpc::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTAnimNpc{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTAnimNpc");
		return nullptr;
	}

	return pInstance;
}
