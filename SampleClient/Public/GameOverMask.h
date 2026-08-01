#pragma once

#include "UITex.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CButtonComponent;
NS_END

NS_BEGIN(Client)

class CGameOverMask final : public E::CUITex
{
public:
	DECLARE_DERIVED_TYPE(CGameOverMask, E::CUITex)

private:
	CGameOverMask();
	~CGameOverMask() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

private:
	CComConstantBuffer* m_pComCBufferPerUI = nullptr;

private:
	_float m_fAccTime = 0.f;
public:
	static E::UPtr<CGameOverMask> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
