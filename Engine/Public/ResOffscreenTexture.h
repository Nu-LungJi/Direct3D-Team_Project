#pragma once
#include "ResTexture.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResOffscreenTexture final : public CResTexture
{
public:
	DECLARE_DERIVED_TYPE(CResOffscreenTexture, CResTexture)

public:
	struct DESC
	{
		uint32_t iWidth{};
		uint32_t iHeight{};
		DXGI_FORMAT eFormat{ DXGI_FORMAT_R8G8B8A8_UNORM };
	};

private:
	explicit CResOffscreenTexture(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResOffscreenTexture() override;

public:
	ComPtr<ID3D11ShaderResourceView> GetSRV() const { return m_pSRV; }
	ComPtr<ID3D11Texture2D> GetTexture() const { return m_pTexture; }
	ComPtr<ID3D11RenderTargetView> GetRTV() const { return m_pRTV; }
	ComPtr<ID3D11UnorderedAccessView> GetUAV() const { return m_pUAV; }

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

private:
	ComPtr<ID3D11Texture2D> m_pTexture{};
	ComPtr<ID3D11ShaderResourceView> m_pSRV{};
	ComPtr<ID3D11RenderTargetView> m_pRTV{};
	ComPtr<ID3D11UnorderedAccessView> m_pUAV{};

public:
	static SPtr<CResOffscreenTexture> Create();

};

NS_END