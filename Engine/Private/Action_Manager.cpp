#include "pch.h"
#include "Action_Manager.h"
#include "BTRoot.h"
#include "BTActionNode.h"
#include "BTDecorator.h"
#include "BTSecqunce.h"
#include "BTSelector.h"
#include "ComAnimator.h"
CAction_Manager::CAction_Manager()
{
}

CAction_Manager::~CAction_Manager()
{
}

HRESULT CAction_Manager::Initialize()
{

	Add_Action_Prototype(NODEGROUP::SELECTOR, "BTSelector", CBTSelector::Create(nullptr));
	Add_Action_Prototype(NODEGROUP::SEQUENCE, "BTSequnce", CBTSecqunce::Create(nullptr));
	return S_OK;
}
void CAction_Manager::Show_Action_NodeWidget(CBTRoot* pNode)
{
	if (pNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::ACTION)
	{
		auto pAction = static_cast<CBTActionNode*>(pNode);
		pAction->Update_Gui();
	}
	else if (pNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::DECORATOR)
	{
		auto pAction = static_cast<CBTDecorator*>(pNode);
		pAction->Update_Gui();
	}
}
HRESULT CAction_Manager::Add_Action_Prototype(NODEGROUP eType, const _string& strActionName, UPtr<class CBTRoot> pAction)
{
	if (nullptr == pAction)
	{
		MSG_BOX("Create Failed to Action Proto");
		return E_FAIL;
	}

	auto iter = m_Prototype_Actions[ETOUI(eType)].find(strActionName);

	if (iter != m_Prototype_Actions[ETOUI(eType)].end())
	{
		MSG_BOX("Create Failed to Action Proto : Is Same class");
		return E_FAIL;
	}
	
	m_Prototype_Actions[ETOUI(eType)][strActionName] = std::move(pAction);

	return S_OK;
}

UPtr<class CBTRoot> CAction_Manager::Clone_Action(NODEGROUP eType, const _string& strActionName, void* pArg)
{
	auto iter = m_Prototype_Actions[ETOUI(eType)].find(strActionName);

	if (iter == m_Prototype_Actions[ETOUI(eType)].end())
	{
		MSG_BOX("Create Failed Action Clone : Is not Class");
		return nullptr;
	}

	auto pNode = iter->second->Clone(pArg);
	return std::move(pNode);
}


UPtr<class CBTRoot> CAction_Manager::Show_ActioNode_List(NODEGROUP eType, uint32_t& iNode, ImVec2 vNodePos, CHandle Handle)
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
		for (auto& iter : m_Prototype_Actions[ETOUI(eType)])
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
			const _char* pComboPreview = pNodeType[ETOUI(eType)];
			ImGui::Text("NodeType");
			if (eType != NODEGROUP::END)
			{
				ImGui::Text(m_SelectName.c_str()); ImGui::SameLine(100);
				if (ImGui::Button("Add"))
				{
					_string  FinalName = _string(pComboPreview) + " : " + m_SelectName;
					uint32_t iNodeCnt = iNode + 1;
					NodeDesc.Handle = Handle;
					NodeDesc.eGroup = eType;
					BEHAVIOR eNodeType = eType == NODEGROUP::DECORATOR ? BEHAVIOR::DECORATOR : BEHAVIOR::ACTION;
					NodeDesc.m_GuiLink = eType == NODEGROUP::DECORATOR ? (GUINODE_LINK(1)) : NodeDesc.m_GuiLink = (GUINODE_LINK(0));
					_float4 vColor = { 1.0f,1.f, 1.f,1};
					NodeDesc.m_GuiNode = GUINODE(eNodeType, iNode++, FinalName.c_str(), _float2(vNodePos.x, vNodePos.y), 0.5f, vColor);
				
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					m_bPopup = false;
					return Clone_Action(eType,m_SelectName, &NodeDesc);
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