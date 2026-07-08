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
HRESULT CBTAnimation::InitalizePrototype(void* pArg)
{
	__super::InitalizePrototype(pArg);
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
	if (pAnimator != nullptr && -1 != m_Value.iAnimIndex)
		pAnimator->SetPlayAnimIndex(m_Value.iAnimIndex);
	if(pAnimator->GetPlay())
		return EVALUATE::SUCCESS;

	return EVALUATE::FAILED;
}
void CBTAnimation::Update_Gui()
{
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
E::UPtr<CBTAnimation> CBTAnimation::Create()
{
	auto pInstance = E::ToUPtr(new CBTAnimation{});
	if (FAILED(pInstance->InitalizePrototype()))
	{
		MSG_BOX("Failed to Created : CBTAnimation");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CBTRoot> CBTAnimation::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTAnimation{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTAnimation");
		return nullptr;
	}

	return pInstance;
}
