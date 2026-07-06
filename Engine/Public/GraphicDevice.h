#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)

class CGraphicDevice final : public CEngineBase
{
private:
	CGraphicDevice();
	~CGraphicDevice();

public:
	ComPtr<ID3D11Device> GetDevice() const { return m_pDevice; }
	ComPtr<ID3D11DeviceContext> GetContext() const { return m_pDeviceContext; }
	ComPtr<ID3D11RenderTargetView> GetBackBufferRTV() const { return m_pBackBufferRTV; }
	ComPtr<ID3D11DepthStencilView> GetBackBufferDSV() const { return m_pDepthStencilView; }
	ComPtr<ID3D11Texture2D> GetBackBufferTexture() const { return pBackBufferTexture; }

public:
	HRESULT Initialize(ComPtr< ID3D11Device>& ppDevice, ComPtr< ID3D11DeviceContext>& ppContext);

	HRESULT ReadyDevice(HWND hWnd, WINMODE eWinMode, uint32_t iWinSizeX, uint32_t iWinSizeY);

	HRESULT ClearBackBufferView(const _float4* pClearColor);

	HRESULT ClearDepthStencilView();

	HRESULT Present();

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pDeviceContext{};
	ComPtr<IDXGISwapChain> m_pSwapChain{};
	ComPtr<ID3D11RenderTargetView> m_pBackBufferRTV{};
	ComPtr<ID3D11DepthStencilView> m_pDepthStencilView{};
	ComPtr<ID3D11Texture2D> pBackBufferTexture{};

private:
	HRESULT ReadySwapChain(HWND hWnd, WINMODE isWindowed, uint32_t iWinCX, uint32_t iWinCY);
	HRESULT ReadyBackBufferRenderTargetView();
	HRESULT ReadyDepthStencilView(uint32_t iWinCX, uint32_t iWinCY);

public:
	static UPtr<CGraphicDevice> Create(ComPtr< ID3D11Device>& ppDevice, ComPtr< ID3D11DeviceContext>& ppContext);
};

NS_END