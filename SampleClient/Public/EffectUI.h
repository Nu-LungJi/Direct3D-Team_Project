#pragma once

#include "UIObject.h"
#include "FlipbookUI.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
NS_END


NS_BEGIN(Client)

class CEffectUI final : public E::CFlipbookUI
{
public:
	DECLARE_DERIVED_TYPE(CFlipBook, CUIObject)

private:
	CEffectUI();
	~CEffectUI() override;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

private:
	CComConstantBuffer* m_pComCBufferPerUI = nullptr;

private:
	uint32_t	m_iPuaseFrame = 9;
	_float		m_fPauseTime = 0.2f;
	_float		m_fPauseSumTime = 0.f;

	bool m_isPause = false;

public:
	static E::UPtr<CEffectUI> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
