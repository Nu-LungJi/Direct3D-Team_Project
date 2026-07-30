#pragma once

#include "UITex.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CButtonComponent;
class TweenComponent;
NS_END

NS_BEGIN(Client)
class CEffectUI;

class CButton final : public E::CUITex
{
public:
	DECLARE_DERIVED_TYPE(CButton, E::CUITex)

private:
	CButton();
	~CButton() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

public:
	void SetMouseTracking(_bool isTracking) { m_bMouseTracking = isTracking; }
	void SetEffectHovered(std::optional<CHandle> effectUIHandle) { m_Effect_Hovered_Handle = effectUIHandle; }
	void SetEffectClicked(std::optional<CHandle> effectUIHandle) { m_Effect_Clicked_Handle = effectUIHandle; }

	void SetClickTargetName(std::string targetName) { ClickTargetName = targetName; }
	std::function<void(CUIObject* pCaller)> ClickFunc;
private:
	_bool m_bMouseTracking{ false };

private:
	bool m_bOutline{};

	CComConstantBuffer* m_pComCBufferPerUI = nullptr;
	CButtonComponent* m_pComCButton = nullptr;

protected:
	virtual void PlayEffect(uint32_t uiState);

	_bool m_EffectLoad = false;
	std::optional<CHandle> m_Effect_Hovered_Handle = std::nullopt;
	std::optional<CHandle> m_Effect_Clicked_Handle = std::nullopt;

	std::string ClickTargetName = "0SpellDesc";

	float m_fHoverScale = 1.1f;
	float m_fScaleDuration = 0.1f;
	float m_fAlphaDuration = 0.3f;
private:
	std::vector<std::optional<CHandle>> m_vEffects;
	void ClearEffectTweens();
	void ClearHoveredEffect();
	void ClearClickEffect();

private:
	std::string m_EffectTag;

	/******스펠 디스크립션*****/
	CHandle m_SpellDesc;
	CHandle m_SpellPaper;
private:
	/*******스펠버튼*****/
	void SpellBtnSet();
	void PlayScaleAlphaDownDelete(CHandle pHandle);
	CUIObject* SafeGetOBJ(CHandle pHandle);
private:
	uint32_t m_SpellType{};
	uint32_t m_colorType{};
	_float4 m_BGColor{ 1.f, 1.f, 1.f, 1.f };
public:
	static E::UPtr<CButton> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
