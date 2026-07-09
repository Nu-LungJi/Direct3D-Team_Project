#include "pch.h"
#include "BTAnimation.h"
#include "ComAnimator.h" 
NS_USING(Client)

CBTAnimation::CBTAnimation()
{

}
CBTAnimation::CBTAnimation(const CBTAnimation& rhs) : CBTActionNode(rhs)
{

}

CBTAnimation::~CBTAnimation()
{
}
HRESULT CBTAnimation::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_eGroup = NODEGROUP::ANIMATION;
	m_MasterName = "BTAnimation";
	return S_OK;
}
HRESULT CBTAnimation::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	
	return S_OK;
}

EVALUATE CBTAnimation::Evaluate(_float fTimeDelta)
{
	auto pAnimator = Cast<CComAnimator>(Get_Component<CComAnimator>(m_Handle, "ComCModelAnimator"));
	if (pAnimator == nullptr || -1 == m_Value.iAnimIndex)
		return m_eDebug = EVALUATE::FAILED;
	if (m_bStart)
	{
		pAnimator->SetPlay(true);
		m_bStart = false;
	}

	pAnimator->SetPlayAnimIndex(m_Value.iAnimIndex);
	pAnimator->SetLoop(m_bLoop);
	_bool bFinished = pAnimator->GetFinish();

	if (m_bLoop)
	{
		m_bStart = true;
		return m_eDebug = EVALUATE::SUCCESS;
	}
		
	if (bFinished)
	{
		if(!m_bLoop)
			m_bStart = false;
		return m_eDebug = EVALUATE::SUCCESS;
	}
		
	return m_eDebug = EVALUATE::RUN;
}
void CBTAnimation::Update_Gui()
{

	if (ImGui::Button("Loop Change"))
		m_bLoop = !m_bLoop;
	ImGui::Text("Loop : "); ImGui::SameLine(50.f);
	m_bLoop == true ? ImGui::Text("TRUE") : ImGui::Text("FALSE");

	if (ImGui::Button("Animation"))
		m_bPopup = true;
	if (m_bPopup)
	{
		if (CGameInstance::Get().MouseDown(MOUSEKEYSTATE::RB))
			m_bPopup = false;
		int32_t iIndex = CGameInstance::Get().GetAnimIndex(m_Handle);

		if (-1 != iIndex)
		{
			m_bPopup = false;
			m_Value.iAnimIndex = iIndex;
		}
	}
}
nlohmann::json CBTAnimation::Save_Node()
{
	nlohmann::json j;
	
	j = __super::Save_Node();
	SaveJsonValue(j, "Loop", m_bLoop);
	return j;
}
HRESULT CBTAnimation::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "Loop", m_bLoop);
	return S_OK;
}
E::UPtr<CBTAnimation> CBTAnimation::Create()
{
	auto pInstance = E::ToUPtr(new CBTAnimation{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTAnimation");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTAnimation::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTAnimation{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTAnimation");
		return nullptr;
	}

	return pInstance;
}
