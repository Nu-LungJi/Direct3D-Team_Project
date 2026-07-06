#include "pch.h"
#include "Action_Manager.h"
#include "BTRoot.h"
#include "BTActionNode.h"
#include "ComAnimator.h"
CAction_Manager::CAction_Manager()
{
}

CAction_Manager::~CAction_Manager()
{
}

HRESULT CAction_Manager::Initialize()
{
	return S_OK;
}
void CAction_Manager::Show_Action_NodeWidget(CBTRoot* pNode)
{
	auto pAction = static_cast<CBTActionNode*>(pNode);
	NODE_ACTION eActionType = pAction->Get_Value().eNodeType;
	CHandle     Handle = pAction->Get_Handle();
	
	if (eActionType == NODE_ACTION::MOVE)
	{
		
	}
	else if (eActionType == NODE_ACTION::ANIMATION)
	{
		if (auto pAnimatior = Cast<CComAnimator>(pAction->Get_Component<CComAnimator>(Handle, "Com_Animation")))
			PopupAnimation(pAnimatior,pAction);
	}

}
HRESULT CAction_Manager::Add_Action_Prototype(const _string& strActionName, UPtr<class CBTRoot> pAction)
{
	if (nullptr == pAction)
	{
		MSG_BOX("Create Failed to Action Proto");
		return E_FAIL;
	}

	auto iter = m_Prototype_Actions.find(strActionName);

	if (iter != m_Prototype_Actions.end())
	{
		MSG_BOX("Create Failed to Action Proto : Is Same class");
		return E_FAIL;
	}
	
	m_Prototype_Actions[strActionName] = std::move(pAction);

	return S_OK;
}

UPtr<class CBTRoot> CAction_Manager::Clone_Action(const _string& strActionName, void* pArg)
{
	auto iter = m_Prototype_Actions.find(strActionName);

	if (iter == m_Prototype_Actions.end())
	{
		MSG_BOX("Create Failed Action Clone : Is not Class");
		return nullptr;
	}

	auto pNode = iter->second->Clone(pArg);
	return std::move(pNode);
}

void CAction_Manager::PopupAnimation(CComAnimator* pAnimator, CBTActionNode* pNode)
{
	//int32_t iIndex = -1;
	//if (iIndex = CGameInstance::Get().)
	//{
	//	pNode->Get_Value().iAnimIndex = iIndex;
	//}
}

UPtr<class CBTRoot> CAction_Manager::Show_ActioNode_List(uint32_t& iNode, ImVec2 vNodePos, CHandle Handle)
{

	_char Name[32]{};
	_char NameBuffer[32]{};
	UPtr<CBTRoot>  pNode{ nullptr };
	CBTActionNode::ACTION_NODE_DESC NodeDesc{};
	ImGui::OpenPopup("Action Node List");
	
	if (ImGui::BeginPopup("Action Node List", ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (ImGui::Button("Cancle"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::Text("Action Name : ");
		if(!m_bPopup)
		for (auto& iter : m_Prototype_Actions)
		{
			if (ImGui::Button(iter.first.c_str()))
			{
				m_SelectName = iter.first;
				m_bPopup = true;
			}
		}
		else
		{
#define  X(name)#name,
			const _char* pNodeType[] = {NODE_ACTION_M};
#undef X
			static NODE_ACTION eNode = NODE_ACTION::END;
			const _char* pComboPreview = pNodeType[ETOUI(eNode)];
			ImGui::Text("NodeType");
			if (ImGui::BeginCombo("##", pComboPreview, ImGuiComboFlags_PopupAlignLeft))
			{
				for (uint32_t i = 0; i < ETOUI(NODE_ACTION::END); ++i)
				{
					const _bool bSelect = (ETOUI(eNode) == i);
					if (ImGui::Selectable(pNodeType[i], bSelect))
						eNode = static_cast<NODE_ACTION>(i);
					if (bSelect)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			if (eNode != NODE_ACTION::END)
			{
				ImGui::Text(m_SelectName.c_str()); ImGui::SameLine(100);
				if (ImGui::Button("Add"))
				{
					_string  FinalName = _string(pComboPreview) + " : " + m_SelectName;
					uint32_t iNodeCnt = iNode + 1;
					NodeDesc.Value = ACTION_VALUE(-1,eNode);
					NodeDesc.Handle = Handle;
					NodeDesc.m_GuiNode = GUINODE(BEHAVIOR::ACTION, iNode++, FinalName.c_str(), _float2(vNodePos.x, vNodePos.y), 0.5f, _float4(100, 100, 200, 255));
					NodeDesc.m_GuiLink = (GUINODE_LINK(0));
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					return Clone_Action(m_SelectName, &NodeDesc);
				}
			}
		}

		ImGui::EndPopup();
	}
	return nullptr;
}

UPtr<CAction_Manager> CAction_Manager::Create()
{
	auto pInstance = ToUPtr(new CAction_Manager());
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Create Failed : CAction_Manager");
		return nullptr;
	}
	return pInstance;

}