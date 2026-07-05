#include "pch.h"
#include "CreatureEditor.h"

NS_USING(Engine)


CCreatureEditor::CCreatureEditor()
{
}
CCreatureEditor::~CCreatureEditor()
{

}

HRESULT CCreatureEditor::Initilize()
{

    return S_OK;
}


void CCreatureEditor::Update(_float fTimeDelta)
{

}

void CCreatureEditor::UpdateGUI()
{
    if (m_hTestModel.GetIndex() != 0)
        return;

}


UPtr<CCreatureEditor> CCreatureEditor::Create()
{
    auto pInstance = UPtr<CCreatureEditor>(new CCreatureEditor{});
    if (FAILED(pInstance->Initilize()))
    {
        MSG_BOX("CreatureEditor Create Failed");
        return nullptr;
    }
    return pInstance;
}
