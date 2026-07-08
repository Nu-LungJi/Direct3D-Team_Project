#include "pch.h"
#include "BTRoot.h"

CBTRoot::CBTRoot()
{
}

CBTRoot::CBTRoot(const CBTRoot& rhs) : CEngineBase(rhs)
{
	m_eGroup = rhs.m_eGroup;
	m_MasterName = rhs.m_MasterName;
}

CBTRoot::~CBTRoot()
{
}

HRESULT CBTRoot::InitalizePrototype(void* pArg)
{
	
	return S_OK;
}

HRESULT CBTRoot::Initalize(void* pArg)
{
	auto pDesc = static_cast<BTROOT_DESC*>(pArg);
	if (nullptr != pDesc)
	{
		m_Handle = pDesc->Handle;
		m_GuiNode = std::move(pDesc->m_GuiNode);
		m_GuiLink = std::move(pDesc->m_GuiLink);
	}
	return S_OK;
}


nlohmann::json CBTRoot::Save_Node()
{
	nlohmann::json j;

	SaveJsonValue(j, "ID", m_GuiNode.iID);
	SaveJsonValue(j, "fValue", m_GuiNode.fValue);
	
	SaveJsonEnum(j, "Group", m_eGroup);
	SaveJsonEnum(j, "GuiNode_BeHaviorType", m_GuiNode.eMyType);
	JsonSaveLoadManager::SaveJsonTypeFloat2(j, "GuiNode_Pos", m_GuiNode.vPos);
	JsonSaveLoadManager::SaveJsonTypeFloat2(j, "GuiNode_Size", m_GuiNode.vSize);
	JsonSaveLoadManager::SaveJsonTypeFloat4(j, "GuiNode_Color", m_GuiNode.vColor);
	JsonSaveLoadManager::SaveJsonTypeString(j, "GuiNode_Name", m_GuiNode.Name);

	SaveJsonEnum(j, "GuiLink_ParentNodeEnum", m_GuiLink.ParentNode.eType);
	SaveJsonValue(j, "GuiLink_StartIndex", m_GuiLink.iStartIdx);
	SaveJsonValue(j, "GuiLink_StartParentNode", m_GuiLink.ParentNode);
	JsonSaveLoadManager::SaveJsonTypeString(j, "MasterName", m_MasterName);

	return j;
}
HRESULT				CBTRoot::Load_json(const nlohmann::json& j)
{
	LoadJsonValue(j, "ID", m_GuiNode.iID);
	LoadJsonValue(j, "fValue", m_GuiNode.fValue);

	LoadJsonEnum(j, "Group", m_eGroup);
	LoadJsonEnum(j, "GuiNode_BeHaviorType", m_GuiNode.eMyType);
	JsonSaveLoadManager::LoadJsonTypeFloat2(j, "GuiNode_Pos", m_GuiNode.vPos);
	JsonSaveLoadManager::LoadJsonTypeFloat2(j, "GuiNode_Size", m_GuiNode.vSize);
	JsonSaveLoadManager::LoadJsonTypeFloat4(j, "GuiNode_Color", m_GuiNode.vColor);
	JsonSaveLoadManager::LoadJsonTypeString(j, "GuiNode_Name", m_GuiNode.Name);

	LoadJsonEnum(j, "GuiLink_ParentNodeEnum", m_GuiLink.ParentNode.eType);
	LoadJsonValue(j, "GuiLink_StartIndex", m_GuiLink.iStartIdx);
	LoadJsonValue(j, "GuiLink_StartParentNode", m_GuiLink.ParentNode);
	JsonSaveLoadManager::LoadJsonTypeString(j, "MasterName", m_MasterName);
	return S_OK;
}
