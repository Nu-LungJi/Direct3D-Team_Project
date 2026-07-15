#pragma once

#include "UITex.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CButtonComponent;
NS_END

NS_BEGIN(Client)

class CSpellMeter final : public E::CUITex
{
public:
	DECLARE_DERIVED_TYPE(CSpellMeter, E::CUITex)

private:
	CSpellMeter();
	~CSpellMeter() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

private:
	virtual void PlayEffect(uint32_t uiState);

private:
	CComConstantBuffer* m_pComCBufferPerSpellMeter = nullptr;
	CButtonComponent* m_pComCButton = nullptr;

private:
	std::vector<std::optional<CHandle>> m_vEffects;

private:
	std::string m_EffectTag;
	_float s_fAccumulatedTime;
	_float m_fCurrentAmount;

private:
	void StartCooldown(float fCooldownTime);
public:
	static E::UPtr<CSpellMeter> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
