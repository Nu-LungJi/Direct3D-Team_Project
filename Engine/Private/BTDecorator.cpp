#include "pch.h"
#include "BTDecorator.h"

CBTDecorator::CBTDecorator()
{
}

CBTDecorator::CBTDecorator(const CBTDecorator& Prototype) : CBTRoot(Prototype)
{
}

CBTDecorator ::~CBTDecorator()
{
}


HRESULT CBTDecorator::InitalizePrototype(void* pArg)
{
	__super::InitalizePrototype(pArg);
	
	return S_OK;
}

HRESULT CBTDecorator::Initalize(void* pArg)
{
    if (FAILED(__super::Initalize(pArg)))
        return E_FAIL;

    return S_OK;
}

EVALUATE CBTDecorator::Evaluate(_float fTimeDelta)
{
    if(m_pDecorator != nullptr)
        return m_pDecorator->Evaluate(fTimeDelta);
    
    return EVALUATE::FAILED;
}

nlohmann::json CBTDecorator::Save_Node()
{
	const _string Name = "Child";
	nlohmann::json j;
	j = __super::Save_Node();

	if (m_GuiLink.SlotEnd[0].iDestNode != -1)
	{
		JsonSaveLoadManager::SaveJsonTypeString(j, "LinkEndSlotName", m_GuiLink.SlotEnd[0].DestName);
		SaveJsonValue(j, "LinkEndSlotID", m_GuiLink.SlotEnd[0].iDestNode);
		SaveJsonEnum(j, "LinkEndSlotEnum", m_GuiLink.SlotEnd[0].eType);
	}	
	if(m_pDecorator != nullptr)
		j[Name] = m_pDecorator->Save_Node();
	
	return j;
}

HRESULT CBTDecorator::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	m_GuiLink.SlotEnd.resize(1);
	if (j.contains("LinkEndSlot"))
	{
		JsonSaveLoadManager::LoadJsonTypeString(j, "LinkEndSlotName", m_GuiLink.SlotEnd[0].DestName);
		LoadJsonValue(j, "LinkEndSlotID", m_GuiLink.SlotEnd[0].iDestNode);
		LoadJsonEnum(j, "LinkEndSlotEnum", m_GuiLink.SlotEnd[0].eType);
	}


	if (j.contains("Child"))
	{
		_string MasterName{};
		NODEGROUP eGroup{};
		if (JsonSaveLoadManager::LoadJsonTypeString(j["Child"], "MasterName", MasterName))
		{
			if (LoadJsonEnum(j["Child"], "Group", eGroup))
			{
				auto pSrc = CGameInstance::Get().Clone_Action(eGroup, MasterName, nullptr);
				pSrc->Load_json(j["Child"]);
				m_pDecorator = std::move(pSrc);
			}

		}
	}
	

    return S_OK;
}

