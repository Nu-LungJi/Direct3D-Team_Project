#include "pch.h"
#include "GeneralButton.h"

#include "GameInstance.h"
#include "TweenComponent.h"
#include "UIManager.h"

NS_USING(Client)

namespace
{
	CUIObject* FindRootedUIByName(std::string_view targetName)
	{
		const auto* uiHandles =
			E::CGameInstance::Get().GetGameObjectLayer("Layer_UI");
		if (!uiHandles)
			return nullptr;

		for (const CHandle uiHandle : *uiHandles)
		{
			auto* ui = GetSafeUI(uiHandle);
			if (ui && std::string_view(ui->GetName()) == targetName)
				return ui;
		}

		return nullptr;
	}
}

std::optional<CHandle> CGeneralButton::s_SelectedWandItem{};
std::optional<CHandle> CGeneralButton::s_SelectedWandMaterial{};
std::optional<CHandle> CGeneralButton::s_SelectedWandCategory{};

CGeneralButton::CGeneralButton() = default;
CGeneralButton::~CGeneralButton()
{
	if (s_SelectedWandItem && *s_SelectedWandItem == GetHandle())
		s_SelectedWandItem.reset();
	if (s_SelectedWandMaterial &&
		*s_SelectedWandMaterial == GetHandle())
	{
		s_SelectedWandMaterial.reset();
	}
	if (s_SelectedWandCategory &&
		*s_SelectedWandCategory == GetHandle())
	{
		s_SelectedWandCategory.reset();
	}
}

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
	m_bBaseScaleCaptured = false;
	return S_OK;
}

void CGeneralButton::Update(_float fTimeDelta)
{
	CTextureUI::Update(fTimeDelta);
	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CATEGORY &&
		(m_bSelected || m_CategoryGradientEffect ||
			m_CategorySphereEffect))
	{
		UpdateCategorySelectionEffect(fTimeDelta);
	}
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
	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CATEGORY &&
		m_sCommandParameter == "1" &&
		(!s_SelectedWandCategory || !GetSafeUI(*s_SelectedWandCategory)))
	{
		SelectWithinGroup();
	}

	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_ITEM &&
		m_sCommandParameter == "0" &&
		(!s_SelectedWandItem || !GetSafeUI(*s_SelectedWandItem)))
	{
		SelectWithinGroup();
		UpdateWandMaterialTextures(false);
		SelectFirstWandMaterial();
		UpdateCenterWandTexture();
	}

	if (Appear)
		Appear(this);
}

void CGeneralButton::HandleDisappear()
{
	RemoveHoverEffect();
	SetSelected(false);
	if (Disappear)
		Disappear(this);
}

void CGeneralButton::HandleEnter()
{
	if (m_bHovering)
		return;

	EnsureBaseScaleCaptured();
	m_bHovering = true;

	switch (m_eButtonType)
	{
	case GENERAL_BUTTON_TYPE::WAND_CATEGORY:
		break;
	case GENERAL_BUTTON_TYPE::WAND_MATERIAL:
	case GENERAL_BUTTON_TYPE::WAND_ITEM:
		PlayScaleTo(GetSelectedScale(), WAND_HOVER_SCALE_DURATION);
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
	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CATEGORY)
		return;
	RemoveHoverEffect();
	PlayScaleTo(
		m_bSelected ? GetSelectedScale() : m_fBaseScale,
		(m_eButtonType == GENERAL_BUTTON_TYPE::WAND_ITEM ||
			m_eButtonType == GENERAL_BUTTON_TYPE::WAND_MATERIAL) ?
			WAND_HOVER_SCALE_DURATION : 0.10f);
}

void CGeneralButton::HandleClick()
{
	EnsureBaseScaleCaptured();
	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CATEGORY)
	{
		SelectWithinGroup();
		ExecuteCommand();
		return;
	}

	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_ITEM ||
		m_eButtonType == GENERAL_BUTTON_TYPE::WAND_MATERIAL)
	{
		SelectWithinGroup();
		if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_ITEM)
		{
			SelectFirstWandMaterial();
			UpdateWandMaterialTextures();
		}
		UpdateCenterWandTexture();
		ExecuteCommand();
		return;
	}

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

void CGeneralButton::EnsureBaseScaleCaptured()
{
	if (!m_bBaseScaleCaptured)
		RefreshBaseScale();
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

void CGeneralButton::SetSelected(_bool selected)
{
	if (m_bSelected == selected)
		return;

	EnsureBaseScaleCaptured();
	m_bSelected = selected;
	if (m_pComTween)
		m_pComTween->ClearTweens();

	if (selected)
	{
		if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CATEGORY)
		{
			CreateCategorySelectionEffect();
			PlayCategoryEffectVisibility(true);
			PlayScaleTo(GetSelectedScale(), WAND_SELECTION_SCALE_DURATION);
		}
		else
		{
			CreateSelectionEffect();
			PlayScaleTo(GetSelectedScale(), WAND_SELECTION_SCALE_DURATION);
		}
	}
	else
	{
		if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CATEGORY)
		{
			RemoveCategorySelectionEffect();
			PlayScaleTo(m_fBaseScale, WAND_SELECTION_SCALE_DURATION);
		}
		else
		{
			RemoveSelectionEffect();
			PlayScaleTo(m_fBaseScale, WAND_SELECTION_SCALE_DURATION);
		}
		if (s_SelectedWandItem &&
			*s_SelectedWandItem == GetHandle())
		{
			s_SelectedWandItem.reset();
		}
		if (s_SelectedWandMaterial &&
			*s_SelectedWandMaterial == GetHandle())
		{
			s_SelectedWandMaterial.reset();
		}
		if (s_SelectedWandCategory &&
			*s_SelectedWandCategory == GetHandle())
		{
			s_SelectedWandCategory.reset();
		}
	}
}

void CGeneralButton::SelectWithinGroup()
{
	std::optional<CHandle>* selectedGroup = nullptr;
	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_ITEM)
		selectedGroup = &s_SelectedWandItem;
	else if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_MATERIAL)
		selectedGroup = &s_SelectedWandMaterial;
	else if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CATEGORY)
		selectedGroup = &s_SelectedWandCategory;

	if (!selectedGroup)
		return;
	if (*selectedGroup && **selectedGroup == GetHandle())
		return;

	if (*selectedGroup)
	{
		if (auto* previous = E::CGameInstance::Get().
			GetGameObjectByHandleT<CGeneralButton>(**selectedGroup))
		{
			previous->SetSelected(false);
		}
	}

	*selectedGroup = GetHandle();
	SetSelected(true);
}

void CGeneralButton::UpdateWandMaterialTextures(_bool animate) const
{
	uint32_t wandShapeIndex = 0;
	if (m_eButtonType != GENERAL_BUTTON_TYPE::WAND_ITEM ||
		!TryGetButtonIndex(this, wandShapeIndex))
		return;

	for (uint32_t materialIndex = 0; materialIndex < 3u; ++materialIndex)
	{
		char buttonName[32]{};
		char resourceTag[64]{};
		sprintf_s(buttonName, "WandTexture%u", materialIndex + 1u);
		sprintf_s(resourceTag,
			"TEX_UI_T_T%03u_M%02u_var",
			wandShapeIndex,
			materialIndex);

		auto* materialUI = FindRootedUIByName(buttonName);
		if (!materialUI)
			continue;

		if (!animate)
		{
			materialUI->SetResTag(resourceTag);
			continue;
		}

		auto* materialButton = E::CGameInstance::Get().
			GetGameObjectByHandleT<CGeneralButton>(materialUI->GetHandle());
		if (materialButton)
			materialButton->AnimateWandMaterialTexture(resourceTag);
		else
			materialUI->SetResTag(resourceTag);
	}
}

void CGeneralButton::AnimateWandMaterialTexture(
	const std::string& resourceTag)
{
	if (!m_pComTween)
	{
		SetResTag(resourceTag);
		return;
	}

	if (!m_bWandTransitionBaseCaptured)
	{
		m_fWandTransitionBaseX = m_UIINFO.fX;
		m_fWandTransitionBaseAlpha = m_UIINFO.Alpha;
		m_bWandTransitionBaseCaptured = true;
	}

	const CHandle handle = GetHandle();
	const _float baseX = m_fWandTransitionBaseX;
	const _float leftX = baseX - WAND_TEXTURE_SLIDE_OFFSET;
	const _float baseAlpha = m_fWandTransitionBaseAlpha;

	// Restart from the authored location so repeated fast selections never
	// accumulate the slide offset. Re-add the scale tween after clearing.
	m_pComTween->ClearTweens();
	m_UIINFO.fX = baseX;
	m_UIINFO.Alpha = baseAlpha;
	CalcUICoord();
	PlayScaleTo(
		m_bSelected ? GetSelectedScale() : m_fBaseScale,
		WAND_SELECTION_SCALE_DURATION);

	m_pComTween->PlayTween(
		baseX,
		leftX,
		WAND_TEXTURE_FADE_HALF_DURATION,
		[handle](_float value)
		{
			if (auto* ui = GetSafeUI(handle))
			{
				ui->GetUIInfo().fX = value;
				ui->CalcUICoord();
			}
		},
		nullptr,
		EEaseType::EaseOutQuad);

	m_pComTween->PlayTween(
		baseAlpha,
		0.f,
		WAND_TEXTURE_FADE_HALF_DURATION,
		[handle](_float value)
		{
			if (auto* ui = GetSafeUI(handle))
				ui->SetAlpha(value);
		},
		[handle, resourceTag]()
		{
			if (auto* ui = GetSafeUI(handle))
				ui->SetResTag(resourceTag);
		},
		EEaseType::EaseOutQuad);

	m_pComTween->PlayTween(
		leftX,
		baseX,
		WAND_TEXTURE_FADE_HALF_DURATION,
		[handle](_float value)
		{
			if (auto* ui = GetSafeUI(handle))
			{
				ui->GetUIInfo().fX = value;
				ui->CalcUICoord();
			}
		},
		nullptr,
		EEaseType::EaseOutQuad,
		WAND_TEXTURE_FADE_HALF_DURATION);

	m_pComTween->PlayTween(
		0.f,
		baseAlpha,
		WAND_TEXTURE_FADE_HALF_DURATION,
		[handle](_float value)
		{
			if (auto* ui = GetSafeUI(handle))
				ui->SetAlpha(value);
		},
		nullptr,
		EEaseType::EaseOutQuad,
		WAND_TEXTURE_FADE_HALF_DURATION);
}

void CGeneralButton::SelectFirstWandMaterial() const
{
	auto* firstMaterialUI = FindRootedUIByName("WandTexture1");
	if (!firstMaterialUI)
		return;

	auto* firstMaterial = E::CGameInstance::Get().
		GetGameObjectByHandleT<CGeneralButton>(firstMaterialUI->GetHandle());
	if (firstMaterial)
		firstMaterial->SelectWithinGroup();
}

void CGeneralButton::UpdateCenterWandTexture() const
{
	if (!s_SelectedWandItem || !s_SelectedWandMaterial)
		return;

	auto* selectedItem = E::CGameInstance::Get().
		GetGameObjectByHandleT<CGeneralButton>(*s_SelectedWandItem);
	auto* selectedMaterial = E::CGameInstance::Get().
		GetGameObjectByHandleT<CGeneralButton>(*s_SelectedWandMaterial);
	uint32_t wandShapeIndex = 0;
	uint32_t materialIndex = 0;
	if (!TryGetButtonIndex(selectedItem, wandShapeIndex) ||
		!TryGetButtonIndex(selectedMaterial, materialIndex))
	{
		return;
	}

	char resourceTag[64]{};
	sprintf_s(resourceTag,
		"TEX_UI_T_T%03u_M%02u",
		wandShapeIndex,
		materialIndex);

	if (auto* centerWand = FindRootedUIByName("WandImage"))
		centerWand->SetResTag(resourceTag);
}

std::optional<CHandle> CGeneralButton::CreateCategoryEffectTexture(
	const std::string& name,
	const std::string& resourceTag,
	const _float2& size,
	const _float2& localPosition,
	_float alphaRatio,
	int weightOffset)
{
	const uint32_t levelID =
		E::CGameInstance::Get().GetCurrentLevelID();
	const std::string currentLevel = levelID > 100u ?
		"LEVEL_LOADING" :
		_string("LEVEL_") + MagicEnumToStringView(
			static_cast<LEVEL>(levelID)).data();

	CTextureUI::UIOBJECT_DESC desc{};
	desc.sObjectTag = name;
	desc.Name = name;
	desc.fX = m_UIINFO.fX;
	desc.fY = m_UIINFO.fY;
	desc.fSizeX = size.x;
	desc.fSizeY = size.y;
	desc.fAlpha = 1.f;
	desc.ResTag = resourceTag;
	desc.ResWeight = m_UIINFO.Weight + weightOffset;
	desc.UIType = ETOUI(UI_TYPE::TEXUI);

	const auto handle = E::CGameInstance::Get().AddGameObjectToLayer(
		currentLevel,
		"Prototype_GameObject_TextureUI",
		"Layer_UI",
		&desc);
	if (!handle)
		return std::nullopt;

	auto* effect = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(*handle);
	if (!effect)
		return std::nullopt;

	effect->SetParent(GetHandle());
	AddChildren(*handle);
	auto& effectInfo = effect->GetUIInfo();
	effectInfo.LocalX = localPosition.x;
	effectInfo.LocalY = localPosition.y;
	effectInfo.WeightOffset = weightOffset;
	effect->SetColor({ 1.34f, 0.94f, 0.42f });
	effect->SetAlphaRatio(alphaRatio * m_fCategoryEffectVisibility);
	effect->SetAdditiveBlend(true);
	effect->SetInputLcok(true);
	effect->CalcUICoord();
	return handle;
}

void CGeneralButton::CreateCategorySelectionEffect()
{
	if (m_CategoryGradientEffect &&
		GetSafeUI(*m_CategoryGradientEffect))
	{
		return;
	}

	for (const CHandle childHandle : GetChildren())
	{
		auto* child = GetSafeUI(childHandle);
		if (!child || std::string_view(child->GetName()) != "menuFrame")
			continue;

		if (!m_bCategoryFrameColorCaptured)
		{
			m_vCategoryFrameBaseColor = child->GetUIInfo().Color;
			m_bCategoryFrameColorCaptured = true;
		}
		child->SetColor({ 1.30f, 0.90f, 0.34f });
		break;
	}

	m_CategoryGradientEffect = CreateCategoryEffectTexture(
		"WandCategory_Gradient",
		"TEX_VFX_T_GradientSpline_D",
		{ 128.f, 170.f },
		{ 0.f, -54.f },
		0.48f,
		-1);
	m_CategorySphereEffect = CreateCategoryEffectTexture(
		"WandCategory_Sphere",
		"TEX_VFX_T_Circle_Sphere_D",
		{ 128.f, 128.f },
		{ 0.f, 0.f },
		0.13f,
		3);
	m_CategorySplatterEffects[0] = CreateCategoryEffectTexture(
		"WandCategory_Splatter0",
		"TEX_UI_T_MagicSplatter",
		{ 42.f, 42.f },
		{ -12.f, 8.f },
		0.f,
		2);
	m_CategorySplatterEffects[1] = CreateCategoryEffectTexture(
		"WandCategory_Splatter1",
		"TEX_UI_T_MagicSplatter",
		{ 36.f, 36.f },
		{ 14.f, 4.f },
		0.f,
		2);
	m_CategorySplatterEffects[2] = CreateCategoryEffectTexture(
		"WandCategory_Splatter2",
		"TEX_UI_T_MagicSplatter",
		{ 32.f, 32.f },
		{ -4.f, 10.f },
		0.f,
		2);
	m_CategorySplatterEffects[3] = CreateCategoryEffectTexture(
		"WandCategory_Splatter3",
		"TEX_UI_T_MagicSplatter",
		{ 38.f, 38.f },
		{ 6.f, 2.f },
		0.f,
		2);
	m_fCategoryEffectTime = 0.f;
}

void CGeneralButton::RemoveCategorySelectionEffect()
{
	// The frame belongs to the button itself, so restore its normal blue color
	// immediately. The auxiliary glow textures continue their fade-out below.
	for (const CHandle childHandle : GetChildren())
	{
		auto* child = GetSafeUI(childHandle);
		if (!child || std::string_view(child->GetName()) != "menuFrame")
			continue;

		if (m_bCategoryFrameColorCaptured)
			child->SetColor(m_vCategoryFrameBaseColor);
		break;
	}

	PlayCategoryEffectVisibility(false);
}

void CGeneralButton::DeleteCategorySelectionEffects()
{
	for (const CHandle childHandle : GetChildren())
	{
		auto* child = GetSafeUI(childHandle);
		if (child && std::string_view(child->GetName()) == "menuFrame")
		{
			if (m_bCategoryFrameColorCaptured)
				child->SetColor(m_vCategoryFrameBaseColor);
			break;
		}
	}

	auto removeEffect = [](std::optional<CHandle>& handle)
	{
		if (handle && GetSafeUI(*handle))
			GET_SINGLE(UIManager)->DeleteUIRecursive(*handle);
		handle.reset();
	};
	removeEffect(m_CategoryGradientEffect);
	removeEffect(m_CategorySphereEffect);
	for (auto& splatterHandle : m_CategorySplatterEffects)
		removeEffect(splatterHandle);
	m_fCategoryEffectTime = 0.f;
	m_fCategoryEffectVisibility = 0.f;
}

void CGeneralButton::PlayCategoryEffectVisibility(_bool visible)
{
	if (!m_pComTween)
	{
		m_fCategoryEffectVisibility = visible ? 1.f : 0.f;
		if (!visible)
			DeleteCategorySelectionEffects();
		return;
	}

	const CHandle handle = GetHandle();
	const _float targetVisibility = visible ? 1.f : 0.f;
	std::function<void()> onComplete{};
	if (!visible)
	{
		onComplete = [handle]()
		{
			auto* button = E::CGameInstance::Get().
				GetGameObjectByHandleT<CGeneralButton>(handle);
			if (button && !button->m_bSelected)
				button->DeleteCategorySelectionEffects();
		};
	}

	m_pComTween->PlayTween(
		m_fCategoryEffectVisibility,
		targetVisibility,
		SELECTION_EFFECT_FADE_DURATION,
		[handle](_float value)
		{
			auto* button = E::CGameInstance::Get().
				GetGameObjectByHandleT<CGeneralButton>(handle);
			if (button)
				button->m_fCategoryEffectVisibility = value;
		},
		onComplete,
		EEaseType::EaseOutQuad);
}

void CGeneralButton::UpdateCategorySelectionEffect(_float fTimeDelta)
{
	const _float safeDelta = std::isfinite(fTimeDelta) && fTimeDelta > 0.f ?
		std::min(fTimeDelta, 0.05f) : 0.f;
	m_fCategoryEffectTime = std::fmod(
		m_fCategoryEffectTime + safeDelta,
		CATEGORY_SPLATTER_LOOP_DURATION * 16.f);

	const _float pulse =
		0.5f + 0.5f * std::sin(m_fCategoryEffectTime * 2.8f);
	if (m_CategoryGradientEffect)
	{
		if (auto* gradient = GetSafeUI(*m_CategoryGradientEffect))
			gradient->SetAlphaRatio(
				(0.42f + pulse * 0.10f) * m_fCategoryEffectVisibility);
	}
	if (m_CategorySphereEffect)
	{
		if (auto* sphere = GetSafeUI(*m_CategorySphereEffect))
			sphere->SetAlphaRatio(
				(0.09f + pulse * 0.05f) * m_fCategoryEffectVisibility);
	}

	static constexpr _float phases[] = { 0.f, 0.27f, 0.55f, 0.78f };
	static constexpr _float baseX[] = { -18.f, -6.f, 8.f, 18.f };
	for (size_t i = 0; i < m_CategorySplatterEffects.size(); ++i)
	{
		if (!m_CategorySplatterEffects[i])
			continue;
		auto* splatter = GetSafeUI(*m_CategorySplatterEffects[i]);
		if (!splatter)
			continue;

		const _float progress = std::fmod(
			m_fCategoryEffectTime / CATEGORY_SPLATTER_LOOP_DURATION +
				phases[i],
			1.f);
		const _float spread = std::sin(progress * XM_2PI + phases[i]) * 4.f;
		splatter->GetUIInfo().LocalX = baseX[i] + spread;
		splatter->GetUIInfo().LocalY = 8.f - progress * 92.f;
		splatter->GetUIInfo().LocalRot = progress * 28.f;
		splatter->SetLocalScaleRatio(0.55f + progress * 0.60f);
		splatter->SetAlphaRatio(
			std::sin(progress * XM_PI) * 0.22f *
			m_fCategoryEffectVisibility);
	}
}

_bool CGeneralButton::TryGetButtonIndex(
	const CGeneralButton* button,
	uint32_t& index)
{
	if (!button || button->m_sCommandParameter.empty())
		return false;

	index = 0;
	for (const char digit : button->m_sCommandParameter)
	{
		if (digit < '0' || digit > '9')
			return false;
		index = index * 10u + static_cast<uint32_t>(digit - '0');
	}
	return true;
}

const char* CGeneralButton::GetSelectionEffectPrefab() const
{
	switch (m_eButtonType)
	{
	case GENERAL_BUTTON_TYPE::WAND_ITEM:
		return "WandSelectEffect";
	case GENERAL_BUTTON_TYPE::WAND_MATERIAL:
		return "TextureSelectEffect";
	default:
		return nullptr;
	}
}

_float CGeneralButton::GetSelectedScale() const
{
	return m_fBaseScale * SELECTED_SCALE_RATIO;
}

void CGeneralButton::PlaySelectionEffectFadeIn(CHandle effectHandle)
{
	auto* effect = GetSafeUI(effectHandle);
	if (!effect)
		return;

	effect->SetAlphaRatio(0.f);
	if (auto* tween = effect->GetTweenCom())
	{
		tween->PlayTween(
			0.f,
			1.f,
			SELECTION_EFFECT_FADE_DURATION,
			[effectHandle](_float value)
			{
				if (auto* ui = GetSafeUI(effectHandle))
					ui->SetAlphaRatio(value);
			},
			nullptr,
			EEaseType::EaseOutQuad);
	}
	else
	{
		effect->SetAlphaRatio(1.f);
	}
}

void CGeneralButton::CreateSelectionEffect()
{
	if (m_SelectionEffect && GetSafeUI(*m_SelectionEffect))
		return;

	const char* prefabName = GetSelectionEffectPrefab();
	if (!prefabName)
		return;

	const std::filesystem::path effectPath =
		std::filesystem::path(SELECTION_EFFECT_BASE_PATH) /
		(prefabName + std::string(".json"));
	if (!std::filesystem::exists(effectPath))
		return;

	auto handles = GET_SINGLE(UIManager)->LoadPrefab(
		prefabName,
		SELECTION_EFFECT_BASE_PATH);
	if (handles.empty())
		return;

	m_SelectionEffect = handles.front();
	auto* effect = GetSafeUI(*m_SelectionEffect);
	if (!effect)
	{
		m_SelectionEffect.reset();
		return;
	}

	const _float authoredScale = effect->GetScaleRatio();
	effect->SetParent(GetHandle());
	AddChildren(effect->GetHandle());
	auto& effectInfo = effect->GetUIInfo();
	effectInfo.LocalX = 0.f;
	effectInfo.LocalY = 0.f;
	effectInfo.WeightOffset = 2;
	effect->SetLocalScaleRatio(
		authoredScale / std::max(m_fBaseScale, FLT_EPSILON));
	effect->SetAlphaRatio(0.f);
	effect->CalcUICoord();

	// A newly loaded texture UI clears all tweens while processing its first
	// APPEAR state. Register the fade there so it starts after that reset.
	const CHandle effectHandle = effect->GetHandle();
	effect->Appear = [effectHandle](CUIObject*)
	{
		PlaySelectionEffectFadeIn(effectHandle);
	};
}

void CGeneralButton::RemoveSelectionEffect()
{
	if (!m_SelectionEffect)
		return;

	const CHandle effectHandle = *m_SelectionEffect;
	m_SelectionEffect.reset();

	auto* effect = GetSafeUI(effectHandle);
	if (!effect)
		return;
	effect->Appear = {};

	if (auto* tween = effect->GetTweenCom())
	{
		const _float startAlphaRatio = effect->GetAlphaRatio();
		tween->ClearTweens();
		tween->PlayTween(
			startAlphaRatio,
			0.f,
			SELECTION_EFFECT_FADE_DURATION,
			[effectHandle](_float value)
			{
				if (auto* ui = GetSafeUI(effectHandle))
					ui->SetAlphaRatio(value);
			},
			[effectHandle]()
			{
				if (GetSafeUI(effectHandle))
					GET_SINGLE(UIManager)->DeleteUIRecursive(effectHandle);
			},
			EEaseType::EaseOutQuad);
	}
	else
	{
		GET_SINGLE(UIManager)->DeleteUIRecursive(effectHandle);
	}
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
