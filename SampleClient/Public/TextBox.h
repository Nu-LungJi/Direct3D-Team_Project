#pragma once

#include "TextUI.h"
#include "Client_Defines.h"


NS_BEGIN(Engine)
class CComConstantBuffer;
NS_END

NS_BEGIN(Client)

class CTextBox final : public E::CTextUI
{
public:
	DECLARE_DERIVED_TYPE(CTextBox, E::CTextUI)

private:
	CTextBox();
	~CTextBox() override;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

private:
	CComConstantBuffer* m_pComCBufferPerUI = nullptr;
	CButtonComponent* m_pComCButton = nullptr;

private:
	_bool m_bMouseTracking{ false };

public:
	void PlayEffect(uint32_t uiState);

public:
	static E::UPtr<CTextBox> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
