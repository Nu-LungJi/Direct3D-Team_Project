#pragma once

#include "UIObject.h"
#include "Client_Defines.h"
NS_BEGIN(Client)

class CTexUI final : public E::CUIObject
{
public:
	DECLARE_DERIVED_TYPE(CTexUI, CUIObject)

private:
	CTexUI();
	~CTexUI() override;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

public:
	static E::UPtr<CTexUI> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

};

NS_END