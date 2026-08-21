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
	void SelectFirstWandMaterial() const;
	void UpdateCenterWandTexture() const;
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
	static void PlaySelectionEffectFadeIn(CHandle effectHandle);
	void CreateSelectionEffect();
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
	_bool m_bBaseScaleCaptured{};
	_bool m_bWandTransitionBaseCaptured{};
	_bool m_bCategoryFrameColorCaptured{};
	_bool m_bHovering{};
	_bool m_bSelected{};

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
	static std::optional<CHandle> s_SelectedWandItem;
	static std::optional<CHandle> s_SelectedWandMaterial;
	static std::optional<CHandle> s_SelectedWandCategory;

public:
	static E::UPtr<CGeneralButton> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
