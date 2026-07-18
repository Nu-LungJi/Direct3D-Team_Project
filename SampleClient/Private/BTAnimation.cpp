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
	auto pAnimator =(Get_Component<CComAnimator>(m_Handle, "ComCModelAnimator"));
	
	if (pAnimator == nullptr || -1 == m_Value.iAnimIndex)
		return m_eDebug = EVALUATE::FAILED;
	if (m_bStart)
		pAnimator->SetPlay(true);
	
	pAnimator->Play_Anim(m_Value.iAnimIndex, m_bLoop);
	_bool bFinished = pAnimator->GetFinish();

	if (m_bLoop || bFinished)
	{

		Set_Flag(m_iEndFlag, FLAGTYPE::DEL);
		return m_eDebug = EVALUATE::SUCCESS;
	}

	return m_eDebug = EVALUATE::RUN;
}
void CBTAnimation::Update_Gui()
{
	if (ImGui::Button("Abort : ")) 
		m_GuiNode.bAbort = !m_GuiNode.bAbort;
	ImGui::Text("Abort : %s", m_GuiNode.bAbort ? "TRUE" : "FALSE");

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

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 0,0,0,1 });
	ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.f, 0.f, 0.f, 1.f));
	
//NONE = 0x0000000, HIT = 0x0000001, ATTACK = 0x0000002, ABORT = 0x0000004, SUPERARMOR = 0x0000008, THROW = 0x0000010, DEAD = 0x0000020
	
//, EMISSIVE = 0x0000040
#define X(name)#name,
	const _char* Flag[] = { BTFLAG_M };
#undef X
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
void CBTAnimation::Abort()
{
	m_iLoopCnt = 0;
}
nlohmann::json CBTAnimation::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	
	SaveJsonValue(j, "Loop", m_bLoop);
	SaveJsonValue(j ,"EndFlag", m_iEndFlag);
	return j;
}
HRESULT CBTAnimation::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "Loop", m_bLoop);
	LoadJsonValue(j, "EndFlag", m_iEndFlag);
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
