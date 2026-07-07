#pragma once

#include "UITex.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
NS_END

NS_BEGIN(Client)

class CUI_Item final : public E::CUITex
{
public:
	DECLARE_DERIVED_TYPE(CUI_Item, E::CUITex)

private:
	CUI_Item();
	~CUI_Item() override;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

protected:
	virtual void Creating();
	virtual void StartHovering();
	virtual void Hovering();
	virtual void EndHovering();
	virtual void Ending();

public:
	void SetMouseTracking(_bool isTracking) { m_bMouseTracking = isTracking; }
private:
	_bool m_bMouseTracking{};

private:
	bool m_bOutline{};

	CComConstantBuffer* m_pComCBufferPerUI = nullptr;

public:
	static E::UPtr<CUI_Item> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END