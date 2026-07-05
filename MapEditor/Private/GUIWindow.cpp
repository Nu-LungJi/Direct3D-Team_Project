#include "pch.h"
#include "GUIWindow.h"
#include "GameInstance.h"

NS_USING(Client)

CGUIWindow::CGUIWindow()
{

}

CGUIWindow::~CGUIWindow()
{

}

HRESULT CGUIWindow::Initialize(E::CHandle* pSelectedObject)
{
	if (pSelectedObject == nullptr)
	{
		return E_FAIL;
	}

	m_pSelectedObject = pSelectedObject;
	return S_OK;
}

E::CGameObject* CGUIWindow::GetSelectedObject() const
{
	if (m_pSelectedObject == nullptr)
	{
		return nullptr;
	}

	return E::CGameInstance::Get().GetGameObjectByHandle(*m_pSelectedObject);
}
