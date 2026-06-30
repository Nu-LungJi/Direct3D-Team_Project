#pragma once
#include "ResShader.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResPixelShader final: public CResShader
{
public:
	DECLARE_DERIVED_TYPE(CResPixelShader, CResShader)

public:
	ComPtr<ID3D11PixelShader> GetPixelShader() const { return m_pPixelShader; }

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

private:
	explicit CResPixelShader(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResPixelShader() override;

private:
	ComPtr<ID3D11PixelShader> m_pPixelShader{};

public:
	static SPtr<CResPixelShader> Create(const _string& sPath);
};

NS_END