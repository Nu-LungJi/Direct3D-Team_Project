#include "pch.h"

#include "ButtonComponent.h"
#include "UIObject.h"

NS_USING(Engine)

CButtonComponent::CButtonComponent()
{
}

CButtonComponent::~CButtonComponent()
{
}

bool CButtonComponent::CheckPixelPerfectCollision(_float2 mousePos, bool bIsTopUI)
{
	CUIObject* pOwner = static_cast<CUIObject*>(m_pGameObject);
	if (!pOwner) return false;

	bool bCurrentCollision = bIsTopUI && PtInRect(mousePos);

	uint32_t currentStates = 0;

	if (m_bAppear)
	{
		m_bAppear = false;
		currentStates |= ETOUI(CUIObject::UI_STATE::APPEAR);
	}

	if (m_bDisppear)
	{
		m_bDisppear = false;
		currentStates |= ETOUI(CUIObject::UI_STATE::DISAPPEAR);
	}

	if (pOwner->GetInputLcok())
	{
		return false;
	}

	if (!m_bIsHovered && bCurrentCollision)
	{
		m_bIsHovered = true;
		currentStates |= ETOUI(CUIObject::UI_STATE::ENTER); // 상태 추가
	}
	else if (m_bIsHovered && !bCurrentCollision)
	{
		m_bIsHovered = false;
		currentStates |= ETOUI(CUIObject::UI_STATE::EXIT);  // 상태 추가
	}
	else if (bCurrentCollision)
	{
		currentStates |= ETOUI(CUIObject::UI_STATE::HOVERED);
	}

	if (bCurrentCollision && CGameInstance::Get().MouseDown(MOUSEKEYSTATE::LB))
	{
		currentStates |= ETOUI(CUIObject::UI_STATE::CLICK); // 상태 추가
	}

	if (currentStates != 0)
	{
		pOwner->PlayEffect(currentStates);
	}

	return bCurrentCollision;
}

bool CButtonComponent::PtInRect(_float2 mousePos)
{
	CUIObject* pOwner = static_cast<CUIObject*>(m_pGameObject);
	const UI_INFO& selectInfo = pOwner->GetUIInfo();

	_float2 origin = { selectInfo.fX, selectInfo.fY };
	_float2 size = { selectInfo.SizeX, selectInfo.SizeY };

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
		return true;
	}

	return false;
}

HRESULT CButtonComponent::Initialize(void* pArg)
{
	if (FAILED(CUIComponent::Initialize(pArg)))
		return E_FAIL;


	return S_OK;
}

void CButtonComponent::Update(_float fTimeDelta, _float2 mousePos)
{
	CUIComponent::Update(fTimeDelta);

	CheckPixelPerfectCollision(mousePos, true);
}

UPtr<CButtonComponent> CButtonComponent::Create()
{
	auto pInstance = ToUPtr(new CButtonComponent{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CButtonComponent");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CButtonComponent::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CButtonComponent{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned: CButtonComponent");
		return nullptr;
	}
	return pInstance;
}
