#pragma once
#include "ResTexture.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResTexture2D final: public CResTexture
{
public:
	DECLARE_DERIVED_TYPE(CResTexture2D, CResTexture)

private:
	explicit CResTexture2D(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResTexture2D() override;

public:
	ComPtr<ID3D11ShaderResourceView> GetSRV() const { return m_pSRV; }
	ComPtr<ID3D11Texture2D> GetTexture() const { return m_pTexture; }
	const D3D11_TEXTURE2D_DESC& GetTexture2DDesc() const { return m_Texture2DDesc; }

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

private:
	ComPtr<ID3D11Texture2D> m_pTexture{};
	ComPtr<ID3D11ShaderResourceView> m_pSRV{};
	D3D11_TEXTURE2D_DESC m_Texture2DDesc{};

public:
	static SPtr<CResTexture2D> Create(const _string& sPath);

};

NS_END