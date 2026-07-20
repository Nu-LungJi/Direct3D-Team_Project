#include "pch.h"
#include "BTRandMoveAnim.h"
#include "ComTransform.h" 
#include "ComAnimator.h"
#include "ComCharacterMoveIntent.h"
NS_USING(Client)

CBTRandMoveAnim::CBTRandMoveAnim()
{

}
CBTRandMoveAnim::CBTRandMoveAnim(const CBTRandMoveAnim& rhs) : CBTActionNode(rhs)
{

}

CBTRandMoveAnim::~CBTRandMoveAnim()
{
}
HRESULT CBTRandMoveAnim::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::ANIMATION;
	m_MasterName = "BTRandMoveAnim";
	return S_OK;
}
HRESULT CBTRandMoveAnim::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	return S_OK;
}

void CBTRandMoveAnim::Abort()
{
	m_bInit = false;
}

nlohmann::json CBTRandMoveAnim::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	

	SaveJsonValue(j, "Clamp", m_fClamp);
	SaveJsonValue(j, "Distance", m_fDis);
	return j;
}

HRESULT CBTRandMoveAnim::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "Distance", m_fDis);
	LoadJsonValue(j, "Clamp", m_fClamp);
	return S_OK;
}

EVALUATE CBTRandMoveAnim::Evaluate(_float fTimeDelta)
{
	auto pTransform = (Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	auto pAnimator = (Get_Component<CComAnimator>(m_Handle, "ComCModelAnimator"));

	EVALUATE eType{};
	if (pTransform == nullptr || pAnimator == nullptr)
		return m_eDebug =  EVALUATE::FAILED;
	if (!m_bInit)
	{
		RandomDirSelect();
		m_bInit = true;
	}
	pAnimator->Play_Anim(m_Value.iAnimIndex, true);
	_bool bFinished = pAnimator->GetFinish();

	if (m_GuiNode.bAbort)
	{
		//애니매이션 겹침 방지
		//Set_Flag(ETOUI(BTFLAG::ABORT), FLAGTYPE::ADD);
	}
	eType = Move(fTimeDelta);
	return m_eDebug = eType;
}
void CBTRandMoveAnim::RandomDirSelect()
{
	uint32_t iRand = rand() % 2;
	_vector  vCurDir{}, vCurPos{};
	_matrix mat = XMMatrixIdentity();

	if (auto iter = Get_ComBT())
	{
		if (auto pObj = iter->GetGameObject())
		{
			vCurDir = XMVector3Normalize(pObj->GetTransform().GetState(STATE::LOOK));
			vCurPos = pObj->GetTransform().GetState(STATE::POSITION);
			
		}
	}
	switch (iRand)
	{
	case 0:
		vCurDir = XMVector3TransformNormal(vCurDir, XMMatrixRotationY(XMConvertToRadians(45)));
		break;
	case 1:
		vCurDir = XMVector3TransformNormal(vCurDir, XMMatrixRotationY(XMConvertToRadians(-45)));
		break;
	}

	XMStoreFloat3(&m_vFinishPos ,vCurPos - vCurDir * m_fDis);
	XMStoreFloat3(&m_vDir, vCurDir);
}
EVALUATE CBTRandMoveAnim::Move(_float fTimeDelta)
{
	auto pTransform = (Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	auto pMoveIntent = Get_Component<CComCharacterMoveIntent>(m_Handle, "ComCharacterMoveIntent");
	if (pTransform == nullptr || pMoveIntent == nullptr)
		return m_eDebug = EVALUATE::FAILED;

	_vector vCurPos = pTransform->GetState(STATE::POSITION);
	_float fDis = XMVectorGetX(XMVector3Length(vCurPos - XMLoadFloat3(&m_vFinishPos)));

	_float fMove = m_fClamp * fTimeDelta;
	if (fDis <= fMove)
	{
		m_bInit = false;
		return m_eDebug = EVALUATE::SUCCESS;
	}

	const _float3 vMoveDirection{ -m_vDir.x, 0.f, -m_vDir.z };
	pMoveIntent->SetMoveIntent(vMoveDirection, m_fClamp);
	return m_eDebug = EVALUATE::RUN;
}
void CBTRandMoveAnim::Update_Gui()
{
	ImGui::Text("Distance");
	ImGui::DragFloat("##Distance", &m_fDis, 0, 100);

	ImGui::Text("Clamp");
	ImGui::DragFloat("##Clamp", &m_fClamp, 0, 1);
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
}
E::UPtr<CBTRandMoveAnim> CBTRandMoveAnim::Create()
{
	auto pInstance = E::ToUPtr(new CBTRandMoveAnim{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTRandMoveAnim");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTRandMoveAnim::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTRandMoveAnim{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTRandMoveAnim");
		return nullptr;
	}

	return pInstance;
}
