#pragma once

#include "UITex.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CButtonComponent;
NS_END

NS_BEGIN(Client)

class CTextureUI final : public E::CUITex
{
public:
	DECLARE_DERIVED_TYPE(CTextureUI, E::CUITex)

private:
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
private:
	_bool m_bMouseTracking{false};

private:
	bool m_bOutline{};

	CComConstantBuffer* m_pComCBufferPerUI = nullptr;
	CButtonComponent* m_pComCButton = nullptr;

private:
	std::vector<std::optional<CHandle>> m_vEffects;

private:
	std::string m_EffectTag;

public:
	static E::UPtr<CTextureUI> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
