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
		j["LinkEndSlot"] = m_GuiLink.SlotEnd[0];
		SaveJsonEnum(j, "LinkEndSlotEnum", m_GuiLink.SlotEnd[0].eType);
	}	
	if(m_pDecorator != nullptr)
		j[Name] = m_pDecorator->Save_Node();
	
	return j;
}
void CBTDecorator::Update_Gui()
{

}
HRESULT CBTDecorator::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	m_GuiLink.SlotEnd.resize(1);
	if (j.contains("LinkEndSlot"))
	{
		DEST_NODE Node{};
		j["LinkEndSlot"].get_to<DEST_NODE>(Node);
		m_GuiLink.SlotEnd[0] = Node;
		LoadJsonEnum(j, "LinkEndSlotEnum", m_GuiLink.SlotEnd[0].eType);
	}


	if (j.contains("Child"))
	{
		auto pSrc = CGameInstance::Get().Clone_Action(m_eGroup, m_MasterName, nullptr);
		pSrc->Load_json(j["Child"]);
		m_pDecorator = std::move(pSrc);
	}
	

    return S_OK;
}

