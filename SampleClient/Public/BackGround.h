#pragma once
#include "UIObject.h"
#include "Client_Defines.h"
NS_BEGIN(Client)

class CBackGround final: public E::CUIObject
{
public:
	DECLARE_DERIVED_TYPE(CBackGround, CUIObject)

private:
	CBackGround();
	~CBackGround() override;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

public:
	static E::UPtr<CBackGround> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END