#include "pch.h"
#include "GraphicDevice.h"
#include "GameInstance.h"

#include "Resources.h"

NS_USING(Engine)

static uint32_t g_iMsaaCnt = 1;

CGraphicDevice::CGraphicDevice()
{

}
CGraphicDevice::~CGraphicDevice()
{
}

HRESULT CGraphicDevice::Initialize(ComPtr<ID3D11Device>& ppDevice, ComPtr<ID3D11DeviceContext>& ppContext)
{
	int32_t		iFlag = 0;

#ifdef _DEBUG
	iFlag = D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL			FeatureLV;

	if (FAILED(D3D11CreateDevice(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, iFlag, nullptr, 0, D3D11_SDK_VERSION,
		m_pDevice.GetAddressOf(), &FeatureLV,
		m_pDeviceContext.GetAddressOf())))
	{
		return E_FAIL;
	}

	ppDevice = m_pDevice;
	ppContext = m_pDeviceContext;

	return S_OK;
}

HRESULT CGraphicDevice::ReadyDevice(HWND hWnd, WINMODE eWinMode, uint32_t iWinSizeX, uint32_t iWinSizeY)
{

	if (FAILED(ReadySwapChain(hWnd, eWinMode, iWinSizeX, iWinSizeY)))
	{
		return E_FAIL;
	}

	if (FAILED(ReadyBackBufferRenderTargetView()))
	{
		return E_FAIL;
	}

	if (FAILED(ReadyDepthStencilView(iWinSizeX, iWinSizeY)))
	{
		return E_FAIL;
	}

	ID3D11RenderTargetView* pRTVs[] = {
		m_pBackBufferRTV.Get()
	};

	m_pDeviceContext->OMSetRenderTargets(1, pRTVs,
		m_pDepthStencilView.Get());

	D3D11_VIEWPORT ViewPortDesc;
	ZeroMemory(&ViewPortDesc, sizeof(D3D11_VIEWPORT));
	ViewPortDesc.TopLeftX = 0;
	ViewPortDesc.TopLeftY = 0;
	ViewPortDesc.Width = (_float)iWinSizeX;
	ViewPortDesc.Height = (_float)iWinSizeY;
	ViewPortDesc.MinDepth = 0.f;
	ViewPortDesc.MaxDepth = 1.f;

	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_VP, "VP_BackBuffer", CResViewPort::Create()))
	{
		if (FAILED(res->Load(ViewPortDesc)))
		{
			return E_FAIL;
		}
	}

	m_pDeviceContext->RSSetViewports(1, &ViewPortDesc);



	return S_OK;
}

HRESULT CGraphicDevice::ClearBackBufferView(const _float4* pClearColor)
{
	if (m_pDeviceContext == nullptr)
	{
		return E_FAIL;
	}

	m_pDeviceContext->ClearRenderTargetView(m_pBackBufferRTV.Get(), reinterpret_cast<const _float*>(pClearColor));



	return S_OK;
}

HRESULT CGraphicDevice::ClearDepthStencilView()
{
	if (m_pDeviceContext == nullptr)
	{
		return E_FAIL;
	}

	m_pDeviceContext->ClearDepthStencilView(
		m_pDepthStencilView.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	return S_OK;
}

HRESULT CGraphicDevice::Present()
{
	if (m_pSwapChain == nullptr)
	{
		return E_FAIL;
	}

	//return m_pSwapChain->Present(ETOUI(VSYNC::OFF), 0);
	return m_pSwapChain->Present(0, 0);
}

HRESULT CGraphicDevice::ReadySwapChain(HWND hWnd, WINMODE isWindowed, uint32_t iWinCX, uint32_t iWinCY)
{
	ComPtr<IDXGIDevice> pDevice{};
	m_pDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)pDevice.GetAddressOf());

	ComPtr<IDXGIAdapter> pAdapter{};
	pDevice->GetParent(__uuidof(IDXGIAdapter), (void**)pAdapter.GetAddressOf());

	ComPtr<IDXGIFactory> pFactory{};
	pAdapter->GetParent(__uuidof(IDXGIFactory), (void**)pFactory.GetAddressOf());

	DXGI_SWAP_CHAIN_DESC SwapChain;
	ZeroMemory(&SwapChain, sizeof(DXGI_SWAP_CHAIN_DESC));

	SwapChain.BufferDesc.Width = iWinCX;
	SwapChain.BufferDesc.Height = iWinCY;

	SwapChain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChain.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	SwapChain.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	SwapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// DXGI_SWAP_EFFECT_FLIP_DISCARD = 2
	// DXGI_SWAP_EFFECT_DISCARD = 1
	SwapChain.BufferCount = 2;
	//SwapChain.BufferCount = 1;



	SwapChain.BufferDesc.RefreshRate.Numerator = 60;
	SwapChain.BufferDesc.RefreshRate.Denominator = 1;

	SwapChain.SampleDesc.Quality = 0;
	SwapChain.SampleDesc.Count = g_iMsaaCnt;

	SwapChain.OutputWindow = hWnd;
	SwapChain.Windowed = static_cast<BOOL>(isWindowed);
	SwapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//SwapChain.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	if (FAILED(pFactory->CreateSwapChain(m_pDevice.Get(), &SwapChain, m_pSwapChain.GetAddressOf())))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CGraphicDevice::ReadyBackBufferRenderTargetView()
{
	if (m_pDevice == nullptr)
	{
		return E_FAIL;
	}

	if (FAILED(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)pBackBufferTexture.GetAddressOf())))
	{
		return E_FAIL;
	}

	if (FAILED(m_pDevice->CreateRenderTargetView(pBackBufferTexture.Get(), nullptr, m_pBackBufferRTV.GetAddressOf())))
	{
		return E_FAIL;
	}

	return S_OK;;
}

HRESULT CGraphicDevice::ReadyDepthStencilView(uint32_t iWinCX, uint32_t iWinCY)
{
	if (m_pDevice == nullptr)
	{
		return E_FAIL;
	}


	{
		auto resource = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_TEXTURE, "DYNTEX_DSV_BackBuffer", CResDynamicTexture2D::Create());

		if (FAILED(resource->Load(CResDynamicTexture2D::DESC{ .texDesc = {
			.Width = iWinCX,
			.Height = iWinCY,
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = DXGI_FORMAT_D24_UNORM_S8_UINT,

			.SampleDesc = {
				.Count = g_iMsaaCnt,
				.Quality = 0,
			},

			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_DEPTH_STENCIL,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
			} })))
		{
			return E_FAIL;
		};

		if (FAILED(resource->CreateDSV()))
		{
			return E_FAIL;
		};
		m_pDepthStencilView = resource->GetDSV();

	}

	//ComPtr<ID3D11Texture2D> pDepthStencilTexture{};
	//D3D11_TEXTURE2D_DESC TextureDesc{};
	//TextureDesc.Width = iWinCX;
	//TextureDesc.Height = iWinCY;
	//TextureDesc.MipLevels = 1;
	//TextureDesc.ArraySize = 1;
	//TextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//TextureDesc.SampleDesc.Quality = 0;
	//TextureDesc.SampleDesc.Count = g_iMsaaCnt;

	//TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	//TextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	//TextureDesc.CPUAccessFlags = 0;
	//TextureDesc.MiscFlags = 0;

	//if (FAILED(m_pDevice->CreateTexture2D(&TextureDesc, nullptr, pDepthStencilTexture.GetAddressOf())))
	//{
	//	return E_FAIL;
	//}

	//if (FAILED(m_pDevice->CreateDepthStencilView(pDepthStencilTexture.Get(), nullptr, m_pDepthStencilView.GetAddressOf())))
	//{
	//	return E_FAIL;
	//}

	return S_OK;
}

UPtr<CGraphicDevice> CGraphicDevice::Create(ComPtr< ID3D11Device>& ppDevice, ComPtr< ID3D11DeviceContext>& ppContext)
{
	auto pInstance = ToUPtr(new CGraphicDevice{});
	if (FAILED(pInstance->Initialize(ppDevice, ppContext)))
	{
		MSG_BOX("Failed to Created : CGraphicDevice");
		return nullptr;
	}
	return pInstance;
}
