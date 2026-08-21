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

	void SetButtonType(GENERAL_BUTTON_TYPE type) { m_eButtonType = type; }
	GENERAL_BUTTON_TYPE GetButtonType() const { return m_eButtonType; }
	void SetCommandParameter(const std::string& parameter) { m_sCommandParameter = parameter; }
	const std::string& GetCommandParameter() const { return m_sCommandParameter; }
	void SetCommandCallback(CommandCallback callback) { m_CommandCallback = std::move(callback); }

protected:
	void PlayEffect(uint32_t uiState) override;

private:
	void HandleAppear();
	void HandleDisappear();
	void HandleEnter();
	void HandleExit();
	void HandleClick();

	void PlayScaleTo(_float targetScale, _float duration, _float delay = 0.f);
	void CreateHoverEffect();
	void RemoveHoverEffect();
	void ExecuteCommand();

private:
	GENERAL_BUTTON_TYPE m_eButtonType{ GENERAL_BUTTON_TYPE::DEFAULT };
	std::string m_sCommandParameter{};
	CommandCallback m_CommandCallback{};
	std::optional<CHandle> m_HoverEffect{};
	_float m_fBaseScale{ 1.f };
	_bool m_bHovering{};

private:
	static constexpr const char* HOVER_EFFECT_PREFAB = "ButtonGlowEffect";

public:
	static E::UPtr<CGeneralButton> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
