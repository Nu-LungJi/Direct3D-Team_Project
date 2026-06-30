#include "pch.h"
#include "UIObject.h"
#include "GameInstance.h"

NS_USING(Engine)

CUIObject::CUIObject()
{
}

CUIObject::~CUIObject()
{
}

void CUIObject::UpdateGUI()
{
	CGameObject::UpdateGUI();

	if (ImGui::DragFloat("fX", (float*)&m_fX, 0.1f))
	{
		CalcUICoord();
	}
	if (ImGui::DragFloat("fY", (float*)&m_fY, 0.1f))
	{
		CalcUICoord();
	}
}

HRESULT CUIObject::Initialize(void* pArg)
{
	auto		pDesc = static_cast<UIOBJECT_DESC*>(pArg);

	m_fX = pDesc->fX;
	m_fY = pDesc->fY;
	m_fSizeX = pDesc->fSizeX;
	m_fSizeY = pDesc->fSizeY;

	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	
	CalcUICoord();
	return S_OK;
}

void CUIObject::CalcUICoord()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();
	auto clientWidth = clientSize.x;
	auto clientHeight = clientSize.y;
	GetTransform().SetScale(E::_float3{ m_fSizeX, m_fSizeY, 1.f });
	//auto a = m_fX - clientWidth * 0.5f;
	//auto b = -m_fY + clientHeight * 0.5f;

	
	GetTransform().SetPosition(XMVectorSet(m_fX - clientWidth * 0.5f, -m_fY + clientHeight * 0.5f, GetTransform().GetPosition().z, 1.f));
}
