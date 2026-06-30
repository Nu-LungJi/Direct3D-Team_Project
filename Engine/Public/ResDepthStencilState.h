#pragma once
#include "Resource.h"
NS_BEGIN(Engine)

class ENGINE_DLL CResDepthStencilState final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResDepthStencilState, CResource)


private:
	explicit CResDepthStencilState(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResDepthStencilState() override;

public:
	ComPtr<ID3D11DepthStencilState> GetDepthStencilState() const { return m_pDepthStencilState; }
public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {})  override;

private:
	ComPtr<ID3D11DepthStencilState> m_pDepthStencilState{};

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};
public:
	static SPtr<CResDepthStencilState> Create();
};

NS_END