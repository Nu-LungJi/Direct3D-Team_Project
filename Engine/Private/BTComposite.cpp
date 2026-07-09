#include "pch.h"
#include "BTComposite.h"
#include "BTSelector.h"
#include "BTSecqunce.h"
#include "BTActionNode.h"
#include "BTDecorator.h"
CBTComposite::CBTComposite()
{

}
CBTComposite::CBTComposite(const CBTComposite& rhs) : CBTRoot(rhs), m_Actions{  }
{
}
CBTComposite::~CBTComposite()
{

}

void CBTComposite::Tick(_float fTimeDelta)
{
	for (auto& iter : m_Actions)
	{
		if(iter != nullptr)
		iter->ResetDebug();
	}
	for (auto& iter : m_Actions)
	{
		if(iter != nullptr)
		iter->Evaluate(fTimeDelta);
	}
		
}

void CBTComposite::ResetDebug()
{
	m_eDebug = EVALUATE::END;
	for (auto& iter : m_Actions)
	{
		if(nullptr != iter)
			iter->ResetDebug();
	}
		
}

HRESULT CBTComposite::Add_Node(void* pArg, UPtr<CBTRoot> pNode)
{
	auto pDesc = static_cast<CBTRoot::BTROOT_DESC*>(pArg);
	
	int32_t iIndex = {};
	//	return E_FAIL;
	return S_OK;
}

nlohmann::json CBTComposite::Save_Node()
{
	size_t iCnt(0);
	nlohmann::json j;
	j = __super::Save_Node();
	for (size_t i = 0; i < m_GuiLink.SlotEnd.size(); ++i)
	{
		if (m_GuiLink.SlotEnd[i].iDestNode != -1)
		{
			_string DestSlotName = "LinkEndSlotName" + std::to_string(iCnt);
			_string DestID = "LinkEndSlotID" + std::to_string(iCnt);
			_string SlotNameEnum = "LinkEndSlotType" + std::to_string(iCnt);
			JsonSaveLoadManager::SaveJsonTypeString(j, DestSlotName, m_GuiLink.SlotEnd[i].DestName);
			SaveJsonValue(j, DestID, m_GuiLink.SlotEnd[i].iDestNode);
			SaveJsonEnum(j, SlotNameEnum, m_GuiLink.SlotEnd[i].eType);
			++iCnt;
		}
	}
	j["GuiLink_SlotSize"] = iCnt;
	iCnt = 0;
	for (auto& iter : m_Actions)
	{
		if (iter != nullptr)
		{
			j["Child"].push_back(iter->Save_Node());
			++iCnt;
		}
	}
	j["Size"] = iCnt;
	return j;
}
HRESULT		CBTComposite::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	size_t iSlotSize = j["GuiLink_SlotSize"];
	m_GuiLink.SlotEnd.resize(iSlotSize);
	for (size_t i = 0; i < iSlotSize; ++i)
	{
		_string DestSlotName = "LinkEndSlotName" + std::to_string(i);
		_string DestID = "LinkEndSlotID" + std::to_string(i);
		_string SlotNameEnum = "LinkEndSlotType" + std::to_string(i);
		JsonSaveLoadManager::LoadJsonTypeString(j, DestSlotName, m_GuiLink.SlotEnd[i].DestName);
		LoadJsonValue(j, DestID, m_GuiLink.SlotEnd[i].iDestNode);
		LoadJsonEnum(j, SlotNameEnum, m_GuiLink.SlotEnd[i].eType);
	}
	size_t iArraySize = j["Size"];
	m_Actions.resize(iArraySize);

	for (size_t i =0; i < iArraySize; ++i)
	{
		_string MasterName{}; 
		NODEGROUP eGroup{}; 
		if (JsonSaveLoadManager::LoadJsonTypeString(j["Child"][i], "MasterName", MasterName))
		{
			if (LoadJsonEnum(j["Child"][i], "Group", eGroup))
			{
				auto pSrc = CGameInstance::Get().Clone_Action(eGroup, MasterName, nullptr);
				pSrc->Load_json(j["Child"][i]);
				m_Actions[i] = std::move(pSrc);
			}

		}
	}
	return S_OK;
}
HRESULT CBTComposite::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	m_Actions.resize(m_GuiLink.SlotEnd.size());
	return S_OK;
}
UPtr<CBTComposite> CBTComposite::Create(void* pArg)
{
	auto pInstance = ToUPtr(new CBTComposite());
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Created : CBTComposite");
		return nullptr;
	}
	return pInstance;
}

