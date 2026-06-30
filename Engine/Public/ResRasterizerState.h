#pragma once
#include "Resource.h"
NS_BEGIN(Engine)

class ENGINE_DLL CResRasterizerState final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResRasterizerState, CResource)

private:
	explicit CResRasterizerState(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResRasterizerState() override;

public:
	ComPtr<ID3D11RasterizerState> GetRasterizerState() const { return m_pRasterizerState; }

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {})  override;

private:
	ComPtr<ID3D11RasterizerState> m_pRasterizerState{};

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};
public:
	static SPtr<CResRasterizerState> Create();
};

NS_END