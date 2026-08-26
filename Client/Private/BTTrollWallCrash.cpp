#include "pch.h"
#include "BTTrollWallCrash.h"
#include "ComAnimator.h" 
NS_USING(Client)

CBTTrollWallCrash::CBTTrollWallCrash()
{

}
CBTTrollWallCrash::CBTTrollWallCrash(const CBTTrollWallCrash& rhs) : CBTAnimRoot(rhs)
{

}

CBTTrollWallCrash::~CBTTrollWallCrash()
{
}
HRESULT CBTTrollWallCrash::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_MasterName = "BTTrollWallCrash";
	return S_OK;
}
HRESULT CBTTrollWallCrash::Initalize(void* pArg)
{
	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTTrollWallCrash::Evaluate(_float fTimeDelta)
{
	auto pBT = Get_ComBT();
	if (!pBT) return EVALUATE::FAILED;
	auto pOwner = static_cast<CMonster*>(pBT->GetGameObject());
	if (!pOwner) return EVALUATE::FAILED;
	auto pTarget = pOwner->Get_Target();
	if (!pTarget) return EVALUATE::FAILED;

	auto pTransform = (Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	auto pMoveIntent = Get_Component<CComCharacterMoveIntent>(m_Handle, "ComCharacterMoveIntent");
	auto pAnimator = (Get_Component<CComAnimator>(m_Handle, "ComCModelAnimator"));

	if (!pTransform || !pMoveIntent || !pAnimator || -1 == m_Value.iAnimIndex)
		return m_eDebug = EVALUATE::FAILED;
	if(Wall_Crash(pBT))
		return m_eDebug = EVALUATE::SUCCESS;
	_float fAnimRatio = pAnimator->GetPlayAnimRatio();

	if (m_bStart)
		pAnimator->SetPlay(true);
	if (!m_bUseCurAnim)
		pAnimator->Play_Anim(m_Value.iAnimIndex, m_bLoop, m_fBlend);

	Play_Sound(fTimeDelta);
	Rotation(pTransform, pMoveIntent, pTarget, fTimeDelta, fAnimRatio);

	_bool bFinished = pAnimator->GetFinish();
	if (m_bEarly && m_fEarlyRatio <= fAnimRatio || bFinished)
	{
		return m_eDebug = EVALUATE::SUCCESS;
	}
	if (m_bLoop || bFinished)
	{
		return m_eDebug = EVALUATE::SUCCESS;
	}

	return m_eDebug = EVALUATE::RUN;
}
void CBTTrollWallCrash::Update_Gui()
{

	BoolButton("UseCurAnim : ", m_bUseCurAnim);
	__super::Update_Gui();
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
}
void CBTTrollWallCrash::Abort()
{
	__super::Abort();
}
nlohmann::json CBTTrollWallCrash::Save_Node()
{
	nlohmann::json j = __super::Save_Node();

	SaveJsonValue(j, "UseCurAnim", m_bUseCurAnim);
	return j;
}
HRESULT CBTTrollWallCrash::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);

	LoadJsonValue(j, "UseCurAnim", m_bUseCurAnim);
	return S_OK;
}
_bool CBTTrollWallCrash::Wall_Crash(CComBeHavior* pBeHavior)
{
	return _bool();
}
void CBTTrollWallCrash::OnEnter()
{
	__super::OnEnter();

}
void CBTTrollWallCrash::OnExit(EVALUATE eResult)
{
	__super::OnExit(eResult);
}
E::UPtr<CBTTrollWallCrash> CBTTrollWallCrash::Create()
{
	auto pInstance = E::ToUPtr(new CBTTrollWallCrash{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTTrollWallCrash");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTTrollWallCrash::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTTrollWallCrash{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTTrollWallCrash");
		return nullptr;
	}

	return pInstance;
}
