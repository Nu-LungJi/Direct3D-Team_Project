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

public:
	void SetSpellType(uint32_t spelltype) { m_SpellType = spelltype; m_ResTag_DirtyFlag = true; }
	uint32_t GetSpellType() { return m_SpellType; }
	void SetResTagDirtyFlag(_bool flag) { m_ResTag_DirtyFlag = flag; }
	void StartCooldown();
private:
	virtual void PlayEffect(uint32_t uiState);

private:
	CComConstantBuffer* m_pComCBufferPerSpellMeter = nullptr;
	CButtonComponent* m_pComCButton = nullptr;

private:
	// *********     계산용 변수
	_float s_fAccumulatedTime = 0.f;
	_float m_fCurrentAmount = 1.f;

	uint32_t m_SpellType{};
	uint32_t m_colorType{};
	_float4 m_BGColor{1.f, 1.f, 1.f, 1.f};
	_float m_CoolTime{ 5.f };
	_float m_DistStrength = 0.05f;
private:
	void SetResTag();

private:
	_bool m_ResTag_DirtyFlag{ false };
public:
	static E::UPtr<CSpellMeter> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
