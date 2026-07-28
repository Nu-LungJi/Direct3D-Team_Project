#pragma once

#include "UIObject.h"
#include "FlipbookUI.h"
#include "Client_Defines.h"
#include "FlipbookUI.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class MouseComponent;
NS_END

NS_BEGIN(Client)

class CEffectUI final : public E::CFlipbookUI
{
public:
	DECLARE_DERIVED_TYPE(CEffectUI, CFlipbookUI)

private:
	CEffectUI();
	~CEffectUI() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

protected:
	virtual void PlayEffect(uint32_t uiState);

private:
	CComConstantBuffer* m_pComCBufferPerUI = nullptr;
	CButtonComponent* m_pComCButton = nullptr;

public:
	static E::UPtr<CEffectUI> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
