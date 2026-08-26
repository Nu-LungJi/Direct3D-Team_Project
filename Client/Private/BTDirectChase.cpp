#include "pch.h"
#include "BTDirectChase.h"
#include "ComAnimator.h" 
#include "BTBlackBoard.h"
#include "BlackBoardKey.h"
#include "ComCharacterMoveIntent.h"
NS_USING(Client)

CBTDirectChase::CBTDirectChase()
{

}
CBTDirectChase::CBTDirectChase(const CBTDirectChase& rhs) : CBTAnimRoot(rhs)
{

}

CBTDirectChase::~CBTDirectChase()
{
}
HRESULT CBTDirectChase::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_MasterName = "BTDirectChase";
	return S_OK;
}
HRESULT CBTDirectChase::Initalize(void* pArg)
{
	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDirectChase::Evaluate(_float fTimeDelta)
{
	auto pBT = Get_ComBT();
	if (nullptr == pBT) return m_eDebug = EVALUATE::FAILED;
	auto* pBB = pBT->Get_Blackboard();
	if (nullptr == pBB) return m_eDebug = EVALUATE::FAILED;
	auto* pSrc = pBT->GetGameObject();
	if(nullptr == pSrc) return m_eDebug = EVALUATE::FAILED;
	auto pMoveIntent = Get_Component<CComCharacterMoveIntent>(m_Handle, "ComCharacterMoveIntent");
	if(nullptr == pMoveIntent) return m_eDebug = EVALUATE::FAILED;

	auto* pTargetHandle = pBB->Get_Value<CHandle>(PUBLIC_KEY::TARGETHANDLE);
	if (nullptr == pTargetHandle) return m_eDebug = EVALUATE::FAILED;

	auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(*pTargetHandle);
	if(nullptr == pTarget) return m_eDebug = EVALUATE::FAILED;

	_vector vTargetPos = XMLoadFloat3(&pTarget->GetTransform().GetPosition());
	_vector vSrcPos	   = XMLoadFloat3(&pSrc->GetTransform().GetPosition());

	_vector vLen = vTargetPos - vSrcPos;
	_float fDistance = XMVectorGetX(XMVector3Length(vLen));
	
	if (fDistance >= m_fDist)
	{
		_float3 vDist{};
		XMStoreFloat3(&vDist, XMVector3Normalize(vLen));
		pMoveIntent->SetMoveIntent(vDist, m_fSpeed);
		pMoveIntent->SetFacingIntent(vDist, m_fAngle);
		return m_eDebug = EVALUATE::RUN;

	}
	return m_eDebug = EVALUATE::SUCCESS;
}
void CBTDirectChase::Update_Gui()
{
	__super::Update_Gui();
	if (ImGui::TreeNode("DirectChase"))
	{
		DragFloat("Speed : ", m_fSpeed, 0.1f, 0.f, 100.f);
		DragFloat("Angle : ", m_fAngle, 0.1f, -360, 360.f);
		DragFloat("Dist : ", m_fDist, 0.1f, 0, 5000.f);
		BoolButton("UseCurAnim : ", m_bUseCurAnim);
		if (ImGui::Button("Abort : "))
			m_GuiNode.bAbort = !m_GuiNode.bAbort;
		ImGui::Text("Abort : %s", m_GuiNode.bAbort ? "TRUE" : "FALSE");

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
		ImGui::TreePop();
	}
}
void CBTDirectChase::Abort()
{
	__super::Abort();
}
nlohmann::json CBTDirectChase::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonValue(j, "UseCurAnim", m_bUseCurAnim);
	SaveJsonValue(j, "Speed", m_fSpeed);
	SaveJsonValue(j, "Angle", m_fAngle);
	SaveJsonValue(j, "Distance", m_fDist);
	return j;
}
HRESULT CBTDirectChase::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);

	LoadJsonValue(j, "UseCurAnim", m_bUseCurAnim);
	LoadJsonValue(j, "Speed", m_fSpeed);
	LoadJsonValue(j, "Angle", m_fAngle);
	LoadJsonValue(j, "Distance", m_fDist);
	return S_OK;
}
void CBTDirectChase::OnEnter()
{
	__super::OnEnter();

	auto* pBT = Get_ComBT();
	if (nullptr == pBT) return;

	auto* pAnimator = Get_Component<CComAnimator>(m_Handle, "ComCModelAnimator");
	if (nullptr == pAnimator) return;

	pAnimator->Play_Anim(m_Value.iAnimIndex, m_bLoop, m_fBlend);


}
void CBTDirectChase::OnExit(EVALUATE eResult)
{
	__super::OnExit(eResult);
}
E::UPtr<CBTDirectChase> CBTDirectChase::Create()
{
	auto pInstance = E::ToUPtr(new CBTDirectChase{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDirectChase");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDirectChase::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDirectChase{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDirectChase");
		return nullptr;
	}

	return pInstance;
}
