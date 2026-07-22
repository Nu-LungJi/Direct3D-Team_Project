#include "pch.h"
#include "AnimationObject.h"
#include "Renderer.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"
#include "MyGFSDK_SSAO.h"
#include "UIObject.h"
#include "MyFSR2_2.h"

NS_USING(Engine)
CRenderer::CRenderer(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext) : m_pDevice{ pDevice }, m_pContext{ pContext } {}
CRenderer::~CRenderer() {}

void CRenderer::UpdateGUI()
{
	ImGui::Begin("Renderer");

	ImGui::End();

	PostProcessGUI();

	VolumetricFogGUI();
}
VOID	CRenderer::Update(_float fTimeDelta) {
	TimeAccumulation += fTimeDelta;
}
HRESULT CRenderer::Initialize()
{
	if (FAILED(InitializeShaderResource()))     return E_FAIL;

	if (FAILED(InitializeBackBuffer()))         return E_FAIL;

	if (FAILED(InitializeGFSDK_SSAO()))         return E_FAIL;

	if (FAILED(InitializeFSR2_2()))				return E_FAIL;

    if (FAILED(InitializeOffscreen()))          return E_FAIL;

	if (FAILED(InitializeShadow()))             return E_FAIL;

	if (FAILED(InitializeFullscreen()))         return E_FAIL;

	if (FAILED(InitializeBaseTarget()))         return E_FAIL;

	if (FAILED(InitializeTargetPBR()))          return E_FAIL;

	if (FAILED(InitializeBlendTarget()))        return E_FAIL;

	if (FAILED(InitializePostProcess()))         return E_FAIL;

	if (FAILED(InitializeBloom()))				return E_FAIL;

    if (FAILED(Initialize_Debugging()))         return E_FAIL;

	if (FAILED(InitializeVolumetricEffect()))	return E_FAIL;

	if (FAILED(InitializeUserInterface()))		return E_FAIL;

	if (FAILED(InitializeHizBuffer()))			return E_FAIL;

	return S_OK;
}

#pragma region INITIALIZE
HRESULT CRenderer::InitializeShaderResource()
{
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PostProcess_Bloom_BrightPass", "./ShaderFiles/PostProcess/PS_PostProcess_Bloom.hlsl"))
	{ 
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "PSMain_BrightPass", .sTarget = "ps_5_0" })))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PostProcess_Bloom_VerticalBlur", "./ShaderFiles/PostProcess/PS_PostProcess_Bloom.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "PSMain_GaussianBlur_Vertical", .sTarget = "ps_5_0" })))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PostProcess_Bloom_Combined", "./ShaderFiles/PostProcess/PS_PostProcess_Bloom.hlsl"))
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PostProcess_Filter", "./ShaderFiles/PostProcess/PS_PostProcess_Filter.hlsl"))
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}

	if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_FullScreenQuad", "./ShaderFiles/FullscreenQuad/FullscreenQuad.hlsl"))
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_FullScreenQuad", "./ShaderFiles/FullscreenQuad/FullscreenQuad.hlsl"))
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_PBR", "./ShaderFiles/PBR/VS_PBR.hlsl"))
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PBR", "./ShaderFiles/PBR/PS_PBR.hlsl"))
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_PBR_BLEND", "./ShaderFiles/PBR/VS_PBR.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "VSMain_Blend", .sTarget = "vs_5_0" })))  return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PBR_BLEND", "./ShaderFiles/PBR/PS_PBR.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "PSMain_Blend", .sTarget = "ps_5_0" })))  return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_Deferred", "./ShaderFiles/Deferred Rendering/VS_Deferred.hlsl"))
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_Deferred", "./ShaderFiles/Deferred Rendering/PS_Deferred.hlsl"))
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_Deferred", "./ShaderFiles/Deferred Rendering/PS_Deferred.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "PSMain_OverDraw", .sTarget = "ps_5_0" })))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_Volumetric", "./ShaderFiles/RayMarching/CS_Volumetric.hlsl"))
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTex", "./ShaderFiles/QuadTex/QuadTex.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "PSMain_NonAlpha", .sTarget = "ps_5_0" })))			return E_FAIL;
	}
	return S_OK;
}

HRESULT CRenderer::InitializeBackBuffer()
{
	D3D11_TEXTURE2D_DESC BackBufferDesc{};
	CGameInstance::Get().GetBackBufferTexture()->GetDesc(&BackBufferDesc);

	D3D11_TEXTURE2D_DESC CopyDesc = BackBufferDesc;
	CopyDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	CopyDesc.CPUAccessFlags = 0;
	CopyDesc.Usage = D3D11_USAGE_DEFAULT;

	if (FAILED(m_pDevice->CreateTexture2D(&CopyDesc, nullptr, m_pBackBufferTexture.GetAddressOf()))) {
		MSG_BOX("Cannot Create Texture2D");
		return E_FAIL;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
	SRVDesc.Format = CopyDesc.Format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.MipLevels = 1;

	if (FAILED(m_pDevice->CreateShaderResourceView(m_pBackBufferTexture.Get(), &SRVDesc, m_pBackBufferSRV.GetAddressOf()))) {
		MSG_BOX("Cannot Create SRV");
		return E_FAIL;
	}

	m_pBackBufferDSV = CGameInstance::Get().GetBackBufferDSV();
	m_pBackBufferRTV = CGameInstance::Get().GetBackBufferRTV();
	m_pBackBufferViewPort = CGameInstance::Get().GetResourceFirst<CResViewPort>(TAG_RES_GRP_PERMANENT_VP, "VP_BackBuffer");

	if (nullptr == m_pBackBufferDSV) { MSG_BOX("Invalid : m_pBackBufferDSV");        return E_FAIL; }
	if (nullptr == m_pBackBufferRTV) { MSG_BOX("Invalid : m_pBackBufferRTV");        return E_FAIL; }
	if (nullptr == m_pBackBufferViewPort) { MSG_BOX("Invalid : m_pBackBufferViewPort");   return E_FAIL; }
	if (nullptr == m_pBackBufferTexture) { MSG_BOX("Invalid : m_pBackBufferTexture");    return E_FAIL; }

	// Rasterizer Setting
	Rasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
	m_pContext->RSSetState(Rasterizer->GetRasterizerState().Get());

	return S_OK;
}

HRESULT CRenderer::InitializeOffscreen()
{
	if (m_pOffScreenVertexShader = CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_Deferred")) {
		if (FAILED(m_pOffScreenVertexShader->Load()))   return E_FAIL;
	}

	if (m_pOffScreenPixelShader = CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_Deferred")) {
		if (FAILED(m_pOffScreenPixelShader->Load()))    return E_FAIL;
	}

	if (m_pOffScreenTex2D = Generate_RenderTarget("DynTex2D_Offscreen", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)) {
		if (nullptr == m_pOffScreenTex2D)               return E_FAIL;
	}

	return S_OK;
}

HRESULT CRenderer::InitializeShadow()
{
	uint32_t ShadowMapResolutionX = { 1280 * 4 };
	uint32_t ShadowMapResolutionY = { 720 * 4 };

	m_pResDynTexTargetShadow = Generate_DepthStencil_RenderTarget("DynTex2D_Shadow", DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R24_UNORM_X8_TYPELESS, ShadowMapResolutionX, ShadowMapResolutionY);

	if (nullptr == m_pResDynTexTargetShadow)        return E_FAIL;

	m_pShadowViewPort = Generate_ViewPort("VP_ShadowMap", ShadowMapResolutionX, ShadowMapResolutionY);

	return S_OK;
}

HRESULT CRenderer::InitializeFullscreen()
{
	if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_FullscreenTex", E::CResQuadFullscreenTexBuffer::Create()))
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}

	if (m_pFullscreenVS = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_FullScreenQuad"))
	{
		if (nullptr == m_pFullscreenVS)        return E_FAIL;
	}

	if (m_pFullscreenPS = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_FullScreenQuad"))
	{
		if (nullptr == m_pFullscreenPS)        return E_FAIL;
	}

	if (m_pFullscreenVIBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResVIBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_FullscreenTex"))
	{
		if (nullptr == m_pFullscreenVIBuffer)        return E_FAIL;
	}

	return S_OK;
}

HRESULT CRenderer::InitializeBaseTarget() {
	m_pResDynTexTargetDiffuse = Generate_RenderTarget("DynTex2D_Target_Diffuse", DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	if (nullptr == m_pResDynTexTargetDiffuse)       return E_FAIL;

	m_pResDynTexTargetSMRO = Generate_RenderTarget("DynTex2D_Target_SMRO", DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	if (nullptr == m_pResDynTexTargetSMRO)          return E_FAIL;

	m_pResDynTexTargetEmissive = Generate_RenderTarget("DynTex2D_Target_Emissive", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	if (nullptr == m_pResDynTexTargetEmissive)      return E_FAIL;

	m_pResDynTexTargetNormal = Generate_RenderTarget("DynTex2D_Target_Normal", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	if (nullptr == m_pResDynTexTargetNormal)        return E_FAIL;

	m_pResDynTexTargetDepth = Generate_DepthStencil_RenderTarget("DynTex2D_Target_Depth", DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
	if (nullptr == m_pResDynTexTargetDepth)        return E_FAIL;

	m_pResDynTexTargetPreviousRenderView = Generate_RenderTarget("Previous_RenderView", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	if (nullptr == m_pResDynTexTargetPreviousRenderView)	return E_FAIL;
	return S_OK;
}

HRESULT CRenderer::InitializeTargetPBR()
{
	m_pResDynTexTargetPBR = Generate_RenderTarget("DynTex2D_Target_PBR", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	if (nullptr == m_pResDynTexTargetPBR)	return E_FAIL;

	if (m_pPBRVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_PBR"))
	{
		if (nullptr == m_pPBRVertexShader)	return E_FAIL;
	}
	if (m_pPBRPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PBR"))
	{
		if (nullptr == m_pPBRPixelShader)	return E_FAIL;
	}

	if (m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim"))
	{
		if (nullptr == m_pResVertexShader)	return E_FAIL;
	}
	if (m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim"))
	{
		if (nullptr == m_pResPixelShader)	return E_FAIL;
	}
	m_pResDynTexTargetLight = Generate_RenderTarget("DynTex2D_Target_Shadow", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	if (nullptr == m_pResDynTexTargetLight)	return E_FAIL;
	return S_OK;
}

HRESULT CRenderer::InitializeBlendTarget()
{
	if (m_pBlendVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_PBR_BLEND"))
	{
		if (nullptr == m_pBlendVertexShader)	return E_FAIL;
	}
	if (m_pBlendPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PBR_BLEND"))
	{
		if (nullptr == m_pBlendPixelShader)		return E_FAIL;
	}

	m_pResDynTexTargetEffect = Generate_RenderTarget("DynTex2D_Effect", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);

	return S_OK;
}

HRESULT CRenderer::InitializePostProcess() {

	m_pResDynTexTargetPostProcess = Generate_RenderTarget("DynTex2D_PostProcess", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	if (nullptr == m_pResDynTexTargetPostProcess)        return E_FAIL;

	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PostProcess", E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(POSTPROCESS) })))    return E_FAIL;
	}

	// LUT Texture Create
	if (FAILED(CreateWICTextureFromFile(m_pDevice.Get(), L"./Resources/Engine/Texture/PostProcess/LUT_Fuji.png", nullptr, m_pLUTTexture.GetAddressOf()))) {
		MSG_BOX("Cannot Create LUT Texture File.");
		return E_FAIL;
	}

	m_pPostProcessPS = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PostProcess_Filter");
	{
		if (nullptr == m_pPostProcessPS)		return E_FAIL;
	}

	return S_OK;
}

HRESULT CRenderer::InitializeUserInterface(){
	m_pResDynTexTargetUI = Generate_RenderTarget("DynTex2D_UserInterface", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	if (nullptr == m_pResDynTexTargetUI)        return E_FAIL;

	return S_OK;
}

HRESULT CRenderer::InitializeGFSDK_SSAO()
{
	m_pGFSDK_SSAO = CMyGFSDK_SSAO::Create();
	if (!m_pGFSDK_SSAO)
	{
		return E_FAIL;
	}
	m_pResDynTexTargetHBAO = Generate_RenderTarget("DynTex2D_HBAO_PLUS", DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	return S_OK;
}

HRESULT CRenderer::InitializeFSR2_2()
{
	m_pFSR2_2 = CMyFSR2_2::Create();
	if (!m_pFSR2_2)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT CRenderer::InitializeBloom() {

	m_pResDynTexTargetBrightPass = Generate_RenderTarget("DynTex2D_BrightPass", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	m_pResDynTexTargetBlurPass = Generate_RenderTarget("DynTex2D_BlurPass", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);

	m_pResDynTexTargetBloomPass = Generate_RenderTarget("DynTex2D_BloomPass", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);

	m_pBrightPassPixelShader = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PostProcess_Bloom_BrightPass");
	{
		if (nullptr == m_pBrightPassPixelShader)		return E_FAIL;
	}
	m_pVerticalBlurPixelShader = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PostProcess_Bloom_VerticalBlur");
	{
		if (nullptr == m_pVerticalBlurPixelShader)		return E_FAIL;
	}
	m_pBloomPassPixelShader = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PostProcess_Bloom_Combined");
	{
		if (nullptr == m_pBloomPassPixelShader)		return E_FAIL;
	}

	return S_OK;
}

HRESULT CRenderer::InitializeVolumetricEffect() {

	m_pResDynTexTargetVolumetric = Generate_RenderTarget("DynTex2D_Volumetric", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);

	m_pVolumetricComputeShader = E::CGameInstance::Get().GetResourceFirst<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_Volumetric");

	m_pResDynTexUAVVolumetric = Generate_UnorderedAccessView("UAV_Volumetric", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE);

	if (FAILED(CreateWICTextureFromFile(m_pDevice.Get(), L"./Resources/Engine/Texture/DefaultTexture/BlueNoiseTexture.png", nullptr, BlueNoiseTexture.GetAddressOf()))) {
		MSG_BOX("Cannot Create BlueNoise Texture File.");
		return E_FAIL;
	}

	VolumeTexture = Create_Texture3D(DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_SHADER_RESOURCE, 32, 32, 32);
	//if (FAILED(CreateDDSTextureFromFile(m_pDevice.Get(), L"./Resources/Engine/Texture/DefaultTexture/VolumeTexture.dds", nullptr, VolumeTexture.GetAddressOf()))) {
	//	MSG_BOX("Cannot Create BlueNoise Texture File.");
	//	return E_FAIL;
	//}
	return S_OK;
}
#pragma endregion

#pragma region  EXTRAFUNCTION

SPtr<CResDynamicTexture2D> CRenderer::Generate_RenderTarget(const StringID& _sResTag, DXGI_FORMAT _Format, uint32_t _BindFlags, uint32_t _TexWidth, uint32_t _TexHeight) {
	auto vClientScreenSize = CGameInstance::Get().GetClientScreenSize();

	if (_TexWidth == 0)     _TexWidth = vClientScreenSize.x;
	if (_TexHeight == 0)    _TexHeight = vClientScreenSize.y;

	if (auto Resource = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_TEXTURE, _sResTag, E::CResDynamicTexture2D::Create()))
	{
		CResDynamicTexture2D::DESC Desc{};
		Desc.texDesc = {
			.Width = _TexWidth,
			.Height = _TexHeight,
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = _Format,
			.SampleDesc = {.Count = 1, .Quality = 0 },
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = _BindFlags,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		if (FAILED(Resource->Load(Desc)))    return nullptr;

		if (FAILED(Resource->CreateSRV()))   return nullptr;

		if (FAILED(Resource->CreateRTV()))   return nullptr;

		return Resource;
	}

	return nullptr;
}

SPtr<CResDynamicTexture2D> CRenderer::Generate_DepthStencil_RenderTarget(const StringID& _sResTag, DXGI_FORMAT _TexFormat, DXGI_FORMAT _DSVFormat, DXGI_FORMAT _SRVFormat, uint32_t _TexWidth, uint32_t _TexHeight) {
	auto vClientScreenSize = CGameInstance::Get().GetClientScreenSize();

	if (_TexWidth == 0)     _TexWidth = vClientScreenSize.x;
	if (_TexHeight == 0)    _TexHeight = vClientScreenSize.y;

	if (auto Resource = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_TEXTURE, _sResTag, E::CResDynamicTexture2D::Create()))
	{
		CResDynamicTexture2D::DESC Desc{};
		Desc.texDesc = {
			.Width = _TexWidth,
			.Height = _TexHeight,
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = _TexFormat,
			.SampleDesc = {.Count = 1, .Quality = 0 },
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};

		CResDynamicTexture2D::DESC DynTex2DDesc{};
		DynTex2DDesc.texDesc = Desc.texDesc;
		if (FAILED(Resource->Load(DynTex2DDesc))) return nullptr;

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format = _DSVFormat;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
		if (FAILED(Resource->CreateDSV(dsvDesc))) return nullptr;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = _SRVFormat;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		if (FAILED(Resource->CreateSRV(srvDesc))) return nullptr;

		return Resource;
	}

	return nullptr;
}

SPtr<CResDynamicTexture2D> CRenderer::Generate_UnorderedAccessView(const StringID& _sResTag, DXGI_FORMAT _TexFormat, uint32_t _BindFlags, uint32_t _TexWidth, uint32_t _TexHeight) {
	auto vClientScreenSize = CGameInstance::Get().GetClientScreenSize();

	if (_TexWidth == 0)     _TexWidth = vClientScreenSize.x;
	if (_TexHeight == 0)    _TexHeight = vClientScreenSize.y;

	if (auto Resource = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_TEXTURE, _sResTag, E::CResDynamicTexture2D::Create()))
	{
		CResDynamicTexture2D::DESC Desc{};
		Desc.texDesc = {
			.Width = _TexWidth,
			.Height = _TexHeight,
			.MipLevels = 1,
			.ArraySize = 1,
			.Format = _TexFormat,
			.SampleDesc = {.Count = 1, .Quality = 0 },
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = _BindFlags,
			.CPUAccessFlags = 0,
			.MiscFlags = 0
		};
		if (FAILED(Resource->Load(Desc)))    return nullptr;

		D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.Format = _TexFormat;
		UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		UAVDesc.Texture2D.MipSlice = 0;
		if (FAILED(Resource->CreateUAV(UAVDesc)))	 return nullptr;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = _TexFormat;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		if (FAILED(Resource->CreateSRV(srvDesc))) return nullptr;
		return Resource;
	}

	return nullptr;

}
HRESULT CRenderer::Generate_ShadowMapOutput(ID3D11UnorderedAccessView** _ShadowUAV, ID3D11Texture2D** _Texture, ID3D11ShaderResourceView** _ShadowSRV, uint32_t _LTYPE, uint32_t _ResolutionX, uint32_t _ResolutionY) {

	if (nullptr == _ShadowUAV || nullptr == _Texture || nullptr == _ShadowSRV) return E_FAIL;

	D3D11_TEXTURE2D_DESC Tex2dDesc = {};
	Tex2dDesc.Width = _ResolutionX;
	Tex2dDesc.Height = _ResolutionY;
	Tex2dDesc.MipLevels = 1;
	Tex2dDesc.ArraySize = (_LTYPE == ETOUI(LIGHT_TYPE::POINT)) ? MAX_LIGHT_MAPCOUNT : 1;
	Tex2dDesc.Format = DXGI_FORMAT_R32_FLOAT;
	Tex2dDesc.Usage = D3D11_USAGE_DEFAULT;
	Tex2dDesc.SampleDesc = { .Count = 1, .Quality = 0 };
	Tex2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	Tex2dDesc.MiscFlags = (_LTYPE == ETOUI(LIGHT_TYPE::POINT)) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;

	if (FAILED(m_pDevice->CreateTexture2D(&Tex2dDesc, nullptr, _Texture)))			return E_FAIL;

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
	SRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	if (_LTYPE == ETOUI(LIGHT_TYPE::POINT)) {
		SRVDesc.TextureCube.MipLevels = 1;
		SRVDesc.TextureCube.MostDetailedMip = 0;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	}
	else {
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	}
	if (FAILED(m_pDevice->CreateShaderResourceView(*_Texture, &SRVDesc, _ShadowSRV)))	return E_FAIL;

	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	UAVDesc.ViewDimension = (_LTYPE == ETOUI(LIGHT_TYPE::POINT)) ? D3D11_UAV_DIMENSION_TEXTURE2DARRAY : D3D11_UAV_DIMENSION_TEXTURE2D;
	UAVDesc.Texture2DArray.FirstArraySlice = 0;
	UAVDesc.Texture2DArray.ArraySize = (_LTYPE == ETOUI(LIGHT_TYPE::POINT)) ? 6 : 1;
	UAVDesc.Texture2DArray.MipSlice = 0;

	if (FAILED(m_pDevice->CreateUnorderedAccessView(*_Texture, &UAVDesc, _ShadowUAV)))	 return E_FAIL;

	return S_OK;
}
SPtr<CResViewPort>         CRenderer::Generate_ViewPort(const StringID& _sResTag, uint32_t _TexWidth, uint32_t _TexHeight) {
	if (auto Resource = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_VP, _sResTag, E::CResViewPort::Create()))
	{
		D3D11_VIEWPORT ViewDesc{};
		ViewDesc.TopLeftX = 0.f;
		ViewDesc.TopLeftY = 0.f;
		ViewDesc.Width = static_cast<float>(_TexWidth);
		ViewDesc.Height = static_cast<float>(_TexHeight);
		ViewDesc.MinDepth = 0.f;
		ViewDesc.MaxDepth = 1.f;
		if (FAILED(Resource->Load(ViewDesc)))    return nullptr;

		return Resource;
	}
	return nullptr;
}
HRESULT CRenderer::Generate_Texture2DArray(std::vector<ComPtr<ID3D11DepthStencilView>>* _ShadowDSVList, ID3D11Texture2D** _TextureArray, ID3D11ShaderResourceView** _SRV, uint32_t _Resolution, uint32_t _MaxLightCount) {
	/////////////////////////////////////
	if (_TextureArray && *_TextureArray) {
		(*_TextureArray)->Release();
		*_TextureArray = nullptr;
	}
	if (_SRV && *_SRV) {
		(*_SRV)->Release();
		*_SRV = nullptr;
	}
	if (_ShadowDSVList) {
		for (auto& DSV : *_ShadowDSVList) {
			if (DSV) {
				DSV->Release();
				DSV = nullptr;
			}
		}
		_ShadowDSVList->clear();
	}
	////////////////////////////////////

	D3D11_TEXTURE2D_DESC TEXDesc{};
	TEXDesc.Width = _Resolution;
	TEXDesc.Height = _Resolution;
	TEXDesc.MipLevels = 1;
	TEXDesc.ArraySize = _MaxLightCount;
	TEXDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	TEXDesc.SampleDesc = { .Count = 1, .Quality = 0 };
	TEXDesc.Usage = D3D11_USAGE_DEFAULT;
	TEXDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	TEXDesc.CPUAccessFlags = 0;
	TEXDesc.MiscFlags = 0;

	if (FAILED(m_pDevice->CreateTexture2D(&TEXDesc, nullptr, _TextureArray))) return E_FAIL;

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	SRVDesc.Texture2DArray.MostDetailedMip = 0;
	SRVDesc.Texture2DArray.MipLevels = 1;
	SRVDesc.Texture2DArray.FirstArraySlice = 0;
	SRVDesc.Texture2DArray.ArraySize = _MaxLightCount;

	if (FAILED(m_pDevice->CreateShaderResourceView(*_TextureArray, &SRVDesc, _SRV))) return E_FAIL;

	_ShadowDSVList->resize(_MaxLightCount);
	for (UINT i = 0; i < _MaxLightCount; ++i)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
		DSVDesc.Format = DXGI_FORMAT_D32_FLOAT;
		DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		DSVDesc.Texture2DArray.MipSlice = 0;
		DSVDesc.Texture2DArray.FirstArraySlice = i;
		DSVDesc.Texture2DArray.ArraySize = 1;

		if (FAILED(m_pDevice->CreateDepthStencilView(*_TextureArray, &DSVDesc, &(*_ShadowDSVList)[i]))) return E_FAIL;
	}

	return S_OK;
}

HRESULT CRenderer::Generate_CubeMap(ID3D11DepthStencilView** _ShadowDSV, ID3D11Texture2D** _TextureArray, ID3D11ShaderResourceView** _SRV, uint32_t _Resolution, uint32_t _MaxLightCount) {
	D3D11_TEXTURE2D_DESC TEXDesc{};
	TEXDesc.Width = _Resolution;
	TEXDesc.Height = _Resolution;
	TEXDesc.MipLevels = 1;
	TEXDesc.ArraySize = _MaxLightCount * 6;
	TEXDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	TEXDesc.SampleDesc = { .Count = 1, .Quality = 0 };
	TEXDesc.Usage = D3D11_USAGE_DEFAULT;
	TEXDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	TEXDesc.CPUAccessFlags = 0;
	TEXDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	if (FAILED(m_pDevice->CreateTexture2D(&TEXDesc, nullptr, _TextureArray))) return E_FAIL;

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
	SRVDesc.Texture2DArray.MostDetailedMip = 0;
	SRVDesc.Texture2DArray.MipLevels = 1;
	SRVDesc.Texture2DArray.FirstArraySlice = 0;
	SRVDesc.Texture2DArray.ArraySize = _MaxLightCount;

	if (FAILED(m_pDevice->CreateShaderResourceView(*_TextureArray, &SRVDesc, _SRV))) return E_FAIL;

	D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
	DSVDesc.Format = DXGI_FORMAT_D32_FLOAT;
	DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
	DSVDesc.Texture2DArray.MipSlice = 0;
	DSVDesc.Texture2DArray.FirstArraySlice = 0;
	DSVDesc.Texture2DArray.ArraySize = _MaxLightCount * 6;

	if (FAILED(m_pDevice->CreateDepthStencilView(*_TextureArray, &DSVDesc, _ShadowDSV))) return E_FAIL;

	return S_OK;
}

HRESULT CRenderer::Generate_CubeMapList(std::vector<ComPtr<ID3D11DepthStencilView>>* _ShadowDSVList, uint32_t _Resolution, uint32_t _MaxLightCount) {
	D3D11_TEXTURE2D_DESC TEXDesc{};
	TEXDesc.Width = _Resolution;
	TEXDesc.Height = _Resolution;
	TEXDesc.MipLevels = 1;
	TEXDesc.ArraySize = _MaxLightCount * 6;
	TEXDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	TEXDesc.SampleDesc = { .Count = 1, .Quality = 0 };
	TEXDesc.Usage = D3D11_USAGE_DEFAULT;
	TEXDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	TEXDesc.CPUAccessFlags = 0;
	TEXDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	ComPtr<ID3D11Texture2D> TextureArray = { nullptr };
	if (FAILED(m_pDevice->CreateTexture2D(&TEXDesc, nullptr, TextureArray.GetAddressOf()))) return E_FAIL;

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
	SRVDesc.Texture2DArray.MostDetailedMip = 0;
	SRVDesc.Texture2DArray.MipLevels = 1;
	SRVDesc.Texture2DArray.FirstArraySlice = 0;
	SRVDesc.Texture2DArray.ArraySize = _MaxLightCount;

	ComPtr<ID3D11ShaderResourceView> SRV = { nullptr };
	if (FAILED(m_pDevice->CreateShaderResourceView(TextureArray.Get(), &SRVDesc, SRV.GetAddressOf()))) return E_FAIL;

	return S_OK;
}

HRESULT CRenderer::Generate_ShadowTexture(ID3D11DepthStencilView** _ShadowDSV, ID3D11Texture2D** _Texture, ID3D11ShaderResourceView** _SRV, uint32_t _ResolutionX, uint32_t _ResolutionY){
	D3D11_TEXTURE2D_DESC TEXDesc{};
	TEXDesc.Width = _ResolutionX;
	TEXDesc.Height = _ResolutionY;
	TEXDesc.MipLevels = 1;
	TEXDesc.ArraySize = 1;
	TEXDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	TEXDesc.SampleDesc = { .Count = 1, .Quality = 0 };
	TEXDesc.Usage = D3D11_USAGE_DEFAULT;
	TEXDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	TEXDesc.CPUAccessFlags = 0;
	TEXDesc.MiscFlags = 0;

	if (FAILED(m_pDevice->CreateTexture2D(&TEXDesc, nullptr, _Texture))) return E_FAIL;

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2DArray.MostDetailedMip = 0;
	SRVDesc.Texture2DArray.MipLevels = 1;

	if (FAILED(m_pDevice->CreateShaderResourceView(*_Texture, &SRVDesc, _SRV))) return E_FAIL;

	D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
	DSVDesc.Format = DXGI_FORMAT_D32_FLOAT;
	DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	DSVDesc.Texture2DArray.MipSlice = 0;

	if (FAILED(m_pDevice->CreateDepthStencilView(*_Texture, &DSVDesc, _ShadowDSV))) return E_FAIL;

	return S_OK;
}

ComPtr<ID3D11ShaderResourceView> CRenderer::Create_Texture2D(DXGI_FORMAT _TexFormat, uint32_t _BindFlags, uint32_t _TexWidth, uint32_t _TexHeight) {

	if (_TexWidth == 0)     _TexWidth = 1;
	if (_TexHeight == 0)    _TexHeight = 1;

	D3D11_TEXTURE2D_DESC Tex2dDesc = {};
	Tex2dDesc.Width = _TexWidth;
	Tex2dDesc.Height = _TexHeight;
	Tex2dDesc.MipLevels = 1;
	Tex2dDesc.ArraySize = 1;
	Tex2dDesc.Format = _TexFormat;
	Tex2dDesc.SampleDesc.Count = 1;
	Tex2dDesc.Usage = D3D11_USAGE_IMMUTABLE;
	Tex2dDesc.BindFlags = _BindFlags;

	ComPtr<ID3D11ShaderResourceView> SRV = { nullptr };
	ComPtr<ID3D11Texture2D> Tex2D = { nullptr };

	if (FAILED(m_pDevice->CreateTexture2D(&Tex2dDesc, nullptr, Tex2D.GetAddressOf())))			return nullptr;

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
	SRVDesc.Format = _TexFormat;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	if (FAILED(m_pDevice->CreateShaderResourceView(Tex2D.Get(), &SRVDesc, SRV.GetAddressOf())))	return nullptr;

	return SRV;
}

ComPtr<ID3D11ShaderResourceView> CRenderer::Create_Texture3D(DXGI_FORMAT _TexFormat, uint32_t _BindFlags, uint32_t _TexWidth, uint32_t _TexHeight, uint32_t _TexDepth) {

	if (_TexWidth == 0)     _TexWidth = 1;
	if (_TexHeight == 0)    _TexHeight = 1;
	if (_TexDepth == 0)		_TexDepth = 1;

	std::vector<uint8_t> pPixelData(_TexWidth * _TexHeight * _TexDepth * 4);

	for (uint32_t z = 0; z < _TexDepth; ++z)
	{
		for (uint32_t y = 0; y < _TexHeight; ++y)
		{
			for (uint32_t x = 0; x < _TexWidth; ++x)
			{
				uint32_t iIdx = (z * _TexWidth * _TexHeight + y * _TexWidth + x) * 4;

				float fNoiseR = NoiseHash(x, y, z);
				float fNoiseG = NoiseHash(x * 2, y * 2, z * 2);
				float fNoiseB = NoiseHash(x * 4, y * 4, z * 4);
				float fNoiseA = NoiseHash(x * 8, y * 8, z * 8);

				pPixelData[iIdx + 0] = static_cast<uint8_t>(saturate(fNoiseR * 0.5f + 0.5f) * 255.0f);
				pPixelData[iIdx + 1] = static_cast<uint8_t>(saturate(fNoiseG * 0.5f + 0.5f) * 255.0f);
				pPixelData[iIdx + 2] = static_cast<uint8_t>(saturate(fNoiseB * 0.5f + 0.5f) * 255.0f);
				pPixelData[iIdx + 3] = static_cast<uint8_t>(saturate(fNoiseA * 0.5f + 0.5f) * 255.0f);
			}
		}
	}

	D3D11_TEXTURE3D_DESC Tex3dDesc = {};
	Tex3dDesc.Width = _TexWidth;
	Tex3dDesc.Height = _TexHeight;
	Tex3dDesc.Depth = _TexDepth;
	Tex3dDesc.MipLevels = 1;
	Tex3dDesc.Format = _TexFormat;
	Tex3dDesc.Usage = D3D11_USAGE_DEFAULT;
	Tex3dDesc.BindFlags = _BindFlags;

	ComPtr<ID3D11ShaderResourceView>	SRV = { nullptr };
	ComPtr<ID3D11Texture3D>				Tex3D = { nullptr };

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = pPixelData.data();
	InitData.SysMemPitch = _TexWidth * 4;
	InitData.SysMemSlicePitch = _TexWidth * _TexHeight * 4;

	if (FAILED(m_pDevice->CreateTexture3D(&Tex3dDesc, &InitData, Tex3D.GetAddressOf())))				return nullptr;

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
	SRVDesc.Format = _TexFormat;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	SRVDesc.Texture3D.MipLevels = 1;
	SRVDesc.Texture3D.MostDetailedMip = 0;
	if (FAILED(m_pDevice->CreateShaderResourceView(Tex3D.Get(), &SRVDesc, SRV.GetAddressOf())))			return nullptr;

	return SRV;
}

HRESULT CRenderer::AddRenderObject(RENDERGROUP eRenderGroup, IRenderable* pRenderObject)
{
	if (eRenderGroup >= RENDERGROUP::END ||
		nullptr == pRenderObject)
		return E_FAIL;

	m_RenderObject[ETOUI(eRenderGroup)].push_back(pRenderObject);
	return S_OK;
}

HRESULT CRenderer::Reset_DefaultShader(RENDERGROUP _Group) {

	if (RENDERGROUP::NONBLEND == _Group) {
		m_pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
		m_pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
		m_pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);
	}
	else if (RENDERGROUP::BLEND == _Group) {
		m_pContext->IASetInputLayout(m_pBlendVertexShader->GetInputLayout().Get());
		m_pContext->VSSetShader(m_pBlendVertexShader->GetVertexShader().Get(), nullptr, 0);
		m_pContext->PSSetShader(m_pBlendPixelShader->GetPixelShader().Get(), nullptr, 0);
	}

	return S_OK;
}

VOID	CRenderer::Unbind_Resources()
{
	// UnBind RenderTargets / ShaderResource / Shader
	ID3D11RenderTargetView* pRTVs[8] = { nullptr };
	m_pContext->OMSetRenderTargets(8, pRTVs, nullptr);

	ID3D11ShaderResourceView* pNullSRVs[12] = { nullptr };
	m_pContext->PSSetShaderResources(0, 12, pNullSRVs);
	m_pContext->VSSetShaderResources(0, 12, pNullSRVs);
	m_pContext->CSSetShaderResources(0, 12, pNullSRVs);

	m_pContext->IASetInputLayout(nullptr);
	m_pContext->PSSetShader(nullptr, nullptr, 0);
	m_pContext->CSSetShader(nullptr, nullptr, 0);
}

VOID CRenderer::Render_Quad(){
	const auto& vs = m_pFullscreenVS;
	const auto& ps = m_pFullscreenPS;
	const auto& viBuffer = m_pFullscreenVIBuffer;

	m_pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	m_pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	m_pContext->IASetInputLayout(vs->GetInputLayout().Get());

	ComPtr<ID3D11ShaderResourceView> pSRVs = { m_pResDynTexTargetPreviousRenderView->GetSRV().Get() };
	m_pContext->PSSetShaderResources(0, 1, pSRVs.GetAddressOf());

	ID3D11Buffer* vertexBuffers[] = {
			viBuffer->GetVertexBuffer().Get()
	};
	uint32_t strides[] = {
		viBuffer->GetVertexStride()
	};
	uint32_t offsets[] = {
		0
	};
	m_pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	m_pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
	m_pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

	m_pContext->DrawIndexed(viBuffer->GetIndexStride(), 0, 0);
}

HRESULT CRenderer::Bind_CameraAttribute(CCameraObject* _ActiveCam) {
	auto pCbPerPass = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PASS);
	D3D11_MAPPED_SUBRESOURCE mappedSubResource;
	if (RenderContext.pass == RENDERPASS::SHADOW) {

		ShadowLightVP = _ActiveCam->GetView() * _ActiveCam->GetProj();
	}
	if (SUCCEEDED(m_pContext->Map(pCbPerPass->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
	{
		CB_PER_PASS cbPerPass{};
		XMStoreFloat4x4(&cbPerPass.matView, _ActiveCam->GetView());
		XMStoreFloat4x4(&cbPerPass.matProj, _ActiveCam->GetProj());

		XMStoreFloat4x4(&cbPerPass.matViewProj, _ActiveCam->GetView() * _ActiveCam->GetProj());

		XMStoreFloat4x4(&cbPerPass.matInvView, XMMatrixInverse(nullptr, _ActiveCam->GetView()));
		XMStoreFloat4x4(&cbPerPass.matInvProj, XMMatrixInverse(nullptr, _ActiveCam->GetProj()));

		XMStoreFloat4x4(&cbPerPass.matInvViewProj, XMMatrixMultiply(XMLoadFloat4x4(&cbPerPass.matInvProj), XMLoadFloat4x4(&cbPerPass.matInvView)));

		cbPerPass.vCamPos = _ActiveCam->GetTransform().GetPosition();

		XMStoreFloat4x4(&cbPerPass.matShadowLightViewProj, ShadowLightVP);

		memcpy(mappedSubResource.pData, &cbPerPass, sizeof(cbPerPass));
		m_pContext->Unmap(pCbPerPass->GetCBuffer().Get(), 0);
	}

	m_pContext->VSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
	m_pContext->PSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
	m_pContext->CSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());

	return S_OK;
}

HRESULT CRenderer::Bind_VolumetricFog() {
	auto pCbPerPass = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_FOG");
	D3D11_MAPPED_SUBRESOURCE mappedSubResource;
	if (SUCCEEDED(m_pContext->Map(pCbPerPass->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
	{
		CB_FOG cbFog{};
		cbFog.FogIntensity = m_fFogIntensity;
		cbFog.FogColor = m_fFogColor;
		cbFog.FogMaxHeight = m_fFogMaxHeight;
		cbFog.FogStartPos = m_fFogStartPos;
		cbFog.FogEndPos = m_fFogEndPos;
		cbFog.FogDensity = m_fFogDensity;

		memcpy(mappedSubResource.pData, &cbFog, sizeof(cbFog));
		m_pContext->Unmap(pCbPerPass->GetCBuffer().Get(), 0);
	}
	m_pContext->VSSetConstantBuffers(6, 1, pCbPerPass->GetCBuffer().GetAddressOf());
	m_pContext->PSSetConstantBuffers(6, 1, pCbPerPass->GetCBuffer().GetAddressOf());
	m_pContext->CSSetConstantBuffers(6, 1, pCbPerPass->GetCBuffer().GetAddressOf());

	return S_OK;
}

HRESULT CRenderer::Reset_RenderContext(RENDERPASS _Pass, CCameraObject* _ActiveCam) {
	if (_ActiveCam == nullptr) return E_FAIL;

	RenderContext.pass = _Pass;
	RenderContext.matProj = _ActiveCam->GetProj();
	RenderContext.matView = _ActiveCam->GetView();
	RenderContext.matViewProj = RenderContext.matView * RenderContext.matProj;
	RenderContext.eye = _ActiveCam->GetTransform().GetLoadedPostion();

	return S_OK;
}
_float CRenderer::NoiseHash(uint32_t _X, uint32_t _Y, uint32_t _Z)
{
	int n = _X + _Y * 57 + _Z * 241;
	n = (n << 13) ^ n;
	return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}
#pragma endregion

#pragma region  RENDERING
HRESULT CRenderer::Draw() {
	ZoneScopedN("Draw");

	m_pContext->RSSetState(Rasterizer->GetRasterizerState().Get());

	//if (bApplyShadow)
	//	if (FAILED(Render_Shadow()))	return E_FAIL;

	// DepthMap
	if (FAILED(Render_DepthMap()))       return E_FAIL;

	// Diffuse + Normal + SMRO + Emissive
	if (FAILED(Render_NonAlpha()))       return E_FAIL;

	// Hi-Z build: opaque depth 기반
	if (FAILED(BuildCurrentHizBuffer())) return E_FAIL;

	// HBAO
	if (FAILED(Render_HBAO()))			 return E_FAIL;

	// PBR Lighting
	if (FAILED(Render_Lighting()))       return E_FAIL;

	// Trensparent + PBR
	if (FAILED(Render_Alpha()))          return E_FAIL;

	// Effect
	if (FAILED(Render_Effect()))			return E_FAIL;

	// Volumetric
	//if (FAILED(Render_VolumetricEffect())) return E_FAIL;

	// Combined
	if (FAILED(Render_OffScreen()))      return E_FAIL;

	// PostProcess
	if (FAILED(Render_PostProcess()))     return E_FAIL;

	{
		m_pResDynTexTargetPreviousRenderView = ApplyFilter ? m_pResDynTexTargetPostProcess : m_pOffScreenTex2D;
	}

	// UI
	if (FAILED(Render_UserInterface()))     return E_FAIL;

	// FullScreen : Final
	if (FAILED(Render_FullScreen()))        return E_FAIL;

	// Debugging
	if (FAILED(Render_Debugging()))      return E_FAIL;

	return S_OK;
}

void CRenderer::FrameEnd()
{
	// HizBuffer 교체
	std::swap(m_pCurrentHizBuffer, m_pPrevHizBuffer);
	m_bHasPrevHizBuffer = true;

    for (auto& vecRenderables : m_RenderObject)
    {
        vecRenderables.clear();
    }
}

HRESULT CRenderer::Render_Shadow() {
	{
		ID3D11ShaderResourceView* pNullSRV[1] = { nullptr };
		m_pContext->PSSetShaderResources(6, 1, pNullSRV); // 6번 슬롯을 NULL로 청소
	}
	{
		ID3D11DepthStencilState* pDSS = nullptr;
		m_pContext->OMSetDepthStencilState(pDSS, 0);

		SPtr<CResDepthStencilState> DepthWriteState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHWRITE");
		m_pContext->OMSetDepthStencilState(DepthWriteState->GetDepthStencilState().Get(), 0);

		m_pContext->ClearDepthStencilView(m_pResDynTexTargetShadow->GetDSV().Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
	}

	// RenderTarget/DepthStencil Setting + ViewPort Setting
	{
		ID3D11RenderTargetView* pRTVs[1] = { nullptr };
		m_pContext->OMSetRenderTargets(1, pRTVs, m_pResDynTexTargetShadow->GetDSV().Get());
		m_pContext->RSSetViewports(1, &m_pShadowViewPort->GetViewPort());

		m_pContext->IASetInputLayout(m_pDebugVertexShader->GetInputLayout().Get());
		m_pContext->VSSetShader(m_pDebugVertexShader->GetVertexShader().Get(), nullptr, 0);
		m_pContext->PSSetShader(nullptr, nullptr, 0);

		ID3D11Buffer* vertexBuffers[] = { m_pDebugBuffer->GetVertexBuffer().Get() };
		uint32_t strides[] = { m_pDebugBuffer->GetVertexStride() };
		uint32_t offsets[] = { 0 };

		m_pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
		m_pContext->IASetIndexBuffer(m_pDebugBuffer->GetIndexBuffer().Get(), m_pDebugBuffer->GetIndexFormat(), 0);
		m_pContext->IASetPrimitiveTopology(m_pDebugBuffer->GetPrimitiveType());
	}
	{
		auto pShadowCamera = CGameInstance::Get().GetCamera("Shadow");
		if (nullptr == pShadowCamera) return S_OK;

		if (FAILED(Reset_RenderContext(RENDERPASS::SHADOW, pShadowCamera))) return E_FAIL;

		if (FAILED(Bind_CameraAttribute(pShadowCamera)))					return E_FAIL;

		if (FAILED(RenderNonBlend()))										return E_FAIL;
	}
	// UnBind RenderTargets / ShaderResource / Shader
	{
		ID3D11RenderTargetView* pRTVs[1] = { nullptr };
		m_pContext->OMSetRenderTargets(1, pRTVs, nullptr);
	}

	return S_OK;
}

HRESULT CRenderer::Render_DepthMap() {
	ZoneScopedN("Render_DepthMap");
	{
		{
			ID3D11DepthStencilState* pDSS = nullptr;
			m_pContext->OMSetDepthStencilState(pDSS, 0);

			SPtr<CResDepthStencilState> DepthWriteState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHWRITE");
			m_pContext->OMSetDepthStencilState(DepthWriteState->GetDepthStencilState().Get(), 0);

			m_pContext->ClearDepthStencilView(m_pResDynTexTargetDepth->GetDSV().Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
		} 
		{
			ID3D11RenderTargetView* pRTVs[1] = { m_pResDynTexTargetDepth->GetRTV().Get() };
			m_pContext->OMSetRenderTargets(1, pRTVs, m_pResDynTexTargetDepth->GetDSV().Get());
			m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());
		}

		auto pGameCam = CGameInstance::Get().GetActiveCamera();
		if (nullptr == pGameCam)    return S_OK;

		if (FAILED(Reset_RenderContext(RENDERPASS::DEPTH, pGameCam))) return E_FAIL;

		if (FAILED(Bind_CameraAttribute(pGameCam))) return E_FAIL;

		{
			m_pContext->PSSetShader(nullptr, nullptr, 0);       // Depth 기록, PS 제외
		}

		if (FAILED(RenderNonBlend()))				return E_FAIL;

		{
			ID3D11RenderTargetView* pNullRTVs[1] = { nullptr };
			m_pContext->OMSetRenderTargets(1, pNullRTVs, nullptr);
		}
	}

	return S_OK;
}

HRESULT CRenderer::Render_NonAlpha() {
	ZoneScopedN("Render_NonAlpha");
	{
		ID3D11RenderTargetView* pRTVs[4] = {
			m_pResDynTexTargetDiffuse->GetRTV().Get(),
			m_pResDynTexTargetNormal->GetRTV().Get(),
			m_pResDynTexTargetSMRO->GetRTV().Get(),
			m_pResDynTexTargetEmissive->GetRTV().Get(),
		};
		m_pContext->OMSetRenderTargets(4, pRTVs, m_pResDynTexTargetDepth->GetDSV().Get());
		m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());

		SPtr<CResDepthStencilState> DepthState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHREAD");
		m_pContext->OMSetDepthStencilState(DepthState->GetDepthStencilState().Get(), 0);

		_float4 clearColor = { 0.f, 0.f, 1.f, 1.f };
		m_pContext->ClearRenderTargetView(pRTVs[0], reinterpret_cast<const float*>(&clearColor));
		m_pContext->ClearRenderTargetView(pRTVs[1], reinterpret_cast<const float*>(&clearColor));
		m_pContext->ClearRenderTargetView(pRTVs[2], reinterpret_cast<const float*>(&clearColor));
		m_pContext->ClearRenderTargetView(pRTVs[3], reinterpret_cast<const float*>(&clearColor));
	}
	{
		const auto& VS_NonAlpha = m_pResVertexShader;
		const auto& PS_NonAlpha = m_pResPixelShader;

		m_pContext->IASetInputLayout(VS_NonAlpha->GetInputLayout().Get());
		m_pContext->VSSetShader(VS_NonAlpha->GetVertexShader().Get(), nullptr, 0);
		m_pContext->PSSetShader(PS_NonAlpha->GetPixelShader().Get(), nullptr, 0);
	} 
	{
		auto pGameCam = CGameInstance::Get().GetActiveCamera();
		if (nullptr == pGameCam)    return S_OK;

		if (FAILED(Reset_RenderContext(RENDERPASS::DEFAULT, pGameCam))) return E_FAIL;

		if (FAILED(Bind_CameraAttribute(pGameCam)))                     return E_FAIL;

		if (FAILED(RenderPriority()))									return E_FAIL;

		if (FAILED(RenderNonBlend()))									return E_FAIL;

		if (FAILED(RenderNonBlend_Instanced()))							return E_FAIL;

		if (FAILED(RenderLight()))										return E_FAIL;
	}
	
	Unbind_Resources();

	return S_OK;
}

HRESULT CRenderer::Render_HBAO() {

	ID3D11RenderTargetView* pRTVs[4] = { nullptr, nullptr, nullptr, nullptr };
	m_pContext->OMSetRenderTargets(4, pRTVs, nullptr);

	if (auto pCam = CGameInstance::Get().GetActiveCamera()) {

		GFSDK_SSAO_InputData_D3D11 Input;
		Input.DepthData.DepthTextureType = GFSDK_SSAO_HARDWARE_DEPTHS;
		Input.DepthData.pFullResDepthTextureSRV = m_pResDynTexTargetDepth->GetSRV().Get();
		Input.NormalData.pFullResNormalTextureSRV = m_pResDynTexTargetNormal->GetSRV().Get();
		Input.NormalData.Enable = true;

		Input.NormalData.DecodeScale = 2.0f;
		Input.NormalData.DecodeBias = -1.0f;
		auto ViewMat = pCam->GetView();
		float ViewMat16[16]{};

		// 앞서 투영행렬 복사한 것과 똑같이 16개 배열에 복사
		memcpy(&ViewMat16[0], &ViewMat.r[0], sizeof(float) * 4);
		memcpy(&ViewMat16[4], &ViewMat.r[1], sizeof(float) * 4);
		memcpy(&ViewMat16[8], &ViewMat.r[2], sizeof(float) * 4);
		memcpy(&ViewMat16[12], &ViewMat.r[3], sizeof(float) * 4);

		Input.NormalData.WorldToViewMatrix.Data = GFSDK_SSAO_Float4x4(ViewMat16);
		Input.NormalData.WorldToViewMatrix.Layout = GFSDK_SSAO_ROW_MAJOR_ORDER;

		float ProjMat[16]{};

		auto Mat = pCam->GetProj();
		memcpy(&ProjMat[0], &Mat.r[0], sizeof(float) * 4);
		memcpy(&ProjMat[4], &Mat.r[1], sizeof(float) * 4);
		memcpy(&ProjMat[8], &Mat.r[2], sizeof(float) * 4);
		memcpy(&ProjMat[12], &Mat.r[3], sizeof(float) * 4);

		// 3. 배열을 구조체 생성자에 전달하여 변환 완료
		GFSDK_SSAO_Float4x4 ssaoProjMatrix(ProjMat);
		Input.DepthData.ProjectionMatrix.Data = GFSDK_SSAO_Float4x4(ProjMat);
		Input.DepthData.ProjectionMatrix.Layout = GFSDK_SSAO_ROW_MAJOR_ORDER;

		float SceneScale = 1.0f;
		Input.DepthData.MetersToViewSpaceUnits = SceneScale;

		GFSDK_SSAO_Parameters Params;
		Params.Radius = 4.f;
		Params.Bias = 0.1f;
		Params.PowerExponent = 4.f;
		Params.Blur.Enable = true;
		Params.Blur.Radius = GFSDK_SSAO_BLUR_RADIUS_4;
		Params.Blur.Sharpness = 16.f;
		//Params.DualLayerAO = true;
		GFSDK_SSAO_Output_D3D11 Output;

		Output.pRenderTargetView = m_pResDynTexTargetHBAO->GetRTV().Get();
		Output.Blend.Mode = GFSDK_SSAO_OVERWRITE_RGB;
		m_pGFSDK_SSAO->SetInputDepths(Input);
		m_pGFSDK_SSAO->SetAOParameters(Params);
		m_pGFSDK_SSAO->SetRenderTarget(Output);

		if (FAILED(m_pGFSDK_SSAO->RenderAO())) {
			MSG_BOX("Renderer : HBAO Render Failed.");
		}
	}
	return S_OK;
}

HRESULT CRenderer::Render_Lighting() {
	ComPtr<ID3D11ShaderResourceView> SRVList[] = {
		m_pResDynTexTargetDiffuse->GetSRV(),
		m_pResDynTexTargetNormal->GetSRV(),
		m_pResDynTexTargetSMRO->GetSRV(),
		m_pResDynTexTargetEmissive->GetSRV(),
		m_pResDynTexTargetHBAO->GetSRV(),
		m_pResDynTexTargetDepth->GetSRV(),
	};
	m_pContext->CSSetShaderResources(0, 6, SRVList->GetAddressOf());

	//ComPtr<ID3D11ShaderResourceView> ExtraSRVList[3] = {
	//	m_pIrridianceSRV.Get(),
	//	m_pPreFilterSRV.Get(),
	//	m_pLUTSRV.Get()
	//};
	//
	//m_pContext->CSSetShaderResources(10, 1, &pIBLSRVs[0]);
	//m_pContext->CSSetShaderResources(11, 1, &pIBLSRVs[1]);
	//m_pContext->CSSetShaderResources(12, 1, &pIBLSRVs[2]);

	CGameInstance::Get().Render_ObjectShadow();

	Unbind_Resources();
	
	m_pResDynTexTargetPreviousRenderView = CGameInstance::Get().Get_CombinedResource();
	
	return S_OK;

	{
		CGameInstance::Get().Bind_DynamicLight();
	
		ID3D11RenderTargetView* pRTVs[1] = { m_pResDynTexTargetLight->GetRTV().Get() };
		m_pContext->OMSetRenderTargets(1, pRTVs, nullptr);
		m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());
	
		_float4 clearColor = { 0.f, 0.f, 1.f, 1.f };
		m_pContext->ClearRenderTargetView(pRTVs[0], reinterpret_cast<const float*>(&clearColor));
		m_pContext->ClearDepthStencilView(m_pBackBufferDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
	
		SPtr<CResDepthStencilState> DepthState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHREAD");
		m_pContext->OMSetDepthStencilState(DepthState->GetDepthStencilState().Get(), 0);
	
		const auto& FullScreenBuffer = m_pFullscreenVIBuffer;

		m_pContext->PSSetShader(m_pPBRPixelShader->GetPixelShader().Get(), nullptr, 0);
	
		m_pContext->IASetInputLayout(m_pPBRVertexShader->GetInputLayout().Get());
	
		ID3D11Buffer* vertexBuffers[] = {
			FullScreenBuffer->GetVertexBuffer().Get()
		};
		uint32_t strides[] = {
			FullScreenBuffer->GetVertexStride()
		};
		uint32_t offsets[] = {
			0
		};
		m_pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
		m_pContext->IASetIndexBuffer(FullScreenBuffer->GetIndexBuffer().Get(), FullScreenBuffer->GetIndexFormat(), 0);
		m_pContext->IASetPrimitiveTopology(FullScreenBuffer->GetPrimitiveType());
	
		{   // Diffuse
			ID3D11ShaderResourceView* pSRVs[1] = { m_pResDynTexTargetDiffuse->GetSRV().Get() };
			m_pContext->PSSetShaderResources(0, 1, pSRVs);
		}
		{   // Normal
			ID3D11ShaderResourceView* pSRVs[1] = { m_pResDynTexTargetNormal->GetSRV().Get() };
			m_pContext->PSSetShaderResources(1, 1, pSRVs);
		}
		{   // SMRO
			ID3D11ShaderResourceView* pSRVs[1] = { m_pResDynTexTargetSMRO->GetSRV().Get() };
			m_pContext->PSSetShaderResources(2, 1, pSRVs);
		}
		{   // Emissive
			ID3D11ShaderResourceView* pSRVs[1] = { m_pResDynTexTargetEmissive->GetSRV().Get() };
			m_pContext->PSSetShaderResources(3, 1, pSRVs);
		}
		{   // Depth
			ID3D11ShaderResourceView* pSRVs[1] = { m_pResDynTexTargetDepth->GetSRV().Get() };
			m_pContext->PSSetShaderResources(4, 1, pSRVs);
		}
		{   // HBAO
			auto WhiteResource = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_WHITE");
			//m_pResDynTexTargetHBAO
			ID3D11ShaderResourceView* pSRVs[1] = { WhiteResource->GetSRV().Get() };
			m_pContext->PSSetShaderResources(5, 1, pSRVs);
		}
		{   // Shadow
			ComPtr<ID3D11ShaderResourceView> ShadowResource = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_WHITE")->GetSRV();
			if (bApplyShadow) {
				ShadowResource = m_pResDynTexTargetShadow->GetSRV();
			}
			m_pContext->PSSetShaderResources(6, 1, ShadowResource.GetAddressOf());
		}
	
		SPtr<CResTexture2D> NoiseTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_NOISE");
		m_pContext->PSSetShaderResources(13, 1, NoiseTexture->GetSRV().GetAddressOf());
	
		// Draw On PBRScreen
		m_pContext->DrawIndexed(FullScreenBuffer->GetNumIndices(), 0, 0);
	}
	
	m_pResDynTexTargetPreviousRenderView = m_pResDynTexTargetLight;

	Unbind_Resources();

	return S_OK;
}

HRESULT CRenderer::Render_Alpha() {
	m_pContext->RSSetState(Rasterizer->GetRasterizerState().Get());

	ZoneScopedN("Render_Alpha");
	{
		m_pContext->CopyResource(
			m_pResDynTexTargetPBR->GetTexture().Get(),
			m_pResDynTexTargetPreviousRenderView->GetTexture().Get());
	}
	{
		ID3D11RenderTargetView* pRTVs[1] = { m_pResDynTexTargetPBR->GetRTV().Get() };
		m_pContext->OMSetRenderTargets(1, pRTVs, m_pResDynTexTargetDepth->GetDSV().Get());
		m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());
	}
	{
		m_pContext->IASetInputLayout(m_pBlendVertexShader->GetInputLayout().Get());
		m_pContext->VSSetShader(m_pBlendVertexShader->GetVertexShader().Get(), nullptr, 0);
		m_pContext->PSSetShader(m_pBlendPixelShader->GetPixelShader().Get(), nullptr, 0);
	}
	{
		auto pGameCam = CGameInstance::Get().GetActiveCamera();
		if (nullptr == pGameCam)    return S_OK;

		if (FAILED(Reset_RenderContext(RENDERPASS::DEFAULT, pGameCam))) return E_FAIL;

		if (FAILED(Bind_CameraAttribute(pGameCam)))                     return E_FAIL;

		if (FAILED(RenderBlend()))										return E_FAIL;

		if (FAILED(RenderSkybox()))										return E_FAIL;

		if (FAILED(RenderCollider()))									return E_FAIL;

		//if (FAILED(RenderParticle()))									return E_FAIL;
	}
		
	Unbind_Resources();
	
	m_pResDynTexTargetPreviousRenderView = m_pResDynTexTargetPBR;

	return S_OK;
}

HRESULT CRenderer::Render_Effect()
{
	ZoneScopedN("Render_Effect");
	{
		m_pContext->CopyResource(
			m_pResDynTexTargetEffect->GetTexture().Get(),
			m_pResDynTexTargetPreviousRenderView->GetTexture().Get());
	}
	{
		ID3D11ShaderResourceView* pBackgroundSRV = m_pResDynTexTargetPreviousRenderView->GetSRV().Get();
		m_pContext->PSSetShaderResources(7, 1, &pBackgroundSRV);
	}
	{
		ID3D11RenderTargetView* pRTVs[1] = { m_pResDynTexTargetEffect->GetRTV().Get() };
		m_pContext->OMSetRenderTargets(1, pRTVs,  m_pResDynTexTargetDepth->GetDSV().Get());
		m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());
	}

	auto pGameCam = CGameInstance::Get().GetActiveCamera();
	if (nullptr == pGameCam)    return S_OK;

	if (FAILED(Reset_RenderContext(RENDERPASS::DEFAULT, pGameCam))) return E_FAIL;

	if (FAILED(Bind_CameraAttribute(pGameCam)))                     return E_FAIL;

	if (FAILED(RenderEffect()))										return E_FAIL;

	Unbind_Resources();

	m_pResDynTexTargetPreviousRenderView = m_pResDynTexTargetEffect;

	return S_OK;
}

HRESULT CRenderer::Render_VolumetricEffect() {
	if (ApplyVolumetric == false) return S_OK;
	ZoneScopedN("Render_VolumetricEffect");
	{
		ID3D11RenderTargetView* NullRTV[1] = { nullptr }; 
		m_pContext->OMSetRenderTargets(1, NullRTV, nullptr);

		const auto& cs = m_pVolumetricComputeShader;

		m_pContext->CSSetShader(cs->GetComputeShader().Get(), nullptr, 0);

		ID3D11UnorderedAccessView* pUAVs[1] = { m_pResDynTexUAVVolumetric->GetUAV().Get() };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pUAVs, nullptr);
	}
	{
		ID3D11ShaderResourceView* pSRVs[5] = {
			m_pResDynTexTargetPreviousRenderView->GetSRV().Get(),
			m_pResDynTexTargetShadow->GetSRV().Get(),
			BlueNoiseTexture.Get(),
			VolumeTexture.Get()
		};
		Bind_VolumetricFog();
		m_pContext->CSSetShaderResources(0, 5, pSRVs);
	}
	{
		uint32_t ScreenResolutionX = { 1280 };
		uint32_t ScreenResolutionY = { 720 };

		UINT GroupX = (ScreenResolutionX + 15) / 16;
		UINT GroupY = (ScreenResolutionY + 15) / 16;
		UINT GroupZ = 1;
		m_pContext->Dispatch(GroupX, GroupY, GroupZ);
	}
	{
		ID3D11UnorderedAccessView* NullUAV[1] = { nullptr };
		m_pContext->CSSetUnorderedAccessViews(0, 1, NullUAV, nullptr);

		ID3D11ShaderResourceView* NullSRVs[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
		m_pContext->CSSetShaderResources(0, 5, NullSRVs);
	}

	Unbind_Resources();

	m_pResDynTexTargetPreviousRenderView = m_pResDynTexUAVVolumetric;

	return S_OK;
}

HRESULT CRenderer::Render_OffScreen() {
	ZoneScopedN("Render_OffScreen");
	{
		ID3D11RenderTargetView* nullRTVs[1] = { nullptr };
		m_pContext->OMSetRenderTargets(1, nullRTVs, nullptr);

		ID3D11ShaderResourceView* nullSRVs[4] = { nullptr, nullptr, nullptr, nullptr };
		m_pContext->PSSetShaderResources(0, 4, nullSRVs);

		ID3D11RenderTargetView* pRTVs[1] = { m_pOffScreenTex2D->GetRTV().Get() };
		m_pContext->OMSetRenderTargets(1, pRTVs, nullptr);
		m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());

		_float4 clearColor = { 0.f, 0.f, 1.f, 1.f };
		m_pContext->ClearRenderTargetView(pRTVs[0], reinterpret_cast<const float*>(&clearColor));
		m_pContext->ClearDepthStencilView(m_pBackBufferDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		const auto& vs = m_pOffScreenVertexShader;
		const auto& ps = m_pOffScreenPixelShader;
		const auto& viBuffer = m_pFullscreenVIBuffer;

		m_pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
		m_pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

		m_pContext->IASetInputLayout(vs->GetInputLayout().Get());

		ID3D11Buffer* vertexBuffers[] = {
			viBuffer->GetVertexBuffer().Get()
		};
		uint32_t strides[] = {
			viBuffer->GetVertexStride()
		};
		uint32_t offsets[] = {
			0
		};

		m_pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
		m_pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		m_pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());
		
		// Bind Shader Resource
		{
			ComPtr<ID3D11ShaderResourceView> pSRVs = { m_pResDynTexTargetEffect->GetSRV() };
			m_pContext->PSSetShaderResources(0, 1, pSRVs.GetAddressOf());
		}
		//{
		//	ComPtr<ID3D11ShaderResourceView> pSRVs = { E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_DIFFUSE")->GetSRV() };
		//	m_pContext->PSSetShaderResources(1, 1, m_pResDynTexTargetPreviousRenderView->GetSRV().GetAddressOf());
		//}

		// Draw On OffScreen
		m_pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);

		Unbind_Resources();
		m_pContext->Flush();
		m_pResDynTexTargetPreviousRenderView = m_pOffScreenTex2D;
	}

	return S_OK;
}

HRESULT CRenderer::Render_PostProcess() {
	ID3D11RenderTargetView* pRTVs[1] = { m_pResDynTexTargetPostProcess->GetRTV().Get() };
	m_pContext->OMSetRenderTargets(1, pRTVs, nullptr);
	m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());

	auto pCbPostProcess = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PostProcess");

	_float4 clearColor = { 0.f, 0.f, 1.f, 1.f };
	m_pContext->ClearRenderTargetView(m_pBackBufferRTV.Get(), reinterpret_cast<float*>(&clearColor));

	D3D11_MAPPED_SUBRESOURCE MRES;
	if (SUCCEEDED(m_pContext->Map(pCbPostProcess->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
	{
		POSTPROCESS CBPP{};
		CBPP.BloomIntensity = 1.f;
		CBPP.DistortionIntensity = m_pDistortionIntensity;
		CBPP.ChromaticIntensity = m_pChromaticIntensity;
		CBPP.VignetteIntensity = m_pVignetteIntensity;
		CBPP.VignetteSmoothness = m_pVignetteSmoothness;

		memcpy(MRES.pData, &CBPP, sizeof(POSTPROCESS));
		m_pContext->Unmap(pCbPostProcess->GetCBuffer().Get(), 0);
	}
	m_pContext->PSSetConstantBuffers(8, 1, pCbPostProcess->GetCBuffer().GetAddressOf());

	Render_PostProcess_Bloom();

	Render_PostProcess_Filter();

	return S_OK;
}

HRESULT CRenderer::Render_PostProcess_Bloom() {
	const auto& vs = m_pOffScreenVertexShader;
	const auto& viBuffer = m_pFullscreenVIBuffer;

	m_pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	m_pContext->IASetInputLayout(vs->GetInputLayout().Get());

	ID3D11Buffer* vertexBuffers[] = { viBuffer->GetVertexBuffer().Get() };
	uint32_t strides[] = { viBuffer->GetVertexStride() };
	uint32_t offsets[] = { 0 };

	const auto& BrightPassPS = m_pBrightPassPixelShader;
	const auto& VerticalBlurPS = m_pVerticalBlurPixelShader;
	const auto& CombinedPS = m_pBloomPassPixelShader;

	ID3D11ShaderResourceView* NullSRV[2] = { nullptr, nullptr };

	_float ClearColor[] = { 0.f, 0.f, 1.f, 1.f };

	m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());

	//////////////////////////// Bright Pass /////////////////////////////////
	ID3D11RenderTargetView* BRPRTV[1] = { m_pResDynTexTargetPostProcess->GetRTV().Get() };

	m_pContext->ClearRenderTargetView(BRPRTV[0], reinterpret_cast<float*>(&ClearColor));
	m_pContext->OMSetRenderTargets(1, BRPRTV, nullptr);
	m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());

	m_pContext->PSSetShader(BrightPassPS->GetPixelShader().Get(), nullptr, 0);
	m_pContext->PSSetShaderResources(0, 1, m_pOffScreenTex2D->GetSRV().GetAddressOf());

	m_pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);

	m_pContext->OMSetRenderTargets(0, nullptr, nullptr);

	m_pResDynTexTargetPreviousRenderView = m_pResDynTexTargetPostProcess;

	//////////////////////////// Vertical Blur /////////////////////////////////
	m_pContext->PSSetShaderResources(1, 1, &NullSRV[0]);

	ID3D11RenderTargetView* BlurRTV[1] = { m_pResDynTexTargetBrightPass->GetRTV().Get() };

	m_pContext->ClearRenderTargetView(BlurRTV[0], reinterpret_cast<float*>(&ClearColor));
	m_pContext->OMSetRenderTargets(1, BlurRTV, nullptr);

	m_pContext->PSSetShader(VerticalBlurPS->GetPixelShader().Get(), nullptr, 0);
	m_pContext->PSSetShaderResources(1, 1, m_pResDynTexTargetPreviousRenderView->GetSRV().GetAddressOf());

	m_pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	m_pContext->OMSetRenderTargets(0, nullptr, nullptr);

	m_pResDynTexTargetPreviousRenderView = m_pResDynTexTargetBrightPass;

	////////////////////////////// Bloom Combined /////////////////////////////////
	m_pContext->PSSetShaderResources(1, 1, &NullSRV[0]);

	ID3D11RenderTargetView* CombinedRTV[1] = { m_pResDynTexTargetPostProcess->GetRTV().Get() };

	m_pContext->ClearRenderTargetView(CombinedRTV[0], reinterpret_cast<float*>(&ClearColor));
	m_pContext->OMSetRenderTargets(1, CombinedRTV, nullptr);

	m_pContext->PSSetShader(CombinedPS->GetPixelShader().Get(), nullptr, 0);
	m_pContext->PSSetShaderResources(0, 1, m_pOffScreenTex2D->GetSRV().GetAddressOf());
	m_pContext->PSSetShaderResources(1, 1, m_pResDynTexTargetPreviousRenderView->GetSRV().GetAddressOf());

	m_pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);

	m_pContext->OMSetRenderTargets(0, nullptr, nullptr);

	m_pResDynTexTargetPreviousRenderView = m_pResDynTexTargetPostProcess;

	//////////////////////////// Unbind Resource /////////////////////////////////
	m_pContext->PSSetShaderResources(0, 2, NullSRV);

	return S_OK;
}

HRESULT CRenderer::Render_PostProcess_Filter() {
	ID3D11RenderTargetView* CombinedRTV[1] = { m_pOffScreenTex2D->GetRTV().Get() };
	m_pContext->OMSetRenderTargets(1, CombinedRTV, nullptr);

	const auto& vs = m_pOffScreenVertexShader;
	const auto& ps = m_pPostProcessPS;
	const auto& viBuffer = m_pFullscreenVIBuffer;

	m_pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	m_pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	m_pContext->IASetInputLayout(vs->GetInputLayout().Get());
	
	ID3D11Buffer* vertexBuffers[] = {
			viBuffer->GetVertexBuffer().Get()
	};
	uint32_t strides[] = {
		viBuffer->GetVertexStride()
	};
	uint32_t offsets[] = {
		0
	};

	m_pContext->PSSetShaderResources(0, 1, m_pResDynTexTargetPreviousRenderView->GetSRV().GetAddressOf());		// Combined Texture
	m_pContext->PSSetShaderResources(1, 1, m_pLUTTexture.GetAddressOf());										// LUT Texture

	m_pContext->DrawIndexed(m_pFullscreenVIBuffer->GetNumIndices(), 0, 0);

	Unbind_Resources();

	m_pResDynTexTargetPreviousRenderView = m_pOffScreenTex2D;

	return S_OK;
}

HRESULT CRenderer::Render_UserInterface() {
	auto pUICame = CGameInstance::Get().GetCamera("UI");
	if (nullptr == pUICame) return S_OK;

	RenderContext.matProj = pUICame->GetProj();
	RenderContext.matView = pUICame->GetView();
	RenderContext.matViewProj = RenderContext.matView * RenderContext.matProj;
	RenderContext.eye = pUICame->GetTransform().GetLoadedPostion();

	ZoneScopedN("Render_UserInterface");
	{
		{
			m_pContext->CopyResource(
				m_pResDynTexTargetUI->GetTexture().Get(),
				m_pResDynTexTargetPreviousRenderView->GetTexture().Get());
		}
		ID3D11RenderTargetView* pRTVs[1] = { m_pResDynTexTargetUI->GetRTV().Get() };
		m_pContext->OMSetRenderTargets(1, pRTVs, nullptr);
		m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());

		//_float4 ClearColor = { 0.f, 0.f, 1.f, 1.f };
		//m_pContext->ClearRenderTargetView(m_pResDynTexTargetUI->GetRTV().Get(), reinterpret_cast<float*>(&ClearColor));

		const auto& vs = m_pFullscreenVS;
		const auto& ps = m_pFullscreenPS;
		const auto& viBuffer = m_pFullscreenVIBuffer;

		m_pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
		m_pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

		m_pContext->IASetInputLayout(vs->GetInputLayout().Get());

		ID3D11Buffer* vertexBuffers[] = {
			   viBuffer->GetVertexBuffer().Get()
		};
		uint32_t strides[] = {
			   viBuffer->GetVertexStride()
		};
		uint32_t offsets[] = {
			   0
		};

		m_pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
		m_pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		m_pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		m_pContext->PSSetShaderResources(0, 1, m_pResDynTexTargetPreviousRenderView->GetSRV().GetAddressOf());		// Combined Texture

		if (FAILED(Bind_CameraAttribute(pUICame)))			return E_FAIL;

		if (FAILED(RenderUI()))			return E_FAIL;

		Unbind_Resources();

		m_pResDynTexTargetPreviousRenderView = m_pResDynTexTargetUI;
	}

	return S_OK;
}

HRESULT CRenderer::Render_FullScreen()	
{
	ZoneScopedN("DrawFullscreen");
	ID3D11RenderTargetView* pBackBufferRTVs[1] = { m_pBackBufferRTV.Get() };
	m_pContext->OMSetRenderTargets(1, pBackBufferRTVs, nullptr);

	_float4 clearColor = { 0.f, 0.f, 1.f, 1.f };
	m_pContext->ClearRenderTargetView(m_pBackBufferRTV.Get(), reinterpret_cast<float*>(&clearColor));

	const auto& vs = m_pFullscreenVS;
	const auto& ps = m_pFullscreenPS;
	const auto& viBuffer = m_pFullscreenVIBuffer;

	m_pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	m_pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	m_pContext->IASetInputLayout(vs->GetInputLayout().Get());

	ID3D11Buffer* vertexBuffers[] = {
			viBuffer->GetVertexBuffer().Get()
	};
	uint32_t strides[] = {
		viBuffer->GetVertexStride()
	};
	uint32_t offsets[] = {
		0
	};

	m_pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	m_pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
	m_pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

	if (CGameInstance::Get().KeyPressing(DIK_N))
	{
		ID3D11ShaderResourceView* pSRVs[1] = { m_pResDynTexTargetHBAO->GetSRV().Get() };
		m_pContext->PSSetShaderResources(0, 1, pSRVs);
	}
	else
	{
		ID3D11ShaderResourceView* pSRVs[1] = { m_pResDynTexTargetPreviousRenderView->GetSRV().Get() };
		m_pContext->PSSetShaderResources(0, 1, pSRVs);
	}

	m_pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);

	ID3D11ShaderResourceView* pNullSRVs[1] = { nullptr };
	m_pContext->PSSetShaderResources(0, 1, pNullSRVs);

	return S_OK;
}

HRESULT CRenderer::RenderPriority()
{
	ZoneScopedN("RenderPriority");
	for (auto& pRenderObject : m_RenderObject[ETOUI(RENDERGROUP::PRIORITY)])
	{
		if (pRenderObject->HasRenderPass(RenderContext.pass))
		{
			pRenderObject->Render(m_pContext.Get(), RenderContext);
		}
	}

	return S_OK;
}

HRESULT CRenderer::RenderNonBlend() {
	ZoneScopedN("RenderNonBlend");
	for (auto& pRenderObject : m_RenderObject[ETOUI(RENDERGROUP::NONBLEND)])
	{
		if (pRenderObject->HasRenderPass(RenderContext.pass))
		{
			pRenderObject->Render(m_pContext.Get(), RenderContext);
		}
	}

	return S_OK;
}

HRESULT CRenderer::RenderNonBlend_Instanced() {
	ZoneScopedN("RenderNonBlend_Instanced");


	for (auto& pRenderObject : m_RenderObject[ETOUI(RENDERGROUP::NONBLEND_INSTANCED)])
	{
		if (pRenderObject->HasRenderPass(RenderContext.pass))
		{
			pRenderObject->Render(m_pContext.Get(), RenderContext);
		}
	}

	return S_OK;

}

HRESULT CRenderer::RenderBlend()
{
	ZoneScopedN("RenderBlend");

	auto BlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_EFFECT");
	m_pContext->OMSetBlendState(BlendState->GetBlendState().Get(), nullptr, 0xffffffff);

	for (auto& pRenderObject : m_RenderObject[ETOUI(RENDERGROUP::BLEND)])
	{
		if (pRenderObject->HasRenderPass(RenderContext.pass))
		{
			pRenderObject->Render(m_pContext.Get(), RenderContext);
		}
	}

	BlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_BLEND_NONE");
	m_pContext->OMSetBlendState(BlendState->GetBlendState().Get(), nullptr, 0xffffffff);

	return S_OK;
}

HRESULT CRenderer::RenderLight() {
	for (auto& pRenderObject : m_RenderObject[ETOUI(RENDERGROUP::LIGHT)])
	{
		if (pRenderObject->HasRenderPass(RenderContext.pass))
		{
			pRenderObject->Render(m_pContext.Get(), RenderContext);
		}
	}
	return S_OK;
}

HRESULT CRenderer::RenderSkybox()
{
	ZoneScopedN("RenderSkybox");
	for (auto& pRenderObject : m_RenderObject[ETOUI(RENDERGROUP::SKYBOX)])
	{
		if (pRenderObject->HasRenderPass(RenderContext.pass))
		{
			pRenderObject->Render(m_pContext.Get(), RenderContext);
		}
	}

	return S_OK;
}

HRESULT CRenderer::RenderEffect()
{
	auto BlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_EFFECT");
	m_pContext->OMSetBlendState(BlendState->GetBlendState().Get(), nullptr, 0xffffffff);


	for (auto& pRenderObject : m_RenderObject[ETOUI(RENDERGROUP::EFFECT)])
	{
		if (pRenderObject->HasRenderPass(RenderContext.pass))
		{
			pRenderObject->Render(m_pContext.Get(), RenderContext);
		}
	}

	BlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_BLEND_NONE");
	m_pContext->OMSetBlendState(BlendState->GetBlendState().Get(), nullptr, 0xffffffff);


	return S_OK;
}

HRESULT CRenderer::RenderCollider()
{
	ZoneScopedN("RenderCollider");

	for (auto& pRenderObject : m_RenderObject[ETOUI(RENDERGROUP::COLLIDER)])
	{
		if (pRenderObject->HasRenderPass(RenderContext.pass))
		{
				pRenderObject->Render(m_pContext.Get(), RenderContext);
		}
	}

	return S_OK;
}

HRESULT CRenderer::RenderUI()
{
	auto& renderList = m_RenderObject[ETOUI(RENDERGROUP::UI)];

	std::sort(renderList.begin(), renderList.end(),
		[](const IRenderable* lhs, const IRenderable* rhs)
		{
			const CUIObject* l = static_cast<const CUIObject*>(lhs);
			const CUIObject* r = static_cast<const CUIObject*>(rhs);
			return l->GetWeight() < r->GetWeight();
		});

	auto noDepth = E::CGameInstance::Get().GetResourceFirst<E::CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_NO_DEPTHSTENCIL");

	m_pContext->OMSetDepthStencilState(
		noDepth->GetDepthStencilState().Get(),
		0);

	auto Alphablend = E::CGameInstance::Get().GetResourceFirst<E::CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_BLEND");
	m_pContext->OMSetBlendState(Alphablend->GetBlendState().Get(), nullptr, 0xffffffff);

	for (auto* pRenderObject : renderList)
	{
		if (pRenderObject->HasRenderPass(RenderContext.pass))
		{
			pRenderObject->Render(m_pContext.Get(), RenderContext);
		}
	}

	{
		E::CGameInstance::Get().FontLateDraw(RENDERGROUP::UI);
	}

	auto Nonblend = E::CGameInstance::Get().GetResourceFirst<E::CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_BLEND_NONE");
	m_pContext->OMSetBlendState(Nonblend->GetBlendState().Get(), nullptr, 0xffffffff);

	return S_OK;
}
#pragma endregion

VOID	CRenderer::PostProcessGUI() {
	ImGui::Begin("PostProcess");

	if (ApplyFilter ? ImGui::Button("PostProcess ON", ImVec2(-FLT_MIN, 20)) : ImGui::Button("PostProcess OFF", ImVec2(-FLT_MIN, 20))) {
		ApplyFilter = !ApplyFilter;
	}
	if (ApplyVolumetric ? ImGui::Button("Volumetric ON", ImVec2(-FLT_MIN, 20)) : ImGui::Button("Volumetric OFF", ImVec2(-FLT_MIN, 20))) {
		ApplyVolumetric = !ApplyVolumetric;
	}

	ImGui::InputFloat("DistortionIntensity", &m_pDistortionIntensity);
	ImGui::InputFloat("ChromaticIntensity", &m_pChromaticIntensity);
	ImGui::InputFloat("VignetteIntensity", &m_pVignetteIntensity);
	ImGui::InputFloat("VignetteSmoothness", &m_pVignetteSmoothness);

	ImGui::End();
}

VOID CRenderer::VolumetricFogGUI() {
	ImGui::Begin("VolumetricFog");

	ImGui::SliderFloat("Intensity", &m_fFogIntensity, 0.f, 1.f, "%.2f");
	if (ImGui::ColorEdit3("Color", (float*)&m_fFogColor))
	{
		if (m_fFogColor.x < 0.f) m_fFogColor.x = 0.f;
		if (m_fFogColor.x > 1.f) m_fFogColor.x = 1.f;

		if (m_fFogColor.y < 0.f) m_fFogColor.y = 0.f;
		if (m_fFogColor.y > 1.f) m_fFogColor.y = 1.f;

		if (m_fFogColor.z < 0.f) m_fFogColor.z = 0.f;
		if (m_fFogColor.z > 1.f) m_fFogColor.z = 1.f;
	}

	ImGui::Separator();

	ImGui::DragFloat("Start Distance", &m_fFogStartPos, 1.f, 0.f, 100.f, "%.1f");
	ImGui::DragFloat("End Distance", &m_fFogEndPos, 1.f, m_fFogStartPos, 500.f, "%.1f");

	ImGui::Separator();

	ImGui::DragFloat("Max Height", &m_fFogMaxHeight, 0.5f, -30.f, 30.f, "%.1f");
	ImGui::DragFloat("Density", &m_fFogDensity, 0.0001f, 0.f, 0.1f, "%.4f");

	ImGui::End();
}

HRESULT CRenderer::Initialize_Debugging()
{
	m_pDebugBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");
	if (!m_pDebugBuffer)			return E_FAIL;

	m_pDebugVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTex");
	if (!m_pDebugVertexShader)		return E_FAIL;

	m_pDebugPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTex");
	if (!m_pDebugPixelShader)		return E_FAIL;

	XMFLOAT2	vViewportSize = { m_pBackBufferViewPort->GetViewPort().Width , m_pBackBufferViewPort->GetViewPort().Height };
	XMFLOAT2    vDebugViewSize = { vViewportSize.x / 4.f, vViewportSize.y / 4.f };
	XMMATRIX    mDebugViewScaleMatrix = XMMatrixScaling(vDebugViewSize.x, vDebugViewSize.y, 1.f);

	XMFLOAT2    vDebugViewStartPoint = { vDebugViewSize.x * 0.5f - vViewportSize.x * 0.5f, -vDebugViewSize.y * 0.5f + vViewportSize.y * 0.5f };

	m_pResDynTexTargetList.push_back(m_pResDynTexTargetDiffuse);
	m_pResDynTexTargetList.push_back(m_pResDynTexTargetNormal);

	m_pResDynTexTargetList.push_back(m_pResDynTexTargetSMRO);
	m_pResDynTexTargetList.push_back(m_pResDynTexTargetEmissive);

	m_pResDynTexTargetList.push_back(m_pResDynTexTargetPBR);
	m_pResDynTexTargetList.push_back(m_pResDynTexTargetEffect);

	m_pResDynTexTargetList.push_back(m_pResDynTexTargetBrightPass);
	m_pResDynTexTargetList.push_back(m_pResDynTexTargetUI);

	for (uint32_t i = 0; i < m_pResDynTexTargetList.size(); i++)
	{
		_float fScreenPosX = vDebugViewStartPoint.x + (static_cast<_float>(i % 2) * vDebugViewSize.x);
		_float fScreenPosY = vDebugViewStartPoint.y - (static_cast<_float>(i / 2) * vDebugViewSize.y);

		XMStoreFloat4x4(&m_fDebugWorldMatrix[i], mDebugViewScaleMatrix * XMMatrixTranslation(fScreenPosX, fScreenPosY, 0.f));
	}

	XMStoreFloat4x4(&m_fDebugWorldMatrix[8], XMMatrixScaling(vDebugViewSize.x * 2.f, vDebugViewSize.y * 2.f, 1.f)
		* XMMatrixTranslation(-320.f + vDebugViewSize.x * 2.f, 180.f, 0.f));

	return S_OK;
}

HRESULT CRenderer::Render_Debugging() {
	if (nullptr == m_pResDynTexTargetList[6]) { m_pResDynTexTargetList[6] = CGameInstance::Get().Get_CombinedResource(); }

	if (CGameInstance::Get().KeyDown(DIK_F3))
		m_bRenderable = !m_bRenderable;

	if (!m_bRenderable) return S_OK;

	auto ActiveCam = CGameInstance::Get().GetActiveCamera();

	XMFLOAT2	vViewportSize = { m_pBackBufferViewPort->GetViewPort().Width, m_pBackBufferViewPort->GetViewPort().Height };

	XMMATRIX    m_WorldMatrix, m_ViewMatrix, m_ProjMatrix;
	m_ViewMatrix = XMMatrixIdentity();
	m_ProjMatrix = XMMatrixOrthographicLH(vViewportSize.x, vViewportSize.y, 0.f, 1.f);

	m_pContext->IASetInputLayout(m_pDebugVertexShader->GetInputLayout().Get());
	m_pContext->VSSetShader(m_pDebugVertexShader->GetVertexShader().Get(), nullptr, 0);
	m_pContext->PSSetShader(m_pDebugPixelShader->GetPixelShader().Get(), nullptr, 0);

	ID3D11Buffer* vertexBuffers[] = { m_pDebugBuffer->GetVertexBuffer().Get() };
	uint32_t strides[] = { m_pDebugBuffer->GetVertexStride() };
	uint32_t offsets[] = { 0 };

	m_pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	m_pContext->IASetIndexBuffer(m_pDebugBuffer->GetIndexBuffer().Get(), m_pDebugBuffer->GetIndexFormat(), 0);
	m_pContext->IASetPrimitiveTopology(m_pDebugBuffer->GetPrimitiveType());

	auto pCbPerObject = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT);
	D3D11_MAPPED_SUBRESOURCE MRES{};

    for (uint32_t IDX = 0; IDX < 9; ++IDX) {
	
        if (IDX != 8 && (IDX >= m_pResDynTexTargetList.size() || !m_pResDynTexTargetList[IDX]))
            continue;
	
        m_WorldMatrix = XMLoadFloat4x4(&m_fDebugWorldMatrix[IDX]);
	
        if (SUCCEEDED(m_pContext->Map(pCbPerObject->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
        {
            CB_PER_OBJECT cbPerPass{};
	
            XMStoreFloat4x4(&cbPerPass.matWorld, m_WorldMatrix);
            XMStoreFloat4x4(&cbPerPass.matWVP, m_WorldMatrix * m_ViewMatrix * m_ProjMatrix);
	
            memcpy(MRES.pData, &cbPerPass, sizeof(cbPerPass));
            m_pContext->Unmap(pCbPerObject->GetCBuffer().Get(), 0);
        }
        auto pCBufferPtr = pCbPerObject->GetCBuffer().GetAddressOf();
        m_pContext->VSSetConstantBuffers(0, 1, pCbPerObject->GetCBuffer().GetAddressOf());
        m_pContext->PSSetConstantBuffers(0, 1, pCbPerObject->GetCBuffer().GetAddressOf());
	
        if (IDX == 8) {
            m_pContext->PSSetShaderResources(0, 1, m_pBackBufferSRV.GetAddressOf());
        }
        else {
            m_pContext->PSSetShaderResources(0, 1, m_pResDynTexTargetList[IDX]->GetSRV().GetAddressOf());
        }
        
        m_pContext->DrawIndexed(m_pDebugBuffer->GetNumIndices(), 0, 0);
    }
    return S_OK;
}

HRESULT CRenderer::InitializeHizBuffer()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();

	m_pCurrentHizBuffer = CHizBuffer::Create(
		m_pDevice,
		m_pContext,
		static_cast<uint32_t>(clientSize.x),
		static_cast<uint32_t>(clientSize.y));

	if (m_pCurrentHizBuffer == nullptr)
		return E_FAIL;

	m_pPrevHizBuffer = CHizBuffer::Create(
		m_pDevice,
		m_pContext,
		static_cast<uint32_t>(clientSize.x),
		static_cast<uint32_t>(clientSize.y));

	if (m_pPrevHizBuffer == nullptr)
		return E_FAIL;

	m_bHasPrevHizBuffer = false;

	return S_OK;
}

HRESULT CRenderer::BuildCurrentHizBuffer()
{
	if (m_pCurrentHizBuffer == nullptr || m_pResDynTexTargetDepth == nullptr)
		return E_FAIL;

	ID3D11RenderTargetView* nullRTVs[4] = { nullptr, nullptr, nullptr, nullptr };
	m_pContext->OMSetRenderTargets(4, nullRTVs, nullptr);

	return m_pCurrentHizBuffer->Build(m_pResDynTexTargetDepth->GetSRV().Get());
}

UPtr<CRenderer> CRenderer::Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
{
	auto pInstance = ToUPtr(new CRenderer{ pDevice, pContext });
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CRenderer");
		return nullptr;
	}
	return pInstance;
}
