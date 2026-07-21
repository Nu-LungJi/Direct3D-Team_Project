#pragma once

#include "UITex.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CButtonComponent;
NS_END

NS_BEGIN(Client)

class CMiniMap final : public E::CUITex
{
public:
	DECLARE_DERIVED_TYPE(CMiniMap, E::CUITex)

private:
	CMiniMap();
	~CMiniMap() override;

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
	CComConstantBuffer* m_pComCBufferPerUI = nullptr;
	CComConstantBuffer* m_pMinimapCBuffer = nullptr;
	CButtonComponent* m_pComCButton = nullptr;

	_float2 tMapOffset{};
	_float tRotation{ 0.f};
	_float tScale{1.f};

public:
	static E::UPtr<CMiniMap> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
