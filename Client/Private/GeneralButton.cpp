#include "pch.h"
#include "GeneralButton.h"

#include "GameInstance.h"
#include "TweenComponent.h"
#include "UIManager.h"

NS_USING(Client)

CGeneralButton::CGeneralButton() = default;
CGeneralButton::~CGeneralButton() = default;

HRESULT CGeneralButton::InitializePrototype(void* pArg)
{
	return CTextureUI::InitializePrototype(pArg);
}

HRESULT CGeneralButton::Initialize(void* pArg)
{
	if (FAILED(CTextureUI::Initialize(pArg)))
		return E_FAIL;

	m_UIINFO.UIType = ETOUI(UI_TYPE::GENERAL_BUTTON);
	m_fBaseScale = GetScaleRatio();
	return S_OK;
}

void CGeneralButton::PlayEffect(uint32_t uiState)
{
	if (uiState & ETOUI(UI_STATE::APPEAR))
		HandleAppear();

	if (uiState & ETOUI(UI_STATE::DISAPPEAR))
		HandleDisappear();

	if (m_bInputLocked)
		return;

	if (uiState & ETOUI(UI_STATE::ENTER))
		HandleEnter();

	if (uiState & ETOUI(UI_STATE::EXIT))
		HandleExit();

	if (uiState & ETOUI(UI_STATE::CLICK))
		HandleClick();
}

void CGeneralButton::HandleAppear()
{
	if (Appear)
		Appear(this);
}

void CGeneralButton::HandleDisappear()
{
	RemoveHoverEffect();
	if (Disappear)
		Disappear(this);
}

void CGeneralButton::HandleEnter()
{
	if (m_bHovering)
		return;

	m_bHovering = true;
	m_fBaseScale = GetScaleRatio();

	switch (m_eButtonType)
	{
	case GENERAL_BUTTON_TYPE::WAND_CATEGORY:
		PlayScaleTo(m_fBaseScale * 1.06f, 0.10f);
		CreateHoverEffect();
		break;
	case GENERAL_BUTTON_TYPE::WAND_MATERIAL:
	case GENERAL_BUTTON_TYPE::WAND_ITEM:
		PlayScaleTo(m_fBaseScale * 1.08f, 0.10f);
		CreateHoverEffect();
		break;
	case GENERAL_BUTTON_TYPE::CONFIRM:
		PlayScaleTo(m_fBaseScale * 1.10f, 0.08f);
		CreateHoverEffect();
		break;
	case GENERAL_BUTTON_TYPE::CANCEL:
		PlayScaleTo(m_fBaseScale * 1.05f, 0.08f);
		break;
	case GENERAL_BUTTON_TYPE::DEFAULT:
	default:
		PlayScaleTo(m_fBaseScale * 1.04f, 0.10f);
		break;
	}
}

void CGeneralButton::HandleExit()
{
	if (!m_bHovering)
		return;

	m_bHovering = false;
	RemoveHoverEffect();
	PlayScaleTo(m_fBaseScale, 0.10f);
}

void CGeneralButton::HandleClick()
{
	if (m_pComTween)
		m_pComTween->ClearTweens();

	const _float hoverScale = GetScaleRatio();
	PlayScaleTo(m_fBaseScale * 0.94f, 0.05f);
	PlayScaleTo(m_bHovering ? hoverScale : m_fBaseScale, 0.08f, 0.05f);

	// Gameplay behavior is selected in code by button type. The controller only
	// supplies a callback and never has to be selected from the UIEditor GUI.
	switch (m_eButtonType)
	{
	case GENERAL_BUTTON_TYPE::WAND_CATEGORY:
	case GENERAL_BUTTON_TYPE::WAND_MATERIAL:
	case GENERAL_BUTTON_TYPE::WAND_ITEM:
	case GENERAL_BUTTON_TYPE::CONFIRM:
	case GENERAL_BUTTON_TYPE::CANCEL:
	case GENERAL_BUTTON_TYPE::DEFAULT:
	default:
		ExecuteCommand();
		break;
	}
}

void CGeneralButton::PlayScaleTo(_float targetScale, _float duration, _float delay)
{
	if (!m_pComTween)
		return;

	const CHandle handle = GetHandle();
	const _float startScale = GetScaleRatio();
	m_pComTween->PlayTween(
		startScale,
		targetScale,
		duration,
		[handle](_float value)
		{
			if (auto* ui = GetSafeUI(handle))
			{
				ui->SetScaleRatio(value);
				ui->CalcUICoord();
			}
		},
		nullptr,
		EEaseType::EaseOutQuad,
		delay);
}

void CGeneralButton::CreateHoverEffect()
{
	if (m_HoverEffect && GetSafeUI(*m_HoverEffect))
		return;

	const std::filesystem::path effectPath =
		std::filesystem::path("./Resources/SampleClient/UIData/Prefabs") /
		(HOVER_EFFECT_PREFAB + std::string(".json"));
	if (!std::filesystem::exists(effectPath))
		return;

	auto handles = GET_SINGLE(UIManager)->LoadPrefab(HOVER_EFFECT_PREFAB);
	if (handles.empty())
		return;

	m_HoverEffect = handles.front();
	auto* effect = GetSafeUI(*m_HoverEffect);
	if (!effect)
	{
		m_HoverEffect.reset();
		return;
	}

	effect->SetParent(GetHandle());
	AddChildren(effect->GetHandle());
	auto& effectInfo = effect->GetUIInfo();
	effectInfo.LocalX = 0.f;
	effectInfo.LocalY = 0.f;
	effectInfo.WeightOffset = 1;
	effect->SetAlphaRatio(1.f);
}

void CGeneralButton::RemoveHoverEffect()
{
	if (!m_HoverEffect)
		return;

	if (GetSafeUI(*m_HoverEffect))
		GET_SINGLE(UIManager)->DeleteUIRecursive(*m_HoverEffect);

	m_HoverEffect.reset();
}

void CGeneralButton::ExecuteCommand()
{
	if (m_CommandCallback)
		m_CommandCallback(m_eButtonType, m_sCommandParameter);
}

E::UPtr<CGeneralButton> CGeneralButton::Create()
{
	auto instance = E::ToUPtr(new CGeneralButton{});
	if (FAILED(instance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create : CGeneralButton");
		return nullptr;
	}
	return instance;
}

E::UPtr<E::CPrototype> CGeneralButton::Clone(void* pArg)
{
	auto instance = E::ToUPtr(new CGeneralButton{ *this });
	if (FAILED(instance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone : CGeneralButton");
		return nullptr;
	}
	return instance;
}
