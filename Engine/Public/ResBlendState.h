#pragma once
#include "Resource.h"
NS_BEGIN(Engine)

class ENGINE_DLL CResBlendState final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResBlendState, CResource)

private:
	explicit CResBlendState(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResBlendState() override;

public:
	ComPtr<ID3D11BlendState> GetBlendState() const { return m_pBlendState; }
public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {})  override;

private:
	ComPtr<ID3D11BlendState> m_pBlendState{};

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};
public:
	static SPtr<CResBlendState> Create();
};

NS_END