#pragma once
#include "ResTexture.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResDynamicTexture2D final : public CResTexture
{
public:
	DECLARE_DERIVED_TYPE(CResDynamicTexture2D, CResTexture)

public:
	typedef struct tagDesc {
		D3D11_TEXTURE2D_DESC texDesc{};
		D3D11_SUBRESOURCE_DATA texSubResource{};
	}DESC;

private:
	explicit CResDynamicTexture2D(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResDynamicTexture2D() override;

public:
	ComPtr<ID3D11Texture2D> GetTexture() const { return m_pTexture; }
	ComPtr<ID3D11ShaderResourceView> GetSRV(size_t idx = 0) const { return m_pSRV[idx]; }
	ComPtr<ID3D11UnorderedAccessView> GetUAV(size_t idx = 0) const { return m_pUAV[idx]; }
	ComPtr<ID3D11DepthStencilView> GetDSV(size_t idx = 0) const { return m_pDSV[idx]; }
	ComPtr<ID3D11RenderTargetView> GetRTV(size_t idx = 0) const { return m_pRTV[idx]; }
	
	HRESULT CreateSRV(const D3D11_SHADER_RESOURCE_VIEW_DESC& desc, size_t idx = 0);
	HRESULT CreateSRV(size_t idx = 0);
	HRESULT CreateUAV(const D3D11_UNORDERED_ACCESS_VIEW_DESC& desc, size_t idx = 0);
	HRESULT CreateUAV(size_t idx = 0);
	HRESULT CreateDSV(const D3D11_DEPTH_STENCIL_VIEW_DESC& desc, size_t idx = 0);
	HRESULT CreateDSV(size_t idx = 0);
	HRESULT CreateRTV(const D3D11_RENDER_TARGET_VIEW_DESC& desc, size_t idx = 0);
	HRESULT CreateRTV( size_t idx = 0);

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

private:
	ComPtr<ID3D11Texture2D> m_pTexture{};
	std::vector<ComPtr<ID3D11ShaderResourceView>> m_pSRV{};
	std::vector<ComPtr<ID3D11UnorderedAccessView>> m_pUAV{};
	std::vector<ComPtr<ID3D11DepthStencilView>> m_pDSV{};
	std::vector<ComPtr<ID3D11RenderTargetView>> m_pRTV{};

public:
	static SPtr<CResDynamicTexture2D> Create();

};

NS_END