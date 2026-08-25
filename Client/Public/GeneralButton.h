#pragma once

#include "TextureUI.h"
#include "UI_Enums.h"

NS_BEGIN(Client)

class CGeneralButton final : public CTextureUI
{
public:
	DECLARE_DERIVED_TYPE(CGeneralButton, CTextureUI)
	using CommandCallback = std::function<void(GENERAL_BUTTON_TYPE, const std::string&)>;

private:
	CGeneralButton();
	~CGeneralButton() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void Update(_float fTimeDelta) override;

	void SetButtonType(GENERAL_BUTTON_TYPE type) { m_eButtonType = type; }
	GENERAL_BUTTON_TYPE GetButtonType() const { return m_eButtonType; }
	void SetCommandParameter(const std::string& parameter) { m_sCommandParameter = parameter; }
	const std::string& GetCommandParameter() const { return m_sCommandParameter; }
	void SetCommandCallback(CommandCallback callback) { m_CommandCallback = std::move(callback); }
	void RefreshBaseScale()
	{
		m_fBaseScale = GetScaleRatio();
		m_bBaseScaleCaptured = true;
	}
	void SetSelected(_bool selected);
	_bool IsSelected() const { return m_bSelected; }
	static uint32_t GetSelectedWandShapeIndex() { return s_iSelectedWandShapeIndex; }
	static uint32_t GetSelectedWandMaterialIndex() { return s_iSelectedWandMaterialIndex; }
	static std::string GetSelectedWandShapeName();
	static std::string GetSelectedWandMaterialName();
	static std::string GetSelectedWandStyleText();
	static uint32_t GetSelectedWandWoodIndex() { return s_iSelectedWandWoodIndex; }
	static uint32_t GetSelectedWandLengthIndex() { return s_iSelectedWandLengthIndex; }
	static uint32_t GetSelectedWandFlexibilityIndex() { return s_iSelectedWandFlexibilityIndex; }
	static std::string GetSelectedWandWoodName();
	static _float GetSelectedWandLengthInches();
	static std::string GetSelectedWandLengthText();
	static std::string GetSelectedWandFlexibilityName();
	static std::string GetSelectedWandCoreName();
	static void ResetWandShopSelection();
	static void SetCurrentWandShopPage(uint32_t pageIndex);
	static void RefreshWandShopSummary();
	static void RefreshWandShopCommonUI(uint32_t pageIndex);

protected:
	void PlayEffect(uint32_t uiState) override;

private:
	void HandleAppear();
	void HandleDisappear();
	void HandleEnter();
	void HandleExit();
	void HandleClick();
	void EnsureBaseScaleCaptured();

	void PlayScaleTo(_float targetScale, _float duration, _float delay = 0.f);
	void CreateHoverEffect();
	void RemoveHoverEffect();
	void SelectWithinGroup();
	void UpdateWandMaterialTextures(_bool animate = true) const;
	void AnimateWandMaterialTexture(const std::string& resourceTag);
	void SelectWandMaterial(uint32_t materialIndex) const;
	void UpdateCenterWandTexture() const;
	void InitializeWandSlider() const;
	void ChangeWandSliderOption();
	void UpdateWandSliderInteraction();
	void UpdateWandSliderFromMouse();
	void SetWandSliderHovered(_bool hovered);
	void ApplyWandSliderHoverVisual(_float amount);
	void InitializeWandCoreCard();
	void PlayWandCoreCardHover(_bool hovered);
	void ApplyWandCoreCardHoverVisual(_float amount);
	void PlayChildLocalScaleTo(CHandle childHandle, _float targetScale,
		_float duration);
	static void RefreshWandSlider(uint32_t sliderIndex, _bool animateCursor);
	void CreateCategorySelectionEffect();
	void RemoveCategorySelectionEffect();
	void DeleteCategorySelectionEffects();
	void PlayCategoryEffectVisibility(_bool visible);
	void UpdateCategorySelectionEffect(_float fTimeDelta);
	std::optional<CHandle> CreateCategoryEffectTexture(
		const std::string& name,
		const std::string& resourceTag,
		const _float2& size,
		const _float2& localPosition,
		_float alphaRatio,
		int weightOffset);
	static _bool TryGetButtonIndex(const CGeneralButton* button, uint32_t& index);
	static void PlaySelectionEffectFadeIn(CHandle effectHandle, _float duration);
	void CreateSelectionEffect();
	void CreateWandCoreSelectionEffect();
	void RemoveSelectionEffect();
	const char* GetSelectionEffectPrefab() const;
	_float GetSelectedScale() const;
	void ExecuteCommand();

private:
	GENERAL_BUTTON_TYPE m_eButtonType{ GENERAL_BUTTON_TYPE::DEFAULT };
	std::string m_sCommandParameter{};
	CommandCallback m_CommandCallback{};
	std::optional<CHandle> m_HoverEffect{};
	std::optional<CHandle> m_SelectionEffect{};
	std::optional<CHandle> m_CategoryGradientEffect{};
	std::optional<CHandle> m_CategorySphereEffect{};
	std::array<std::optional<CHandle>, 4> m_CategorySplatterEffects{};
	_float m_fBaseScale{ 1.f };
	_float m_fWandTransitionBaseX{};
	_float m_fWandTransitionBaseAlpha{ 1.f };
	_float m_fCategoryEffectTime{};
	_float m_fCategoryEffectVisibility{};
	_float3 m_vCategoryFrameBaseColor{};
	std::vector<std::pair<CHandle, _float>> m_vSliderBaseBrightness{};
	_float m_fSliderBaseScale{ 1.f };
	_float m_fSliderHoverAmount{};
	_bool m_bBaseScaleCaptured{};
	_bool m_bWandTransitionBaseCaptured{};
	_bool m_bCategoryFrameColorCaptured{};
	_bool m_bHovering{};
	_bool m_bSelected{};
	_bool m_bSliderDragging{};
	_bool m_bSliderTrackHovered{};
	_bool m_bSliderVisualBaseCaptured{};
	_bool m_bWandCoreCardInitialized{};
	std::optional<CHandle> m_WandCoreBackground{};
	std::optional<CHandle> m_WandCoreWorldAgent{};
	std::vector<std::pair<CHandle, _float>> m_vWandCoreBaseBrightness{};
	_float m_fWandCoreBackgroundBaseScale{ 1.1f };
	_float m_fWandCoreWorldAgentBaseScale{ 1.f };
	_float m_fWandCoreHoverAmount{};

private:
	static constexpr const char* HOVER_EFFECT_PREFAB = "ButtonGlowEffect";
	static constexpr const char* SELECTION_EFFECT_BASE_PATH =
		"./Resources/SampleClient/UIData/RTT/";
	static constexpr _float SELECTED_SCALE_RATIO = 1.08f;
	static constexpr _float WAND_HOVER_SCALE_DURATION = 0.20f;
	static constexpr _float WAND_SELECTION_SCALE_DURATION = 0.20f;
	static constexpr _float WAND_TEXTURE_SLIDE_OFFSET = 20.f;
	static constexpr _float WAND_TEXTURE_FADE_HALF_DURATION = 0.18f;
	static constexpr _float SELECTION_EFFECT_FADE_DURATION = 0.45f;
	static constexpr _float CATEGORY_SPLATTER_LOOP_DURATION = 2.20f;
	static constexpr _float WAND_CORE_HOVER_DURATION = 0.22f;
	static constexpr _float WAND_CORE_HOVER_SCALE = 1.03f;
	static constexpr _float WAND_CORE_HOVER_BRIGHTNESS = 1.5f;
	static constexpr _float WAND_CORE_SELECTION_BASE_SCALE = 1.061f;
	static constexpr _float WAND_CORE_SELECTION_LOCAL_Y = 7.6f;
	static constexpr _float WAND_CORE_SELECTION_FADE_IN_DURATION = 0.18f;
	static std::optional<CHandle> s_SelectedWandItem;
	static std::optional<CHandle> s_SelectedWandMaterial;
	static std::optional<CHandle> s_SelectedWandCategory;
	static std::optional<CHandle> s_SelectedWandCore;
	static uint32_t s_iSelectedWandShapeIndex;
	static uint32_t s_iSelectedWandMaterialIndex;
	static uint32_t s_iSelectedWandWoodIndex;
	static uint32_t s_iSelectedWandLengthIndex;
	static uint32_t s_iSelectedWandFlexibilityIndex;
	static uint32_t s_iSelectedWandCoreIndex;
	static uint32_t s_iCurrentWandShopPageIndex;
	static constexpr _bool ENABLE_WAND_CATEGORY_SELECTION_VISUALS = true;

public:
	static E::UPtr<CGeneralButton> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
