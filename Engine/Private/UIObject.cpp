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

	m_UIINFO.fX = pDesc->fX;
	m_UIINFO.fY = pDesc->fY;
	m_UIINFO.SizeX = pDesc->fSizeX;
	m_UIINFO.SizeY = pDesc->fSizeY;
	m_UIINFO.Alpha = pDesc->fAlpha;
	m_UIINFO.Weight = pDesc->ResWeight;
	m_UIINFO.Restag = pDesc->ResTag;
	m_UIINFO.Name = pDesc->Name;
	strcpy_s(m_cName, pDesc->sObjectTag.c_str());

	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	CalcUICoord();

	//m_fX = pDesc->fX;
	//m_fY = pDesc->fY;
	//m_fSizeX = pDesc->fSizeX;
	//m_fSizeY = pDesc->fSizeY;
	//m_fAlpha = pDesc->fAlpha;
	//m_iWeight = pDesc->ResWeight;
	//m_sRestag = pDesc->ResTag;
	//strcpy_s(m_cName, pDesc->sObjectTag.c_str());


	return S_OK;
}

void CUIObject::Update(_float fTimeDelta)
{
	if (std::nullopt != m_pParent)
	{
		CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_pParent);
		UI_INFO& parentInfo = parentUI->GetUIInfo();

		m_UIINFO.fX		= parentInfo.fX + m_UIINFO.LocalX;
		m_UIINFO.fY		= parentInfo.fY + m_UIINFO.LocalY;
		m_UIINFO.SizeX	= parentInfo.SizeX * m_UIINFO.WidthRatioX;
		m_UIINFO.SizeY	= parentInfo.SizeY * m_UIINFO.WidthRatioY;
		m_UIINFO.Alpha	= parentInfo.Alpha * m_UIINFO.AlphaRatio;
		m_UIINFO.Weight = parentInfo.Weight + m_UIINFO.WeightOffset;
		m_UIINFO.Rot	= parentInfo.Rot + m_UIINFO.LocalRot;

		CalcUICoord();
	}
}

void CUIObject::LateUpdate(_float fTimeDelta)
{
}

void CUIObject::DeleteChild(CHandle childHandle)
{
	m_vChildren.erase(
		std::remove(m_vChildren.begin(), m_vChildren.end(), childHandle),
		m_vChildren.end());
}

void CUIObject::CalcUICoord()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();
	auto clientWidth = clientSize.x;
	auto clientHeight = clientSize.y;
	GetTransform().SetScale(E::_float3{ m_UIINFO.SizeX, m_UIINFO.SizeY, 1.f });
	auto x = m_UIINFO.fX - clientWidth * 0.5f;
	auto y = -m_UIINFO.fY + clientHeight * 0.5f;
	
	GetTransform().SetPosition(XMVectorSet(x, y, GetTransform().GetPosition().z, 1.f));
}

_bool CUIObject::CheckHovered()
{
	_bool MouseLB = CGameInstance::Get().MouseDown(MOUSEKEYSTATE::LB);

	_float2 mousePos = CGameInstance::Get().GetMousePos();

	_float2 origin = {m_fX, m_fY};
	_float2 size = {m_fSizeX, m_fSizeY};

	_float2 minPos =
	{
		origin.x - size.x * 0.5f,
		origin.y - size.y * 0.5f
	};

	_float2 maxPos =
	{
		origin.x + size.x * 0.5f,
		origin.y + size.y * 0.5f
	};

	if (mousePos.x >= minPos.x &&
		mousePos.x <= maxPos.x &&
		mousePos.y >= minPos.y &&
		mousePos.y <= maxPos.y)
	{
	}
	else
	{
	}

	return true;
}
