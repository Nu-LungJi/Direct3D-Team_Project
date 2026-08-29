#include "pch.h"
#include "GeneralButton.h"

#include "GameInstance.h"
#include "TextBox.h"
#include "TweenComponent.h"
#include "UIManager.h"

NS_USING(Client)

namespace
{
	constexpr std::array<std::string_view, 7> WAND_SHAPE_NAMES{
		"기본형", "클래식", "나선형", "자연형", "줄기형", "고리형", "비틀림 형"
	};
	constexpr std::array<std::array<std::string_view, 3>, 7> WAND_MATERIAL_NAMES{
		std::array<std::string_view, 3>{ "따뜻한 갈색", "밝은 갈색", "칙칙한 분홍색" },
		std::array<std::string_view, 3>{ "회색", "검은색", "회갈색" },
		std::array<std::string_view, 3>{ "밝은 갈색", "따뜻한 갈색", "검은색" },
		std::array<std::string_view, 3>{ "물푸레나무 갈색", "녹회색", "어두운 갈색" },
		std::array<std::string_view, 3>{ "꿀 갈색", "어두운 갈색", "따뜻한 갈색" },
		std::array<std::string_view, 3>{ "어두운 갈색", "옅은 갈색", "버프" },
		std::array<std::string_view, 3>{ "어두운 회색", "따뜻한 갈색", "옅은 갈색" }
	};
	constexpr std::array<std::string_view, 38> WAND_WOOD_NAMES{
		"가문비나무", "개양귀비나무", "검은 호두나무", "골담초", "너도밤나무",
		"느릅나무", "단풍나무", "도화나무", "라임나무", "라일락", "마가목",
		"물푸레나무", "바인/포도나무", "밤나무", "배나무", "버드나무",
		"버즘나무", "보리수나무", "붉은 참나무", "사과나무", "서나무", "소나무",
		"스네이크우드", "시더/삼나무", "아카시아", "오리나무", "흑단", "딱총나무",
		"오갈피나무", "월계수나무", "호두나무", "은단풍나무", "자작나무",
		"전나무", "주목나무", "참나무", "벚나무", "편백나무"
	};
	constexpr std::array<std::string_view, 19> WAND_FLEXIBILITY_NAMES{
		"아주 잘 휘어짐", "매우 잘 휘어짐", "잘 휘어짐", "유연함", "부드러운 유연함",
		"탄력 있음", "약간 탄력 있음", "유연한 탄력", "적당한 탄력", "보통",
		"약간 낭창거림", "약간 뻣뻣함", "뻣뻣함", "단단함", "매우 단단함",
		"딱딱함", "매우 딱딱함", "강함", "절대 안 휘어짐"
	};
	constexpr _float WAND_MIN_LENGTH_INCHES = 9.5f;
	constexpr _float WAND_LENGTH_STEP_INCHES = 0.25f;
	constexpr uint32_t WAND_LENGTH_OPTION_COUNT = 21u;
	constexpr _float WAND_SLIDER_MIN_X = -265.f;
	constexpr _float WAND_SLIDER_MAX_X = 265.f;
	constexpr _float WAND_SLIDER_TICK_Y = -2.f;

	CUIObject* FindRootedUIByName(std::string_view targetName)
	{
		const auto* uiHandles =
			E::CGameInstance::Get().GetGameObjectLayer("Layer_UI");
		if (!uiHandles)
			return nullptr;

		for (auto iter = uiHandles->rbegin(); iter != uiHandles->rend(); ++iter)
		{
			auto* ui = GetSafeUI(*iter);
			if (ui && std::string_view(ui->GetName()) == targetName)
				return ui;
		}

		return nullptr;
	}

	CUIObject* FindDirectChildByName(
		CUIObject* parent,
		std::string_view targetName)
	{
		if (!parent)
			return nullptr;
		for (const CHandle childHandle : parent->GetChildren())
		{
			auto* child = GetSafeUI(childHandle);
			if (child && std::string_view(child->GetName()) == targetName)
				return child;
		}
		return nullptr;
	}

	CTextBox* FindTopmostTextBoxByName(std::string_view targetName)
	{
		const auto* uiHandles =
			E::CGameInstance::Get().GetGameObjectLayer("Layer_UI");
		if (!uiHandles)
			return nullptr;

		CTextBox* result = nullptr;
		_float topY = FLT_MAX;
		for (const CHandle uiHandle : *uiHandles)
		{
			auto* ui = GetSafeUI(uiHandle);
			if (!ui || std::string_view(ui->GetName()) != targetName ||
				ui->GetUIInfo().fY >= topY)
			{
				continue;
			}
			auto* text = E::CGameInstance::Get().
				GetGameObjectByHandleT<CTextBox>(uiHandle);
			if (text)
			{
				result = text;
				topY = ui->GetUIInfo().fY;
			}
		}
		return result;
	}
}

std::optional<CHandle> CGeneralButton::s_SelectedWandItem{};
std::optional<CHandle> CGeneralButton::s_SelectedWandMaterial{};
std::optional<CHandle> CGeneralButton::s_SelectedWandCategory{};
std::optional<CHandle> CGeneralButton::s_SelectedWandCore{};
uint32_t CGeneralButton::s_iSelectedWandShapeIndex{};
uint32_t CGeneralButton::s_iSelectedWandMaterialIndex{};
uint32_t CGeneralButton::s_iSelectedWandWoodIndex{
	static_cast<uint32_t>(WAND_WOOD_NAMES.size() / 2u) };
uint32_t CGeneralButton::s_iSelectedWandLengthIndex{
	WAND_LENGTH_OPTION_COUNT / 2u };
uint32_t CGeneralButton::s_iSelectedWandFlexibilityIndex{
	static_cast<uint32_t>(WAND_FLEXIBILITY_NAMES.size() / 2u) };
uint32_t CGeneralButton::s_iSelectedWandCoreIndex{};
uint32_t CGeneralButton::s_iCurrentWandShopPageIndex{};

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
	if (s_SelectedWandCore && *s_SelectedWandCore == GetHandle())
		s_SelectedWandCore.reset();
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
	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CORE_CARD &&
		!m_bWandCoreCardInitialized)
	{
		InitializeWandCoreCard();
	}
	CTextureUI::Update(fTimeDelta);
	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_SLIDER_CURSOR)
		UpdateWandSliderInteraction();
	if (ENABLE_WAND_CATEGORY_SELECTION_VISUALS &&
		m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CATEGORY &&
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
	uint32_t coreIndex = UINT32_MAX;
	if (m_UIINFO.Name == "DragonWandCore")
		coreIndex = 0u;
	else if (m_UIINFO.Name == "UniCornWandCore")
		coreIndex = 1u;
	else if (m_UIINFO.Name == "PheonixWandCore")
		coreIndex = 2u;
	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CORE_CARD &&
		coreIndex == s_iSelectedWandCoreIndex &&
		(!s_SelectedWandCore || !GetSafeUI(*s_SelectedWandCore)))
	{
		SelectWithinGroup();
	}

	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_SLIDER_ARROW)
		InitializeWandSlider();

	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CATEGORY &&
		m_sCommandParameter == std::to_string(s_iCurrentWandShopPageIndex) &&
		(!s_SelectedWandCategory || !GetSafeUI(*s_SelectedWandCategory)))
	{
		SelectWithinGroup();
	}

	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_ITEM &&
		m_sCommandParameter == std::to_string(s_iSelectedWandShapeIndex) &&
		(!s_SelectedWandItem || !GetSafeUI(*s_SelectedWandItem)))
	{
		SelectWithinGroup();
		UpdateWandMaterialTextures(false);
		SelectWandMaterial(s_iSelectedWandMaterialIndex);
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
	case GENERAL_BUTTON_TYPE::WAND_SLIDER_ARROW:
	case GENERAL_BUTTON_TYPE::WAND_SLIDER_CURSOR:
		break;
	case GENERAL_BUTTON_TYPE::WAND_CORE_CARD:
		PlayWandCoreCardHover(true);
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
	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CATEGORY ||
		m_eButtonType == GENERAL_BUTTON_TYPE::WAND_SLIDER_ARROW ||
		m_eButtonType == GENERAL_BUTTON_TYPE::WAND_SLIDER_CURSOR)
		return;
	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CORE_CARD)
	{
		PlayWandCoreCardHover(false);
		return;
	}
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
	PlayButtonSelectSound();
	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_SLIDER_ARROW)
	{
		ChangeWandSliderOption();
		return;
	}
	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_SLIDER_CURSOR)
		return;
	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CORE_CARD)
	{
		SelectWithinGroup();
		ExecuteCommand();
		return;
	}
	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CATEGORY)
	{
		SelectWithinGroup();
		uint32_t pageIndex = 0u;
		if (TryGetButtonIndex(this, pageIndex))
			GET_SINGLE(UIManager)->OpenWandShopPage(pageIndex);
		ExecuteCommand();
		return;
	}

	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_ITEM ||
		m_eButtonType == GENERAL_BUTTON_TYPE::WAND_MATERIAL)
	{
		SelectWithinGroup();
		if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_ITEM)
		{
			s_iSelectedWandMaterialIndex = 0u;
			SelectWandMaterial(0u);
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
	case GENERAL_BUTTON_TYPE::WAND_SLIDER_ARROW:
	case GENERAL_BUTTON_TYPE::WAND_SLIDER_CURSOR:
	case GENERAL_BUTTON_TYPE::WAND_CORE_CARD:
	case GENERAL_BUTTON_TYPE::DEFAULT:
	default:
		ExecuteCommand();
		break;
	}
}

void CGeneralButton::PlayButtonSelectSound() const
{
	auto* soundManager = E::CGameInstance::Get().GetSoundManager();
	if (!soundManager)
		return;

	soundManager->Play2D(
		"./Resources/SampleClient/Sound/UI/ButtonSelect.wav",
		SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::UI,
			.fVolume = 1.f,
			.fPitch = 1.f,
			.iPriority = 64,
			.bLoop = false
		});
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
		if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CORE_CARD)
		{
			CreateWandCoreSelectionEffect();
		}
		else if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CATEGORY)
		{
			if (ENABLE_WAND_CATEGORY_SELECTION_VISUALS)
			{
				CreateCategorySelectionEffect();
				PlayCategoryEffectVisibility(true);
				PlayScaleTo(GetSelectedScale(), WAND_SELECTION_SCALE_DURATION);
			}
			else
			{
				DeleteCategorySelectionEffects();
				SetScaleRatio(m_fBaseScale);
			}
		}
		else
		{
			CreateSelectionEffect();
			PlayScaleTo(GetSelectedScale(), WAND_SELECTION_SCALE_DURATION);
		}
	}
	else
	{
		if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CORE_CARD)
		{
			RemoveSelectionEffect();
		}
		else if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CATEGORY)
		{
			if (ENABLE_WAND_CATEGORY_SELECTION_VISUALS)
			{
				RemoveCategorySelectionEffect();
				PlayScaleTo(m_fBaseScale, WAND_SELECTION_SCALE_DURATION);
			}
			else
			{
				DeleteCategorySelectionEffects();
				SetScaleRatio(m_fBaseScale);
			}
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
		if (s_SelectedWandCore &&
			*s_SelectedWandCore == GetHandle())
		{
			s_SelectedWandCore.reset();
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
	else if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CORE_CARD)
		selectedGroup = &s_SelectedWandCore;

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
	if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CORE_CARD)
	{
		if (m_UIINFO.Name == "DragonWandCore")
			s_iSelectedWandCoreIndex = 0u;
		else if (m_UIINFO.Name == "UniCornWandCore")
			s_iSelectedWandCoreIndex = 1u;
		else if (m_UIINFO.Name == "PheonixWandCore")
			s_iSelectedWandCoreIndex = 2u;
	}
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

void CGeneralButton::SelectWandMaterial(uint32_t materialIndex) const
{
	materialIndex = std::min(materialIndex, 2u);
	const std::string buttonName =
		"WandTexture" + std::to_string(materialIndex + 1u);
	auto* materialUI = FindRootedUIByName(buttonName);
	if (!materialUI)
		return;

	auto* material = E::CGameInstance::Get().
		GetGameObjectByHandleT<CGeneralButton>(materialUI->GetHandle());
	if (material)
		material->SelectWithinGroup();
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

	s_iSelectedWandShapeIndex = wandShapeIndex;
	s_iSelectedWandMaterialIndex = materialIndex;
	if (auto* styleTextUI = FindRootedUIByName("TextColor"))
	{
		if (auto* styleText = E::CGameInstance::Get().
			GetGameObjectByHandleT<CTextBox>(styleTextUI->GetHandle()))
		{
			styleText->SetwText(StringToWUTF8(GetSelectedWandStyleText()));
		}
	}
}

std::string CGeneralButton::GetSelectedWandShapeName()
{
	const size_t shapeIndex = std::min<size_t>(
		s_iSelectedWandShapeIndex,
		WAND_SHAPE_NAMES.size() - 1u);
	return std::string(WAND_SHAPE_NAMES[shapeIndex]);
}

std::string CGeneralButton::GetSelectedWandMaterialName()
{
	const size_t shapeIndex = std::min<size_t>(
		s_iSelectedWandShapeIndex,
		WAND_MATERIAL_NAMES.size() - 1u);
	const size_t materialIndex = std::min<size_t>(
		s_iSelectedWandMaterialIndex,
		WAND_MATERIAL_NAMES[shapeIndex].size() - 1u);
	return std::string(WAND_MATERIAL_NAMES[shapeIndex][materialIndex]);
}

std::string CGeneralButton::GetSelectedWandStyleText()
{
	return GetSelectedWandShapeName() + " - " +
		GetSelectedWandMaterialName();
}

std::string CGeneralButton::GetSelectedWandWoodName()
{
	const size_t index = std::min<size_t>(
		s_iSelectedWandWoodIndex,
		WAND_WOOD_NAMES.size() - 1u);
	return std::string(WAND_WOOD_NAMES[index]);
}

_float CGeneralButton::GetSelectedWandLengthInches()
{
	const uint32_t index = std::min(
		s_iSelectedWandLengthIndex,
		WAND_LENGTH_OPTION_COUNT - 1u);
	return WAND_MIN_LENGTH_INCHES +
		static_cast<_float>(index) * WAND_LENGTH_STEP_INCHES;
}

std::string CGeneralButton::GetSelectedWandLengthText()
{
	std::string text = std::to_string(GetSelectedWandLengthInches());
	while (!text.empty() && text.back() == '0')
		text.pop_back();
	if (!text.empty() && text.back() == '.')
		text.pop_back();
	return text + "인치";
}

std::string CGeneralButton::GetSelectedWandFlexibilityName()
{
	const size_t index = std::min<size_t>(
		s_iSelectedWandFlexibilityIndex,
		WAND_FLEXIBILITY_NAMES.size() - 1u);
	return std::string(WAND_FLEXIBILITY_NAMES[index]);
}

std::string CGeneralButton::GetSelectedWandCoreName()
{
	static constexpr std::array<std::string_view, 3> coreNames{
		"용의 심금", "유니콘 털", "불사조 깃털"
	};
	return std::string(coreNames[std::min<size_t>(
		s_iSelectedWandCoreIndex, coreNames.size() - 1u)]);
}

void CGeneralButton::ResetWandShopSelection()
{
	s_SelectedWandItem.reset();
	s_SelectedWandMaterial.reset();
	s_SelectedWandCategory.reset();
	s_SelectedWandCore.reset();

	s_iSelectedWandShapeIndex = 0u;
	s_iSelectedWandMaterialIndex = 0u;
	s_iSelectedWandWoodIndex =
		static_cast<uint32_t>(WAND_WOOD_NAMES.size() / 2u);
	s_iSelectedWandLengthIndex = WAND_LENGTH_OPTION_COUNT / 2u;
	s_iSelectedWandFlexibilityIndex =
		static_cast<uint32_t>(WAND_FLEXIBILITY_NAMES.size() / 2u);
	s_iSelectedWandCoreIndex = 0u;
	s_iCurrentWandShopPageIndex = 0u;
}

void CGeneralButton::SetCurrentWandShopPage(uint32_t pageIndex)
{
	s_iCurrentWandShopPageIndex = std::min(pageIndex, 3u);
}

void CGeneralButton::RefreshWandShopSummary()
{
	char resourceTag[64]{};
	sprintf_s(resourceTag, "TEX_UI_T_T%03u_M%02u",
		s_iSelectedWandShapeIndex, s_iSelectedWandMaterialIndex);
	if (auto* wandImage = FindRootedUIByName("WandImage"))
		wandImage->SetResTag(resourceTag);

	auto setText = [](std::string_view name, const std::string& value)
	{
		auto* ui = FindRootedUIByName(name);
		if (!ui)
			return;
		if (auto* text = E::CGameInstance::Get().
			GetGameObjectByHandleT<CTextBox>(ui->GetHandle()))
		{
			text->SetwText(StringToWUTF8(value));
		}
	};

	if (auto* styleText = FindTopmostTextBoxByName("WandType"))
		styleText->SetwText(StringToWUTF8(GetSelectedWandStyleText()));
	setText("WandTypeText", GetSelectedWandWoodName());
	setText("WorldAgentTypeText", GetSelectedWandCoreName());
	setText("pliabilityText", GetSelectedWandFlexibilityName());
	setText("WandLengthText", GetSelectedWandLengthText());
}

void CGeneralButton::RefreshWandShopCommonUI(uint32_t pageIndex)
{
	static constexpr std::array<std::string_view, 4> pageTitles{
		"지팡이 완성 모습", "지팡이 스타일", "지팡이 종류", "지팡이 심 종류"
	};
	SetCurrentWandShopPage(pageIndex);
	if (auto* titleUI = FindRootedUIByName("Title"))
	{
		if (auto* title = E::CGameInstance::Get().
			GetGameObjectByHandleT<CTextBox>(titleUI->GetHandle()))
		{
			title->SetwText(StringToWUTF8(std::string(pageTitles[
				std::min<size_t>(pageIndex, pageTitles.size() - 1u)])));
		}
	}
	RefreshWandShopSummary();
}

void CGeneralButton::InitializeWandSlider() const
{
	const size_t separator = m_sCommandParameter.find(':');
	if (separator == std::string::npos)
		return;
	const std::string sliderText = m_sCommandParameter.substr(0, separator);
	if (sliderText.empty() || !std::ranges::all_of(sliderText,
		[](const char character) { return character >= '0' && character <= '9'; }))
	{
		return;
	}
	RefreshWandSlider(static_cast<uint32_t>(std::stoul(sliderText)), false);
}

void CGeneralButton::ChangeWandSliderOption()
{
	const size_t separator = m_sCommandParameter.find(':');
	if (separator == std::string::npos)
		return;
	const std::string sliderText = m_sCommandParameter.substr(0, separator);
	const std::string directionText = m_sCommandParameter.substr(separator + 1u);
	if (sliderText.empty() || directionText.empty())
		return;

	const uint32_t sliderIndex = static_cast<uint32_t>(std::stoul(sliderText));
	const int direction = std::stoi(directionText);
	uint32_t* selectedIndex = nullptr;
	uint32_t optionCount = 0u;
	switch (sliderIndex)
	{
	case 0u:
		selectedIndex = &s_iSelectedWandWoodIndex;
		optionCount = static_cast<uint32_t>(WAND_WOOD_NAMES.size());
		break;
	case 1u:
		selectedIndex = &s_iSelectedWandLengthIndex;
		optionCount = WAND_LENGTH_OPTION_COUNT;
		break;
	case 2u:
		selectedIndex = &s_iSelectedWandFlexibilityIndex;
		optionCount = static_cast<uint32_t>(WAND_FLEXIBILITY_NAMES.size());
		break;
	default:
		return;
	}

	const int nextIndex = std::clamp(
		static_cast<int>(*selectedIndex) + (direction < 0 ? -1 : 1),
		0,
		static_cast<int>(optionCount) - 1);
	if (*selectedIndex == static_cast<uint32_t>(nextIndex))
		return;
	*selectedIndex = static_cast<uint32_t>(nextIndex);
	RefreshWandSlider(sliderIndex, true);
}

void CGeneralButton::UpdateWandSliderInteraction()
{
	if (!GetParent())
		return;
	auto* slider = GetSafeUI(*GetParent());
	if (!slider)
		return;

	const _float2 mouse = GET_SINGLE(UIManager)->GetUIInteractionMousePosition();
	const UI_INFO& sliderInfo = slider->GetUIInfo();
	const _float relativeX = mouse.x - sliderInfo.fX;
	const _float relativeY = mouse.y - sliderInfo.fY;
	const _bool hovered = std::abs(relativeX) <= 325.f &&
		std::abs(relativeY) <= 28.f;
	if (hovered != m_bSliderTrackHovered)
		SetWandSliderHovered(hovered);

	if (hovered && std::abs(relativeX) <= 282.f &&
		E::CGameInstance::Get().MouseDown(MOUSEKEYSTATE::LB))
	{
		m_bSliderDragging = true;
	}
	if (m_bSliderDragging &&
		E::CGameInstance::Get().MousePressing(MOUSEKEYSTATE::LB))
	{
		UpdateWandSliderFromMouse();
	}
	if (m_bSliderDragging &&
		E::CGameInstance::Get().MouseUp(MOUSEKEYSTATE::LB))
	{
		m_bSliderDragging = false;
	}
}

void CGeneralButton::UpdateWandSliderFromMouse()
{
	if (!GetParent() || m_sCommandParameter.empty())
		return;
	auto* slider = GetSafeUI(*GetParent());
	if (!slider)
		return;

	const uint32_t sliderIndex = static_cast<uint32_t>(
		std::stoul(m_sCommandParameter));
	uint32_t* selectedIndex = nullptr;
	uint32_t optionCount = 0u;
	switch (sliderIndex)
	{
	case 0u:
		selectedIndex = &s_iSelectedWandWoodIndex;
		optionCount = static_cast<uint32_t>(WAND_WOOD_NAMES.size());
		break;
	case 1u:
		selectedIndex = &s_iSelectedWandLengthIndex;
		optionCount = WAND_LENGTH_OPTION_COUNT;
		break;
	case 2u:
		selectedIndex = &s_iSelectedWandFlexibilityIndex;
		optionCount = static_cast<uint32_t>(WAND_FLEXIBILITY_NAMES.size());
		break;
	default:
		return;
	}

	const _float parentScale = std::max(slider->GetScaleRatio(), 0.001f);
	const _float localMouseX = std::clamp(
		(GET_SINGLE(UIManager)->GetUIInteractionMousePosition().x -
			slider->GetUIInfo().fX) / parentScale,
		WAND_SLIDER_MIN_X,
		WAND_SLIDER_MAX_X);
	const _float ratio = (localMouseX - WAND_SLIDER_MIN_X) /
		(WAND_SLIDER_MAX_X - WAND_SLIDER_MIN_X);
	const uint32_t nearestIndex = static_cast<uint32_t>(std::lround(
		ratio * static_cast<_float>(optionCount - 1u)));
	if (*selectedIndex == nearestIndex)
		return;
	*selectedIndex = nearestIndex;
	RefreshWandSlider(sliderIndex, false);
}

void CGeneralButton::SetWandSliderHovered(_bool hovered)
{
	m_bSliderTrackHovered = hovered;
	if (!GetParent())
		return;
	auto* slider = GetSafeUI(*GetParent());
	if (!slider)
		return;

	if (!m_bSliderVisualBaseCaptured)
	{
		m_fSliderBaseScale = slider->GetScaleRatio();
		m_vSliderBaseBrightness.clear();
		auto captureBrightness = [&](CUIObject* ui)
		{
			if (auto* textureUI = Engine::Cast<CTextureUI>(ui))
				m_vSliderBaseBrightness.emplace_back(
					ui->GetHandle(),
					textureUI->GetTextureBrightness());
		};
		// Keep the slider track at its original brightness. Only the arrows,
		// ticks, and cursor receive the hover brightness boost.
		for (const CHandle childHandle : slider->GetChildren())
			captureBrightness(GetSafeUI(childHandle));
		m_bSliderVisualBaseCaptured = true;
	}

	const CHandle cursorHandle = GetHandle();
	auto applyAmount = [cursorHandle](_float value)
	{
		if (auto* cursorButton = E::CGameInstance::Get().
			GetGameObjectByHandleT<CGeneralButton>(cursorHandle))
		{
			cursorButton->ApplyWandSliderHoverVisual(value);
		}
	};
	if (auto* tween = slider->GetTweenCom())
	{
		tween->ClearTweens();
		tween->PlayTween(
			m_fSliderHoverAmount,
			hovered ? 1.f : 0.f,
			0.14f,
			applyAmount,
			nullptr,
			EEaseType::EaseOutQuad);
	}
	else
	{
		applyAmount(hovered ? 1.f : 0.f);
	}
}

void CGeneralButton::ApplyWandSliderHoverVisual(_float amount)
{
	m_fSliderHoverAmount = std::clamp(amount, 0.f, 1.f);
	if (!GetParent())
		return;
	auto* slider = GetSafeUI(*GetParent());
	if (!slider)
		return;

	slider->SetScaleRatio(std::lerp(
		m_fSliderBaseScale,
		m_fSliderBaseScale * 1.025f,
		m_fSliderHoverAmount));
	for (const auto& [handle, baseBrightness] : m_vSliderBaseBrightness)
	{
		if (auto* textureUI = E::CGameInstance::Get().
			GetGameObjectByHandleT<CTextureUI>(handle))
		{
			textureUI->SetTextureBrightness(baseBrightness *
				(1.f + 0.40f * m_fSliderHoverAmount));
			textureUI->CalcUICoord();
		}
	}
	slider->CalcUICoord();
}

void CGeneralButton::InitializeWandCoreCard()
{
	CUIObject* background = nullptr;
	CUIObject* WorldAgent = nullptr;
	for (const CHandle childHandle : GetChildren())
	{
		auto* child = GetSafeUI(childHandle);
		if (!child)
			continue;

		const std::string_view childName = child->GetName();
		if (childName.find("CoreBG") != std::string_view::npos)
		{
			background = child;
			continue;
		}
		if (childName == "Dragon" || childName == "UniCorn" ||
			childName == "Pheonix")
		{
			WorldAgent = child;
		}
	}

	if (!background || !WorldAgent)
		return;

	m_WandCoreBackground = background->GetHandle();
	m_WandCoreWorldAgent = WorldAgent->GetHandle();
	m_fWandCoreBackgroundBaseScale = background->GetLocalScaleRatio();
	m_fWandCoreWorldAgentBaseScale = WorldAgent->GetLocalScaleRatio();
	m_vWandCoreBaseBrightness.clear();
	std::function<void(CUIObject*)> collectTextureBrightness =
		[this, &collectTextureBrightness](CUIObject* ui)
		{
			if (!ui)
				return;
			if (auto* texture = E::CGameInstance::Get().
				GetGameObjectByHandleT<CTextureUI>(ui->GetHandle()))
			{
				m_vWandCoreBaseBrightness.emplace_back(
					ui->GetHandle(), texture->GetTextureBrightness());
			}
			for (const CHandle childHandle : ui->GetChildren())
				collectTextureBrightness(GetSafeUI(childHandle));
		};
	collectTextureBrightness(this);
	if (auto* textureBackground = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(background->GetHandle()))
	{
		textureBackground->SetAlphaMaskSource(GetHandle());
	}
	if (auto* textureWorldAgent = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(WorldAgent->GetHandle()))
	{
		textureWorldAgent->SetAlphaMaskSource(GetHandle());
	}
	m_bWandCoreCardInitialized = true;
}

void CGeneralButton::PlayWandCoreCardHover(_bool hovered)
{
	if (!m_bWandCoreCardInitialized)
		InitializeWandCoreCard();
	if (!m_WandCoreBackground || !m_WandCoreWorldAgent)
		return;

	PlayChildLocalScaleTo(
		*m_WandCoreBackground,
		hovered ? 0.95f : m_fWandCoreBackgroundBaseScale,
		WAND_CORE_HOVER_DURATION);
	PlayChildLocalScaleTo(
		*m_WandCoreWorldAgent,
		hovered ? m_fWandCoreWorldAgentBaseScale * 1.06f :
			m_fWandCoreWorldAgentBaseScale,
		WAND_CORE_HOVER_DURATION);

	const _float targetAmount = hovered ? 1.f : 0.f;
	if (m_pComTween)
	{
		m_pComTween->ClearTweens();
		const CHandle safeHandle = GetHandle();
		m_pComTween->PlayTween(
			m_fWandCoreHoverAmount,
			targetAmount,
			WAND_CORE_HOVER_DURATION,
			[safeHandle](_float amount)
			{
				if (auto* button = E::CGameInstance::Get().
					GetGameObjectByHandleT<CGeneralButton>(safeHandle))
				{
					button->ApplyWandCoreCardHoverVisual(amount);
				}
			},
			nullptr,
			EEaseType::EaseOutQuad);
	}
	else
	{
		ApplyWandCoreCardHoverVisual(targetAmount);
	}
}

void CGeneralButton::ApplyWandCoreCardHoverVisual(_float amount)
{
	m_fWandCoreHoverAmount = std::clamp(amount, 0.f, 1.f);
	SetScaleRatio(std::lerp(
		m_fBaseScale,
		m_fBaseScale * WAND_CORE_HOVER_SCALE,
		m_fWandCoreHoverAmount));
	CalcUICoord();

	const _float brightnessMultiplier = std::lerp(
		1.f,
		WAND_CORE_HOVER_BRIGHTNESS,
		m_fWandCoreHoverAmount);
	for (const auto& [handle, baseBrightness] : m_vWandCoreBaseBrightness)
	{
		if (auto* texture = E::CGameInstance::Get().
			GetGameObjectByHandleT<CTextureUI>(handle))
		{
			texture->SetTextureBrightness(
				baseBrightness * brightnessMultiplier);
		}
	}
}

void CGeneralButton::PlayChildLocalScaleTo(
	CHandle childHandle,
	_float targetScale,
	_float duration)
{
	auto* child = GetSafeUI(childHandle);
	if (!child)
		return;

	const _float startScale = child->GetLocalScaleRatio();
	const CHandle safeHandle = childHandle;
	if (auto* tween = child->GetTweenCom())
	{
		tween->ClearTweens();
		tween->PlayTween(
			startScale,
			targetScale,
			duration,
			[safeHandle](_float value)
			{
				if (auto* ui = GetSafeUI(safeHandle))
				{
					ui->SetLocalScaleRatio(value);
					ui->CalcUICoord();
				}
			},
			nullptr,
			EEaseType::EaseOutQuad);
	}
	else
	{
		child->SetLocalScaleRatio(targetScale);
		child->CalcUICoord();
	}
}

void CGeneralButton::RefreshWandSlider(
	uint32_t sliderIndex,
	_bool animateCursor)
{
	if (sliderIndex > 2u)
		return;

	const std::string sliderName =
		"Slider" + std::to_string(sliderIndex + 1u);
	auto* slider = FindRootedUIByName(sliderName);
	if (!slider)
		return;

	uint32_t optionCount = 0u;
	uint32_t selectedIndex = 0u;
	const char* textName = nullptr;
	std::string displayedText{};
	switch (sliderIndex)
	{
	case 0u:
		optionCount = static_cast<uint32_t>(WAND_WOOD_NAMES.size());
		selectedIndex = std::min(s_iSelectedWandWoodIndex, optionCount - 1u);
		textName = "WandKind";
		displayedText = "지팡이 종류 : " + GetSelectedWandWoodName();
		break;
	case 1u:
		optionCount = WAND_LENGTH_OPTION_COUNT;
		selectedIndex = std::min(s_iSelectedWandLengthIndex, optionCount - 1u);
		textName = "WandLength";
		displayedText = "길이 : " + GetSelectedWandLengthText();
		break;
	case 2u:
		optionCount = static_cast<uint32_t>(WAND_FLEXIBILITY_NAMES.size());
		selectedIndex = std::min(s_iSelectedWandFlexibilityIndex, optionCount - 1u);
		textName = "WandLength_Copy";
		displayedText = "유연성 : " + GetSelectedWandFlexibilityName();
		break;
	}

	if (auto* textUI = FindRootedUIByName(textName))
	{
		if (auto* textBox = E::CGameInstance::Get().
			GetGameObjectByHandleT<CTextBox>(textUI->GetHandle()))
		{
			textBox->SetwText(StringToWUTF8(displayedText));
		}
	}

	const uint32_t levelID = E::CGameInstance::Get().GetCurrentLevelID();
	const std::string currentLevel = levelID > 100u ?
		"LEVEL_LOADING" :
		_string("LEVEL_") + MagicEnumToStringView(
			static_cast<LEVEL>(levelID)).data();
	auto createSliderChild = [&](const std::string& name,
		const std::string& resourceTag, const _float2& size,
		_float localX, _float localY, int weightOffset,
		_bool interactive) -> CUIObject*
	{
		CTextureUI::UIOBJECT_DESC desc{};
		desc.sObjectTag = name;
		desc.Name = name;
		desc.fX = slider->GetUIInfo().fX;
		desc.fY = slider->GetUIInfo().fY;
		desc.fSizeX = size.x;
		desc.fSizeY = size.y;
		desc.fAlpha = 1.f;
		desc.ResTag = resourceTag;
		desc.ResWeight = slider->GetUIInfo().Weight + weightOffset;
		desc.UIType = interactive ?
			ETOUI(UI_TYPE::GENERAL_BUTTON) : ETOUI(UI_TYPE::TEXUI);
		const auto handle = E::CGameInstance::Get().AddGameObjectToLayer(
			currentLevel,
			interactive ? "Prototype_GameObject_GeneralButton" :
				"Prototype_GameObject_TextureUI",
			"Layer_UI",
			&desc);
		if (!handle)
			return nullptr;
		auto* child = GetSafeUI(*handle);
		if (!child)
			return nullptr;
		child->SetRenderGroupOverride(slider->GetResolvedRenderGroup());
		child->SetParent(slider->GetHandle());
		slider->AddChildren(*handle);
		child->GetUIInfo().LocalX = localX;
		child->GetUIInfo().LocalY = localY;
		child->SetInputLcok(!interactive);
		if (interactive)
		{
			if (auto* button = E::CGameInstance::Get().
				GetGameObjectByHandleT<CGeneralButton>(*handle))
			{
				button->SetButtonType(GENERAL_BUTTON_TYPE::WAND_SLIDER_CURSOR);
				button->SetCommandParameter(std::to_string(sliderIndex));
				button->RefreshBaseScale();
			}
		}
		child->CalcUICoord();
		return child;
	};

	for (uint32_t index = 0u; index < optionCount; ++index)
	{
		const std::string tickName = sliderName + "_Tick" +
			std::to_string(index);
		if (FindDirectChildByName(slider, tickName))
			continue;
		const _float ratio = optionCount > 1u ?
			static_cast<_float>(index) / static_cast<_float>(optionCount - 1u) : 0.f;
		if (auto* tick = createSliderChild(
			tickName,
			"TEX_UI_T_LockLevel_ColorCircle",
			{ 7.f, 7.f },
			std::lerp(WAND_SLIDER_MIN_X, WAND_SLIDER_MAX_X, ratio),
			WAND_SLIDER_TICK_Y,
			1,
			false))
		{
			tick->SetColor({ 1.08f, 0.80f, 0.32f });
			tick->SetAlphaRatio(0.95f);
		}
	}

	const std::string cursorName = sliderName + "_Cursor";
	auto* cursor = FindDirectChildByName(slider, cursorName);
	if (!cursor)
	{
		cursor = createSliderChild(
			cursorName,
			"TEX_UI_T_SliderThumb",
			{ 34.f, 34.f },
			WAND_SLIDER_MIN_X,
			0.f,
			3,
			true);
	}
	if (!cursor)
		return;

	const _float targetRatio = optionCount > 1u ?
		static_cast<_float>(selectedIndex) /
		static_cast<_float>(optionCount - 1u) : 0.f;
	const _float targetX = std::lerp(
		WAND_SLIDER_MIN_X,
		WAND_SLIDER_MAX_X,
		targetRatio);
	const CHandle cursorHandle = cursor->GetHandle();
	if (animateCursor && cursor->GetTweenCom())
	{
		cursor->GetTweenCom()->ClearTweens();
		cursor->GetTweenCom()->PlayTween(
			cursor->GetUIInfo().LocalX,
			targetX,
			0.18f,
			[cursorHandle](_float value)
			{
				if (auto* cursorUI = GetSafeUI(cursorHandle))
				{
					cursorUI->GetUIInfo().LocalX = value;
					cursorUI->CalcUICoord();
				}
			},
			nullptr,
			EEaseType::EaseOutQuad);
	}
	else
	{
		cursor->GetUIInfo().LocalX = targetX;
		cursor->CalcUICoord();
	}
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

	effect->SetRenderGroupOverride(GetResolvedRenderGroup());
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

void CGeneralButton::PlaySelectionEffectFadeIn(
	CHandle effectHandle,
	_float duration)
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
			duration,
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

void CGeneralButton::CreateWandCoreSelectionEffect()
{
	if (m_SelectionEffect && GetSafeUI(*m_SelectionEffect))
		return;

	const uint32_t levelID = E::CGameInstance::Get().GetCurrentLevelID();
	const std::string currentLevel = levelID > 100u ?
		"LEVEL_LOADING" :
		_string("LEVEL_") + MagicEnumToStringView(
			static_cast<LEVEL>(levelID)).data();

	CTextureUI::UIOBJECT_DESC desc{};
	desc.sObjectTag = m_UIINFO.Name + "_SelectionBorder";
	desc.Name = desc.sObjectTag;
	desc.fX = m_UIINFO.fX;
	desc.fY = m_UIINFO.fY;
	desc.fSizeX = m_UIINFO.SizeX;
	desc.fSizeY = m_UIINFO.SizeY;
	desc.fAlpha = 1.f;
	desc.ResTag = "TEX_UI_T_CoreBorderSelect";
	desc.ResWeight = m_UIINFO.Weight + 4;
	desc.UIType = ETOUI(UI_TYPE::TEXUI);

	const auto handle = E::CGameInstance::Get().AddGameObjectToLayer(
		currentLevel,
		"Prototype_GameObject_TextureUI",
		"Layer_UI",
		&desc);
	if (!handle)
		return;

	auto* effect = E::CGameInstance::Get().
		GetGameObjectByHandleT<CTextureUI>(*handle);
	if (!effect)
		return;

	m_SelectionEffect = *handle;
	effect->SetRenderGroupOverride(GetResolvedRenderGroup());
	effect->SetParent(GetHandle());
	AddChildren(*handle);
	auto& effectInfo = effect->GetUIInfo();
	effectInfo.LocalX = 0.f;
	effectInfo.LocalY = WAND_CORE_SELECTION_LOCAL_Y;
	effectInfo.WeightOffset = 4;
	effect->SetLocalScaleRatio(WAND_CORE_SELECTION_BASE_SCALE);
	effect->SetAlphaRatio(0.f);
	effect->CalcUICoord();

	const CHandle effectHandle = *handle;
	effect->Appear = [effectHandle](CUIObject*)
	{
		if (auto* ui = GetSafeUI(effectHandle))
			ui->SetInputLcok(true);
		PlaySelectionEffectFadeIn(
			effectHandle,
			WAND_CORE_SELECTION_FADE_IN_DURATION);
	};
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
		PlaySelectionEffectFadeIn(
			effectHandle,
			SELECTION_EFFECT_FADE_DURATION);
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
		const _float startLocalScale = effect->GetLocalScaleRatio();
		tween->ClearTweens();
		if (m_eButtonType == GENERAL_BUTTON_TYPE::WAND_CORE_CARD)
		{
			tween->PlayTween(
				startLocalScale,
				startLocalScale * 1.06f,
				SELECTION_EFFECT_FADE_DURATION,
				[effectHandle](_float value)
				{
					if (auto* ui = GetSafeUI(effectHandle))
					{
						ui->SetLocalScaleRatio(value);
						ui->CalcUICoord();
					}
				},
				nullptr,
				EEaseType::EaseOutQuad);
		}
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
