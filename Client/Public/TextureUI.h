#pragma once

#include "UITex.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CButtonComponent;
NS_END

NS_BEGIN(Client)

class CTextureUI : public E::CUITex
{
public:
	DECLARE_DERIVED_TYPE(CTextureUI, E::CUITex)

protected:
	CTextureUI();
	~CTextureUI() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

private:
	virtual void PlayEffect(uint32_t uiState);

public:
	void SetMouseTracking(_bool isTracking) { m_bMouseTracking = isTracking; }
	void SetPathProgressMode(_bool enabled) { m_bPathProgressMode = enabled; }
	void SetPathProgress(_float progress) { m_fAmount = std::clamp(progress, 0.f, 1.f); }
	void SetPathProgressType(uint32_t type) { m_iPathProgressType = type; }
	void SetSpellAlarmFlame(uint32_t flameIndex);
	void SetAdditiveBlend(_bool enabled) { m_bAdditiveBlend = enabled; }
	void SetTextureBrightness(_float brightness) { m_fTextureBrightness = std::max(0.f, brightness); }
	_float GetTextureBrightness() const { return m_fTextureBrightness; }
	void SetAlphaMaskSource(CHandle sourceHandle) { m_AlphaMaskSource = sourceHandle; }
	void ClearAlphaMaskSource() { m_AlphaMaskSource.reset(); }
	void SetNineSliceMargins(const _float4& margins) { m_vNineSliceMargins = margins; }
	const _float4& GetNineSliceMargins() const { return m_vNineSliceMargins; }
private:
	_bool m_bMouseTracking{false};
	_bool m_bPathProgressMode{ false };
	uint32_t m_iPathProgressType{};
	_bool m_bSpellAlarmFlame{};
	_bool m_bRaceStartFlagWave{};
	_bool m_bAdditiveBlend{};
	_float m_fTextureBrightness{ 1.f };
	std::optional<CHandle> m_AlphaMaskSource{};
	_float m_fSpellAlarmFlameTime{};
	_float m_fRaceStartFlagWaveTime{};
	_float m_fSpellAlarmFlamePhase{};
	_float m_fSpellAlarmFlameSwayScale{ 1.f };
	_float m_fSpellAlarmFlameSpeed{ 1.f };
	_float4 m_vNineSliceMargins{}; // left, top, right, bottom (texture pixels)

private:
	bool m_bOutline{};

	CComConstantBuffer* m_pComCBufferPerUI = nullptr;
	CButtonComponent* m_pComCButton = nullptr;

private:
	std::vector<std::optional<CHandle>> m_vEffects;
	std::string m_EffectTag;

	_float m_fAmount = 0.f; // 디졸브용
public:
	static E::UPtr<CTextureUI> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
