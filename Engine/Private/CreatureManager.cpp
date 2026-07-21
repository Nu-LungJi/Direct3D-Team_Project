#include "pch.h"
#include "CreatureManager.h"

NS_USING(Engine)


CCreatureManager::CCreatureManager()
{
}
CCreatureManager::~CCreatureManager()
{

}

HRESULT CCreatureManager::Initilize()
{
    return S_OK;
}


void CCreatureManager::Update(_float fTimeDelta)
{

}

void CCreatureManager::UpdateGUI()
{
    if (m_hTestModel.GetIndex() != 0)
        return;
	ImGui::Begin("Craeture Editor");


	if (ImGui::Button("Spawn TestModel x10"))
	{


	}

	ImGui::End();

}


UPtr<CCreatureManager> CCreatureManager::Create()
{
    auto pInstance = UPtr<CCreatureManager>(new CCreatureManager{});
    if (FAILED(pInstance->Initilize()))
    {
        MSG_BOX("CreatureEditor Create Failed");
        return nullptr;
    }
    return pInstance;
}
