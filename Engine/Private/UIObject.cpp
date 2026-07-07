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
	m_fAlpha = pDesc->fAlpha;
	m_iWeight = pDesc->ResWeight;
	m_sRestag = pDesc->ResTag;
	strcpy_s(m_cName, pDesc->sObjectTag.c_str());

	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	
	CalcUICoord();
	return S_OK;
}

void CUIObject::Update(_float fTimeDelta)
{
	if (std::nullopt != m_pParent)
	{
		CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_pParent);

		m_fX = parentUI->GetOrigin().x + m_fLocalX;
		m_fY = parentUI->GetOrigin().y + m_fLocalY;
		m_fSizeX = parentUI->GetSize().x * m_fWidthRatioX;
		m_fSizeY = parentUI->GetSize().y * m_fWidthRatioY;
		m_fAlpha = parentUI->GetAlpha() * m_fAlphaRatio;
		m_iWeight = parentUI->GetWeight() + m_iWeightOffset;

		CalcUICoord();
	}
	InputAnimState();
}

void CUIObject::LateUpdate(_float fTimeDelta)
{
}

void CUIObject::Creating()
{
}

void CUIObject::StartHovering()
{
}

void CUIObject::Hovering()
{
}

void CUIObject::EndHovering()
{
}

void CUIObject::Click()
{
}

void CUIObject::Ending()
{
}

void CUIObject::DeleteChild(CHandle childHandle)
{
	m_vChildren.erase(
		std::remove(m_vChildren.begin(), m_vChildren.end(), childHandle),
		m_vChildren.end());
}

void CUIObject::InputAnimState()
{
	if(m_AnimState == ETOUI(UI_ANIM_STATE::CREATING) || m_AnimState == ETOUI(UI_ANIM_STATE::ENDING))
		CheckHovered();

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
		if (m_AnimState != ETOUI(UI_ANIM_STATE::HOVERING) && m_AnimState != ETOUI(UI_ANIM_STATE::CLICK))
			m_AnimState = ETOUI(UI_ANIM_STATE::STARTHOVERING);
		if(MouseLB)
			m_AnimState = ETOUI(UI_ANIM_STATE::CLICK);
	}
	else
	{
		if(m_AnimState != ETOUI(UI_ANIM_STATE::ENDING))
			m_AnimState = ETOUI(UI_ANIM_STATE::ENDHOVERING);
	}

	return true;
}
