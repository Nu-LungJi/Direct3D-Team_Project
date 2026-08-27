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
CRenderer::CRenderer(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext) : m_pDevice{ pDevice }, m_pContext{ pContext }
{
}
CRenderer::~CRenderer()
{
	if (m_pTracyGpuContext != nullptr)
	{
		TracyD3D11Destroy(m_pTracyGpuContext);
		m_pTracyGpuContext = nullptr;
	}
}

VOID	CRenderer::UpdateGUI()
{
	RendererGUI();
}

VOID	CRenderer::Update(_float fTimeDelta) {
	m_fCurrentLifeTime += fTimeDelta;
	m_fDeltaTime = fTimeDelta;
	m_fTimeAccumulation += fTimeDelta;
	m_pCloudInfo.g_fWindTimeAccumulation += fTimeDelta;
}

HRESULT CRenderer::Initialize()
{
	m_pTracyGpuContext = TracyD3D11Context(m_pDevice.Get(), m_pContext.Get());
	TracyD3D11ContextName(m_pTracyGpuContext, "D3D11 Main Context", 18);

	if (FAILED(InitializeShaderResource()))     return E_FAIL;

	if (FAILED(InitializeBackBuffer()))         return E_FAIL;

	if (FAILED(InitializeGFSDK_SSAO()))         return E_FAIL;

	if (FAILED(InitializeFSR2_2()))				return E_FAIL;

	if (FAILED(InitializeFullscreen()))         return E_FAIL;

	if (FAILED(InitializeBaseTarget()))         return E_FAIL;

	if (FAILED(InitializeTargetPBR()))          return E_FAIL;

	if (FAILED(InitializeBlendTarget()))        return E_FAIL;

	if (FAILED(InitializePostProcess()))         return E_FAIL;

	if (FAILED(InitializeBloom()))				return E_FAIL;

	if (FAILED(InitializeVolumetricEffect()))	return E_FAIL;

	if (FAILED(InitializeUserInterface()))		return E_FAIL;

	if (FAILED(InitializeUI3D()))				return E_FAIL;

	if (FAILED(InitializeHizBuffer()))			return E_FAIL;

#ifdef _DEBUG
	if (FAILED(Initialize_Debugging()))         return E_FAIL;
#endif

	return S_OK;
}

#pragma region INITIALIZE
HRESULT CRenderer::InitializeShaderResource()
{
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PostProcess_Bloom_BrightPass", "./ShaderFiles/PostProcess/CS_PostProcess.hlsl"))
	{ 
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "CSMain_BrightPass", .sTarget = "cs_5_0" })))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PostProcess_Bloom_VerticalBlur", "./ShaderFiles/PostProcess/CS_PostProcess.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "CSMain_VerticalBlur", .sTarget = "cs_5_0" })))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PostProcess_Bloom_HorizontalBlur", "./ShaderFiles/PostProcess/CS_PostProcess.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "CSMain_HorizontalBlur", .sTarget = "cs_5_0" })))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PostProcess_Bloom_UpSampling", "./ShaderFiles/PostProcess/CS_PostProcess.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "CSMain_UpSampling", .sTarget = "cs_5_0" })))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PostProcess_Bloom_DownSampling", "./ShaderFiles/PostProcess/CS_PostProcess.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "CSMain_DownSampling", .sTarget = "cs_5_0" })))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PostProcess_Bloom_Combined", "./ShaderFiles/PostProcess/CS_PostProcess.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "CSMain_Combined", .sTarget = "cs_5_0" })))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PostProcess_Bloom_RadialBlur", "./ShaderFiles/PostProcess/CS_PostProcess.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "CSMain_RadialBlur", .sTarget = "cs_5_0" })))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PostProcess_Filter", "./ShaderFiles/PostProcess/CS_PostProcess.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "CSMain_PostProcess", .sTarget = "cs_5_0" })))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PostProcess_MotionBlur", "./ShaderFiles/PostProcess/PS_PostProcess_MotionBlur.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "PS_Main", .sTarget = "ps_5_0" })))    return E_FAIL;
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
	if (auto res = CGameInstance::Get().AddResourceT<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTex", "./ShaderFiles/QuadTex/QuadTex.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "PSMain_NonAlpha", .sTarget = "ps_5_0" })))			return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_UI3D", "./ShaderFiles/UI/UI3D.hlsl"))
	{
		if (FAILED(res->Load()))			return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_UI3D", "./ShaderFiles/UI/UI3D.hlsl"))
	{
		if (FAILED(res->Load()))			return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_LensFlare", "./ShaderFiles/PostProcess/CS_PostProcess.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "CSMain_LensFlare", .sTarget = "cs_5_0" })))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_Volumetric_LightIntegration", "./ShaderFiles/RayMarching/CS_Volumetric.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "CSMain_LightIntegration", .sTarget = "cs_5_0" })))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_Volumetric_FroxelZAccumulation", "./ShaderFiles/RayMarching/CS_Volumetric.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "CSMain_FroxelZAccumulation", .sTarget = "cs_5_0" })))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_TemporalBlend", "./ShaderFiles/RayMarching/CS_Volumetric.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "CSMain_TemporalBlend", .sTarget = "cs_5_0" })))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_Volumetric_RayMarching", "./ShaderFiles/RayMarching/CS_Volumetric.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "CSMain_RayMarching", .sTarget = "cs_5_0" })))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_Volumetric_Composite", "./ShaderFiles/RayMarching/VS_Volumetric.hlsl"))
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}

	if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_Volumetric_Composite", "./ShaderFiles/RayMarching/PS_Volumetric.hlsl"))
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}

	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_VolumetricCloud", "./ShaderFiles/RayMarching/CS_VolumetricCloud.hlsl"))
	{
		if (FAILED(res->Load()))    return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_VolumetricCloud_TAA", "./ShaderFiles/RayMarching/CS_VolumetricCloud.hlsl"))
	{
		if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "CSMain_CloudTAA", .sTarget = "cs_5_0" })))    return E_FAIL;
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

	if (nullptr == m_pBackBufferDSV)		{ MSG_BOX("Invalid : m_pBackBufferDSV");        return E_FAIL; }
	if (nullptr == m_pBackBufferRTV)		{ MSG_BOX("Invalid : m_pBackBufferRTV");        return E_FAIL; }
	if (nullptr == m_pBackBufferViewPort)	{ MSG_BOX("Invalid : m_pBackBufferViewPort");   return E_FAIL; }
	if (nullptr == m_pBackBufferTexture)	{ MSG_BOX("Invalid : m_pBackBufferTexture");    return E_FAIL; }

	// m_pRasterizer Setting - BackCull
	m_pRasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_BACKCULL);
	m_pContext->RSSetState(m_pRasterizer->GetRasterizerState().Get());

	return S_OK;
}

HRESULT CRenderer::InitializeFullscreen()
{
	if (auto m_pFullscreenVIBuffer = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_FullscreenTex", E::CResQuadFullscreenTexBuffer::Create()))
	{
		if (FAILED(m_pFullscreenVIBuffer->Load()))    return E_FAIL;
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
	if (nullptr == m_pResDynTexTargetDepth)         return E_FAIL;
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		m_pResDynTexTargetDepth->GetDSV()->GetDesc(&dsvDesc);
		dsvDesc.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;
		if (FAILED(m_pDevice->CreateDepthStencilView(m_pResDynTexTargetDepth->GetTexture().Get(), &dsvDesc, m_pDecalReadOnlyDSV.GetAddressOf())))
			return E_FAIL;
	}

	m_pResDynTexTargetFocusingDepthMap = Generate_DepthStencil_RenderTarget("DynTex2D_Target_FocusingDepth", DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
	if (nullptr == m_pResDynTexTargetFocusingDepthMap)        return E_FAIL;

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

	if (m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim"))
	{
		if (nullptr == m_pResVertexShader)	return E_FAIL;
	}
	if (m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim"))
	{
		if (nullptr == m_pResPixelShader)	return E_FAIL;
	}

	if (FAILED(CreateDDSTextureFromFile(m_pDevice.Get(), L"./Resources/Engine/Texture/DefaultTexture/DefaultTex_BRDFLUT.dds", nullptr, m_pSRVBRDFLookUpMap.GetAddressOf()))) 			return E_FAIL;
	if (FAILED(CreateDDSTextureFromFile(m_pDevice.Get(), L"./Resources/Engine/Texture/DefaultTexture/DefaultTex_Irridiance.dds"	 , nullptr, m_pSRVIrradianceMap.GetAddressOf()))) 		return E_FAIL;
	if (FAILED(CreateDDSTextureFromFile(m_pDevice.Get(), L"./Resources/Engine/Texture/DefaultTexture/DefaultTex_PreFilterMap.dds", nullptr, m_pSRVPreFilteredMap.GetAddressOf()))) 		return E_FAIL;

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

	// LUT Texture Create
	if (FAILED(CreateDDSTextureFromFile(m_pDevice.Get(), L"./Resources/Engine/Texture/PostProcess/LUT_Fuji.dds", nullptr, m_pLookUpTableTexture.GetAddressOf()))) {
		MSG_BOX("Cannot Create LUT Texture File.");
		return E_FAIL;
	}
	{
		m_pMotionBlurPixelShader	= E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PostProcess_MotionBlur");
		if (nullptr == m_pMotionBlurPixelShader)		return E_FAIL;

		m_pLensFlareComputeShader	= E::CGameInstance::Get().GetResourceFirst<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_LensFlare");
		if (nullptr == m_pLensFlareComputeShader)		return E_FAIL;

		m_pPostProcessComputeShader = E::CGameInstance::Get().GetResourceFirst<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PostProcess_Filter");
		if (nullptr == m_pPostProcessComputeShader)		return E_FAIL;
	}
	{
		m_pLensFlareCBuffer = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_LENSFLARE", E::CResCBuffer::Create());
		if (nullptr == m_pLensFlareCBuffer || FAILED(m_pLensFlareCBuffer->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_LENSFLARE) })))    return E_FAIL;

		m_pPostProcessCBuffer = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_POSTPROCESS", E::CResCBuffer::Create());
		if (nullptr == m_pPostProcessCBuffer || FAILED(m_pPostProcessCBuffer->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_POSTPROCESS) })))    return E_FAIL;
	}
	{
		m_pResDynTexTargetMotionBlur = Generate_RenderTarget("DynTex2D_PostProcess_MotionBlur", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
		if (nullptr == m_pResDynTexTargetMotionBlur)	return E_FAIL;

		m_pResDynTexTargetLensFlare = Generate_UnorderedAccessView("UAV_LensFlare", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE);
		if (nullptr == m_pResDynTexTargetLensFlare)		return E_FAIL;

		m_pResDynTexTargetPostProcess = Generate_UnorderedAccessView("UAV_PostProcess", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE);
		if (nullptr == m_pResDynTexTargetPostProcess)	return E_FAIL;
	}
	{	// LENSFLARE Value Initialize
		m_fScreenPosition = { 0.5f, 0.5f };
		m_fExpandDuration = 10.f;
		m_fCurrentLifeTime = 0.f;
		m_fChromaticRingAlpha = 0.25f;
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
	_float2 ScreenSize = CGameInstance::Get().GetClientScreenSize();

	const uint32_t FullScreenWidth		= static_cast<uint32_t>(ScreenSize.x)	, FullScreenHeight		= static_cast<uint32_t>(ScreenSize.y);
	const uint32_t HalfScreenWidth		= FullScreenWidth / 2					, HalfScreenHeight		= FullScreenHeight / 2;
	const uint32_t QuarterScreenWidth	= HalfScreenWidth / 2					, QuarterScreenHeight	= HalfScreenHeight / 2;

	m_pResDynTexTargetFinalResult		= Generate_UnorderedAccessView("UAV_FinalResult", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE, FullScreenWidth, FullScreenHeight);
	m_pResDynTexTargetBloomResult		= Generate_UnorderedAccessView("UAV_BloomResult", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE, FullScreenWidth, FullScreenHeight);
	
	m_pResDynTexTargetBloom_HalfScaleA	= Generate_UnorderedAccessView("UAV_BloomHalfA"	, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE, HalfScreenWidth, HalfScreenHeight);
	m_pResDynTexTargetBloom_HalfScaleB	= Generate_UnorderedAccessView("UAV_BloomHalfB"	, DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE, HalfScreenWidth, HalfScreenHeight);
	
	m_pResDynTexTargetBloom_QuarterScaleA = Generate_UnorderedAccessView("UAV_BloomQuarterA", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE, QuarterScreenWidth, QuarterScreenHeight);
	m_pResDynTexTargetBloom_QuarterScaleB = Generate_UnorderedAccessView("UAV_BloomQuarterB", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE, QuarterScreenWidth, QuarterScreenHeight);

	if (!m_pResDynTexTargetBloomResult || !m_pResDynTexTargetFinalResult || 
		!m_pResDynTexTargetBloom_HalfScaleA || !m_pResDynTexTargetBloom_HalfScaleB ||
		!m_pResDynTexTargetBloom_QuarterScaleA || !m_pResDynTexTargetBloom_QuarterScaleB)	return E_FAIL;

	m_pBrightPassComputeShader			= E::CGameInstance::Get().GetResourceFirst<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PostProcess_Bloom_BrightPass");
	m_pVerticalBlurComputeShader		= E::CGameInstance::Get().GetResourceFirst<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PostProcess_Bloom_VerticalBlur");
	m_pHorizontalBlurComputeShader		= E::CGameInstance::Get().GetResourceFirst<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PostProcess_Bloom_HorizontalBlur");
	m_pBloomPassComputeShader			= E::CGameInstance::Get().GetResourceFirst<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PostProcess_Bloom_Combined");
	m_pUpSampleComputeShader			= E::CGameInstance::Get().GetResourceFirst<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PostProcess_Bloom_UpSampling");
	m_pDownSampleComputeShader			= E::CGameInstance::Get().GetResourceFirst<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PostProcess_Bloom_DownSampling");
	m_pRadialBlurComputeShader			= E::CGameInstance::Get().GetResourceFirst<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_PostProcess_Bloom_RadialBlur");

	if (!m_pBrightPassComputeShader || !m_pVerticalBlurComputeShader || !m_pHorizontalBlurComputeShader || !m_pBloomPassComputeShader || 
		!m_pUpSampleComputeShader || !m_pDownSampleComputeShader || !m_pRadialBlurComputeShader)	return E_FAIL;

	m_pBloomCBuffer = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_Bloom", E::CResCBuffer::Create());
	if (nullptr == m_pBloomCBuffer || FAILED(m_pBloomCBuffer->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_BLOOM) })))    return E_FAIL;

	return S_OK;
}

HRESULT CRenderer::InitializeVolumetricEffect() {
	_float2 ScreenSize = CGameInstance::Get().GetClientScreenSize();

	m_pResDynTexTargetVolumetric = Generate_RenderTarget("DynTex2D_Volumetric", DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	m_pVolumetricCloudTex		 = Generate_UnorderedAccessView("UAV_VolumetricCloudMain", DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE, ScreenSize.x, ScreenSize.y);
	m_pVolumetricCloudTAATex[0]  = Generate_UnorderedAccessView("UAV_VolumetricCloudTAA00", DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE, ScreenSize.x, ScreenSize.y);
	m_pVolumetricCloudTAATex[1]  = Generate_UnorderedAccessView("UAV_VolumetricCloudTAA01", DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE, ScreenSize.x, ScreenSize.y);

	if (!m_pResDynTexTargetVolumetric || !m_pVolumetricCloudTex || !m_pVolumetricCloudTAATex[0] || !m_pVolumetricCloudTAATex[1]) return E_FAIL;

	if (FAILED(CreateDDSTextureFromFile(m_pDevice.Get(), L"./Resources/Engine/Texture/DefaultTexture/BlueNoiseTexture.dds", nullptr, m_pBlueNoiseTexture.GetAddressOf()))) {
		MSG_BOX("Cannot Create BlueNoise Texture File.");
		return E_FAIL;
	}

	if (FAILED(CreateDDSTextureFromFile(m_pDevice.Get(), L"./Resources/Engine/Texture/DefaultTexture/VolumeTexture/CloudNoise_Volume.dds", nullptr, m_pVolumeTexture.GetAddressOf()))) {
		MSG_BOX("Cannot Create Volume Texture File.");
		return E_FAIL;
	}

	if (FAILED(CreateDDSTextureFromFile(m_pDevice.Get(), L"./Resources/Engine/Texture/DefaultTexture/VolumeTexture/cumulus.dds", nullptr, m_pWeatherMapTexture.GetAddressOf()))) {
		MSG_BOX("Cannot Create WeatherMap Texture File.");
		return E_FAIL;
	}

	if (FAILED(CreateDDSTextureFromFile(m_pDevice.Get(), L"./Resources/Engine/Texture/DefaultTexture/VolumeTexture/CurlNoise.dds", nullptr, m_pCloudCurlNoiseTexture.GetAddressOf()))) {
		MSG_BOX("Cannot Create Cloud CurlNoise File.");
		return E_FAIL;
	}
	
	if (m_pVolumetricFroxelCBuffer = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_FROXEL", E::CResCBuffer::Create())) {
		if (FAILED(m_pVolumetricFroxelCBuffer->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_FROXEL) })))    return E_FAIL;
	}
	if (m_pVolumetricVFogCBuffer = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_VLFOG", E::CResCBuffer::Create())) {
		if (FAILED(m_pVolumetricVFogCBuffer->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_VLFOG) })))    return E_FAIL;
	}

	if (m_pVolumetricCSMCBuffer = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_CSM", E::CResCBuffer::Create())) {
		if (FAILED(m_pVolumetricCSMCBuffer->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_CSM) })))    return E_FAIL;
	}
	if (m_pVolumetricCloudCBuffer = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_VOLUMECLOUD", E::CResCBuffer::Create())) {
		if (FAILED(m_pVolumetricCloudCBuffer->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_VOLUMECLOUD) })))    return E_FAIL;
	}
	if (m_pVolumetricCloudTAACBuffer = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_CLOUDTAA", E::CResCBuffer::Create())) {
		if (FAILED(m_pVolumetricCloudTAACBuffer->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_CLOUDTAA) })))    return E_FAIL;
	}
	
	m_pLightIntegrationCS	 = CGameInstance::Get().GetResourceFirst<CResComputeShader>	(TAG_RES_GRP_PERMANENT_SHADER, "CS_Volumetric_LightIntegration");
	m_pFroxelAccumulationCS  = CGameInstance::Get().GetResourceFirst<CResComputeShader> (TAG_RES_GRP_PERMANENT_SHADER, "CS_Volumetric_FroxelZAccumulation");
	m_pTemporalBlendedCS	 = CGameInstance::Get().GetResourceFirst<CResComputeShader> (TAG_RES_GRP_PERMANENT_SHADER, "CS_TemporalBlend");

	m_pVolumetricCompositeVS = CGameInstance::Get().GetResourceFirst<CResVertexShader>	(TAG_RES_GRP_PERMANENT_SHADER, "VS_Volumetric_Composite");
	m_pVolumetricCompositePS = CGameInstance::Get().GetResourceFirst<CResPixelShader>	(TAG_RES_GRP_PERMANENT_SHADER, "PS_Volumetric_Composite");

	m_pVolumetricCloudCS	 = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_VolumetricCloud");
	m_pVolumetricCloudTAACS  = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_VolumetricCloud_TAA");

	if (!m_pLightIntegrationCS || !m_pFroxelAccumulationCS || !m_pTemporalBlendedCS || !m_pVolumetricCompositeVS || 
		!m_pVolumetricCompositePS || !m_pVolumetricCloudCS || !m_pVolumetricCloudTAACS)		return E_FAIL;

	m_pVoxelLighting		= Generate_Texture3D(DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, FROXELX, FROXELY, FROXELZ);
	m_pVoxelAccumulated		= Generate_Texture3D(DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, FROXELX, FROXELY, FROXELZ + 1);
	m_pBlendedVolumeTex		= Generate_Texture3D(DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, FROXELX, FROXELY, FROXELZ);
	m_pPreviousVolumeTex	= Generate_Texture3D(DXGI_FORMAT_R16G16B16A16_FLOAT, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS, FROXELX, FROXELY, FROXELZ);
	
	{
		m_pFogInfo.g_fFogColor = _float3(0.8f, 0.85f, 0.9f);
		m_pFogInfo.g_fFogIntensity = 1.f;
		m_pFogInfo.g_fFogDensity = 0.02f;
		m_pFogInfo.g_fFogNoiseScale = 0.05f;
		m_pFogInfo.g_fFogScattering = 0.5f;
		m_pFogInfo.g_fFogBaseBrightness = 0.1f;

		m_pFogInfo.g_fFogLightColor = _float3(1.f, 0.9f, 0.7f);
		m_pFogInfo.g_fFogLightDirection = _float3(0.577f, -0.577f, 0.577f);

		m_pFogInfo.g_fFogBaseHeight = 0.f;
		m_pFogInfo.g_fFogMaxHeight = 20.f;
		m_pFogInfo.g_fFogHeightFallOff = 0.05f;

		m_pFogInfo.g_fFogStartDistance = 0.f;
		m_pFogInfo.g_fFogEndDistance = 200.f;

		m_pFogInfo.g_fFogTime = 0.f;
	}
	
	{
		m_pCloudInfo.g_fWindDirection = XMFLOAT3(0.1f, 0.f, 0.05f);
		m_pCloudInfo.g_fWindTimeAccumulation = 0.f;

		m_pCloudInfo.g_fCloudColor = XMFLOAT3(0.9f, 0.9f, 1.f);
		m_pCloudInfo.g_fCloudBrightness = 1.2f;

		m_pCloudInfo.g_fCloudCoverage = 0.55f;
		m_pCloudInfo.g_fCloudDensity = 1.f;
		m_pCloudInfo.g_fCloudScattering = 0.8f;

		m_pCloudInfo.g_fBaseCloudNoiseScale = 0.0003f;
		m_pCloudInfo.g_fDetailCloudNoiseScale = 0.0015f;

		m_pCloudInfo.g_fCloudMinHeight = 500.f;
		m_pCloudInfo.g_fCloudMaxHeight = 2500.f;
		m_pCloudInfo.g_fCloudLODDistance = 50000.f;

		m_pCloudInfo.g_fCloudLightDirection = XMFLOAT3(0.0f, -1.0f, 0.0f);
		m_pCloudInfo.g_fLightAbsorption = 0.015f;
	}

	{
		m_pCloudTAAInfo.g_fScreenResolution = ScreenSize;
		m_pCloudTAAInfo.g_fInvScreenResolution = _float2(1.f / ScreenSize.x, 1.f / ScreenSize.y);
		m_pCloudTAAInfo.g_mCloudJitterInvProj = XMMatrixIdentity();
		m_pCloudTAAInfo.g_mCloudPrevViewProj = XMMatrixIdentity();
	}

	return S_OK;
}

HRESULT CRenderer::InitializeUI3D()
{
	m_pUI3DVertexShader = CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "VS_UI3D");
	if (!m_pUI3DVertexShader)
		return E_FAIL;

	m_pUI3DPixelShader = CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "PS_UI3D");
	if (!m_pUI3DPixelShader)
		return E_FAIL;

	m_pResDynTexTargetUI3D = Generate_RenderTarget(
		"DynTex2D_UI3D", DXGI_FORMAT_R16G16B16A16_FLOAT,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	if (!m_pResDynTexTargetUI3D)
		return E_FAIL;

	// UAV_PostProcess has no RTV, so world-panel composition needs an
	// RTV-capable copy of the post-process result.
	m_pResDynTexTargetUI3DComposite = Generate_RenderTarget(
		"DynTex2D_UI3DComposite", DXGI_FORMAT_R16G16B16A16_FLOAT,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	if (!m_pResDynTexTargetUI3DComposite)
		return E_FAIL;

	return S_OK;
}

VOID CRenderer::SetUI3DPanel(const _float4x4& worldMatrix, _bool active, _bool ignoreDepth)
{
	XMStoreFloat4x4(&m_UI3DPanelWorld, XMMatrixIdentity());
	m_UI3DPanelWorld = worldMatrix;
	m_bUI3DPanelActive = active;
	m_bUI3DPanelIgnoreDepth = ignoreDepth;
}

VOID CRenderer::ClearUI3DPanel()
{
	m_bUI3DPanelActive = false;
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
	Tex2dDesc.ArraySize = (_LTYPE == ETOUI(LIGHT_TYPE::POINT)) ? POINT_SHADOW_MAPCOUNT : 1;
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

SPtr<CResViewPort> CRenderer::Generate_ViewPort(const StringID& _sResTag, uint32_t _TexWidth, uint32_t _TexHeight) {
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

HRESULT CRenderer::Generate_ShadowCubeMap(ID3D11DepthStencilView** _ShadowDSV, ID3D11Texture2D** _TextureArray, ID3D11ShaderResourceView** _SRV, uint32_t _Resolution, uint32_t _MaxLightCount) {
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

HRESULT CRenderer::Generate_ShadowTexture(ID3D11DepthStencilView** _ShadowDSV, ID3D11Texture2D** _Texture, ID3D11ShaderResourceView** _SRV, uint32_t _ResolutionX, uint32_t _ResolutionY) {
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

TEXTURE3D CRenderer::Generate_Texture3D(DXGI_FORMAT _TexFormat, uint32_t _BindFlags, uint32_t _TexWidth, uint32_t _TexHeight, uint32_t _TexDepth) {

	TEXTURE3D	VTEX3D = {};

	D3D11_TEXTURE3D_DESC Tex3DDesc = {};
	Tex3DDesc.Width = _TexWidth;
	Tex3DDesc.Height = _TexHeight;
	Tex3DDesc.Depth = _TexDepth;
	Tex3DDesc.MipLevels = 1;
	Tex3DDesc.Format = _TexFormat;
	Tex3DDesc.Usage = D3D11_USAGE_DEFAULT;
	Tex3DDesc.BindFlags = _BindFlags;
	Tex3DDesc.CPUAccessFlags = 0;
	Tex3DDesc.MiscFlags = 0;

	if (FAILED(m_pDevice->CreateTexture3D(&Tex3DDesc, nullptr, VTEX3D.pTexture.GetAddressOf())))	return TEXTURE3D{};

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
	SRVDesc.Format = _TexFormat;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	SRVDesc.Texture3D.MipLevels = 1;
	SRVDesc.Texture3D.MostDetailedMip = 0;

	if (FAILED(m_pDevice->CreateShaderResourceView(VTEX3D.pTexture.Get(), &SRVDesc, VTEX3D.pSRV.GetAddressOf())))	return TEXTURE3D{};

	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc{};
	UAVDesc.Format = _TexFormat;
	UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
	UAVDesc.Texture3D.MipSlice = 0;
	UAVDesc.Texture3D.FirstWSlice = 0;
	UAVDesc.Texture3D.WSize = _TexDepth;

	if (FAILED(m_pDevice->CreateUnorderedAccessView(VTEX3D.pTexture.Get(), &UAVDesc, VTEX3D.pUAV.GetAddressOf())))	return TEXTURE3D{};

	return VTEX3D;
}

HRESULT CRenderer::AddRenderObject(RENDERGROUP eRenderGroup, IRenderable* pRenderObject)
{
	if (eRenderGroup >= RENDERGROUP::END || nullptr == pRenderObject)
		return E_FAIL;

	m_pRenderObject[ETOUI(eRenderGroup)].push_back(pRenderObject);

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

VOID	CRenderer::Unbind_Resources() {
	// UnBind RenderTargets / ShaderResource / Shader
	ID3D11RenderTargetView* pNullRTVs[8] = { nullptr };
	m_pContext->OMSetRenderTargets(8, pNullRTVs, nullptr);

	ID3D11UnorderedAccessView* pNullUAVs[8] = { nullptr };
	m_pContext->CSSetUnorderedAccessViews(0, 8, pNullUAVs, nullptr);

	ID3D11ShaderResourceView* pNullSRVs[12] = { nullptr };
	m_pContext->PSSetShaderResources(0, 12, pNullSRVs);
	m_pContext->VSSetShaderResources(0, 12, pNullSRVs);
	m_pContext->CSSetShaderResources(0, 12, pNullSRVs);

	m_pContext->IASetInputLayout(nullptr);
	m_pContext->GSSetShader(nullptr, nullptr, 0);
	m_pContext->PSSetShader(nullptr, nullptr, 0);
	m_pContext->CSSetShader(nullptr, nullptr, 0);
}

HRESULT CRenderer::Bind_CameraAttribute(CCameraObject* _ActiveCam) {
	auto pCbPerPass = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PASS);
	D3D11_MAPPED_SUBRESOURCE mappedSubResource;
	const XMMATRIX currentViewProj = _ActiveCam->GetView() * _ActiveCam->GetProj();

	if (m_pRenderContext.pass == RENDERPASS::SHADOW) {
		m_mShadowLightViewProj = currentViewProj;
	}
	if (SUCCEEDED(m_pContext->Map(pCbPerPass->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
	{
		CB_PER_PASS cbPerPass{};
		XMStoreFloat4x4(&cbPerPass.matView, _ActiveCam->GetView());
		XMStoreFloat4x4(&cbPerPass.matProj, _ActiveCam->GetProj());

		XMStoreFloat4x4(&cbPerPass.matViewProj, currentViewProj);
		cbPerPass.matPrevViewProj = cbPerPass.matViewProj;

		// Bind_CameraAttribute는 그림자/UI 카메라에도 호출된다.
		// 모션 블러 히스토리는 실제 게임 화면을 렌더한 활성 카메라만 추적한다.
		if (_ActiveCam == CGameInstance::Get().GetActiveCamera())
		{
			m_pCurrentCamera = _ActiveCam;
			m_matCurrentViewProj = cbPerPass.matViewProj;

			if (!m_bHasPreviousViewProj || m_pPreviousCamera != _ActiveCam)
				m_matPrevViewProj = m_matCurrentViewProj;

			cbPerPass.matPrevViewProj = m_matPrevViewProj;
		}

		XMStoreFloat4x4(&cbPerPass.matInvView, XMMatrixInverse(nullptr, _ActiveCam->GetView()));
		XMStoreFloat4x4(&cbPerPass.matInvProj, XMMatrixInverse(nullptr, _ActiveCam->GetProj()));

		XMStoreFloat4x4(&cbPerPass.matInvViewProj, XMMatrixMultiply(XMLoadFloat4x4(&cbPerPass.matInvProj), XMLoadFloat4x4(&cbPerPass.matInvView)));

		cbPerPass.vCamPos = _ActiveCam->GetTransform().GetPosition();
		cbPerPass.fDeltaTime = m_fDeltaTime;
		cbPerPass.fTimeAccumulation = m_fTimeAccumulation;

		XMStoreFloat4x4(&cbPerPass.matShadowLightViewProj, m_mShadowLightViewProj);

		memcpy(mappedSubResource.pData, &cbPerPass, sizeof(cbPerPass));
		m_pContext->Unmap(pCbPerPass->GetCBuffer().Get(), 0);
	}

	m_pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_PASS), 1, pCbPerPass->GetCBuffer().GetAddressOf());
	m_pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_PASS), 1, pCbPerPass->GetCBuffer().GetAddressOf());
	m_pContext->CSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_PASS), 1, pCbPerPass->GetCBuffer().GetAddressOf());

	return S_OK;
}

HRESULT CRenderer::Reset_RenderContext(RENDERPASS _Pass, CCameraObject* _ActiveCam) {
	if (_ActiveCam == nullptr) return E_FAIL;

	m_pRenderContext.pass = _Pass;
	m_pRenderContext.matProj = _ActiveCam->GetProj();
	m_pRenderContext.matView = _ActiveCam->GetView();
	m_pRenderContext.matViewProj = m_pRenderContext.matView * m_pRenderContext.matProj;
	m_pRenderContext.eye = _ActiveCam->GetTransform().GetLoadedPostion();

	return S_OK;
}

#pragma endregion

#pragma region  RENDERING
HRESULT CRenderer::Draw() {
	ZoneScopedN("Renderer : Draw");
	// Resolve the previous frame's timestamp queries before opening this frame's GPU zone.
	TracyD3D11Collect(m_pTracyGpuContext);
	TracyD3D11Zone(m_pTracyGpuContext, "Renderer Frame");

	// m_pRasterizer Setting - BackCull
	m_pContext->RSSetState(m_pRasterizer->GetRasterizerState().Get());

	if (FAILED(Render_Shadow()))		 return E_FAIL;

	// DepthMap
	if (FAILED(Render_DepthMap()))       return E_FAIL;

	// Diffuse + Normal + SMRO + Emissive
	if (FAILED(Render_NonAlpha()))       return E_FAIL;

	// Opaque G-buffer projection decals
	if (FAILED(Render_Decal()))			 return E_FAIL;

	// Hi-Z build: opaque depth 기반
	if (FAILED(BuildCurrentHizBuffer())) return E_FAIL;

	// HBAO
	if (FAILED(Render_HBAO()))			 return E_FAIL;

	// PBR Lighting
	if (FAILED(Render_Lighting()))       return E_FAIL;

	// Trensparent + PBR
	if (FAILED(Render_Alpha()))          return E_FAIL;



	// Volumetric
	if (FAILED(Render_VolumetricEffect())) return E_FAIL;

	// Effect
	if (FAILED(Render_Effect()))		 return E_FAIL;


	// PostProcess
	if (FAILED(Render_PostProcess()))    return E_FAIL;

	// UI 3D
	if (FAILED(Render_UI3D()))			 return E_FAIL;

	// UI
	if (FAILED(Render_UserInterface()))  return E_FAIL;

	// FullScreen : Final
	if (FAILED(Render_FullScreen()))     return E_FAIL;

	#ifdef _DEBUG
		// Debugging
		if (FAILED(Render_Debugging()))      return E_FAIL;
	#endif

	return S_OK;
}

VOID CRenderer::FrameEnd()
{
	// 이번 프레임에 실제 렌더한 활성 카메라의 VP를 다음 프레임 히스토리로 확정한다.
	if (m_pCurrentCamera != nullptr)
	{
		m_matPrevViewProj = m_matCurrentViewProj;
		m_pPreviousCamera = m_pCurrentCamera;
		m_bHasPreviousViewProj = true;
	}
	else
	{
		m_pPreviousCamera = nullptr;
		m_bHasPreviousViewProj = false;
	}
	m_pCurrentCamera = nullptr;

	// HizBuffer 교체
	std::swap(m_pCurrentHizBuffer, m_pPrevHizBuffer);
	m_bHasPrevHizBuffer = true;

	for (auto& vecRenderables : m_pRenderObject)
	{
		vecRenderables.clear();
	}
}

HRESULT CRenderer::Render_Shadow() {
	if (!m_bApplyShadow)	return S_OK;
	TracyD3D11Zone(m_pTracyGpuContext, "Shadow");

	if (FAILED(CGameInstance::Get().Capture_ShadowMap()))
	{
		MSG_BOX("Cannot Generate Shadow");
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CRenderer::Render_DepthMap() {
	ZoneScopedN("Render_DepthMap");
	TracyD3D11Zone(m_pTracyGpuContext, "Depth Map");
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

			m_pContext->PSSetShader(nullptr, nullptr, 0);
		}

		auto pGameCam = CGameInstance::Get().GetActiveCamera();
		if (nullptr == pGameCam)    return S_OK;

		if (FAILED(Reset_RenderContext(RENDERPASS::DEPTH, pGameCam))) { Unbind_Resources(); return S_OK; }

		if (FAILED(Bind_CameraAttribute(pGameCam))) { Unbind_Resources(); return S_OK; }

		if (FAILED(RenderNonBlend())) { Unbind_Resources(); return S_OK; }

		{
			ID3D11RenderTargetView* pNullRTVs[1] = { nullptr };
			m_pContext->OMSetRenderTargets(1, pNullRTVs, nullptr);
		}
	}

	return S_OK;
}

HRESULT CRenderer::Render_NonAlpha() {
	ZoneScopedN("Render_NonAlpha");
	TracyD3D11Zone(m_pTracyGpuContext, "Opaque GBuffer");
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

		// Default Texture - Dissolve Noise
		SPtr<CResTexture2D> NoiseTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_NOISE");
		m_pContext->PSSetShaderResources(13, 1, NoiseTexture->GetSRV().GetAddressOf());

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
		if (nullptr == pGameCam) { Unbind_Resources(); return S_OK; }

		if (FAILED(Reset_RenderContext(RENDERPASS::DEFAULT, pGameCam))) { Unbind_Resources(); return S_OK; }

		if (FAILED(Bind_CameraAttribute(pGameCam))) { Unbind_Resources(); return S_OK; }

		if (FAILED(RenderPriority())) { Unbind_Resources(); return S_OK; }

		if (FAILED(RenderNonBlend())) { Unbind_Resources(); return S_OK; }

		if (FAILED(RenderNonBlend_Instanced())) { Unbind_Resources(); return S_OK; }

		if (FAILED(RenderLight())) { Unbind_Resources(); return S_OK; }

		if (FAILED(RenderMapMesh())) { Unbind_Resources(); return S_OK; }
	}

	Unbind_Resources();

	return S_OK;
}

HRESULT CRenderer::Render_Decal()
{
	ZoneScopedN("Render_Decal");
	TracyD3D11Zone(m_pTracyGpuContext, "Decal");

	auto& decals = m_pRenderObject[ETOUI(RENDERGROUP::DECAL)];
	if (decals.empty())
		return S_OK;

	auto& gameInstance = CGameInstance::Get();
	const auto blendState = gameInstance.GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_BLEND");
	const auto noBlendState = gameInstance.GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_BLEND_NONE");
	const auto depthDisabled = gameInstance.GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_NO_DEPTHSTENCIL");
	const auto mapMeshOnly = gameInstance.GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_DECAL_MAPMESH_ONLY");
	const auto frontCull = gameInstance.GetResourceFirst<CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_FRONTCULL);
	const auto backCull = gameInstance.GetResourceFirst<CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_BACKCULL);

	if (!blendState || !noBlendState || !depthDisabled || !mapMeshOnly ||
		!frontCull || !backCull || !m_pDecalReadOnlyDSV)
		return E_FAIL;

	ID3D11RenderTargetView* renderTargets[2] = {
		m_pResDynTexTargetDiffuse->GetRTV().Get(),
		m_pResDynTexTargetEmissive->GetRTV().Get()
	};

	m_pContext->OMSetRenderTargets(2, renderTargets, m_pDecalReadOnlyDSV.Get());
	m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());

	ID3D11ShaderResourceView* gBufferResources[2] = {
		m_pResDynTexTargetDepth->GetSRV().Get(),
		m_pResDynTexTargetNormal->GetSRV().Get()
	};

	m_pContext->PSSetShaderResources(0, 2, gBufferResources);
	m_pContext->OMSetBlendState(blendState->GetBlendState().Get(), nullptr, 0xffffffff);
	m_pContext->OMSetDepthStencilState(mapMeshOnly->GetDepthStencilState().Get(), STENCIL_MASK::DECAL_RECEIVER);
	m_pContext->RSSetState(frontCull->GetRasterizerState().Get());

	const auto RestoreRenderState = [&]()
		{
			Unbind_Resources();
			m_pContext->OMSetBlendState(noBlendState->GetBlendState().Get(), nullptr, 0xffffffff);
			m_pContext->OMSetDepthStencilState(depthDisabled->GetDepthStencilState().Get(), 0);
			m_pContext->RSSetState(backCull->GetRasterizerState().Get());
		};

	auto* activeCamera = gameInstance.GetActiveCamera();
	if (!activeCamera ||
		FAILED(Reset_RenderContext(RENDERPASS::DEFAULT, activeCamera)) ||
		FAILED(Bind_CameraAttribute(activeCamera)))
	{
		RestoreRenderState();
		return E_FAIL;
	}

	for (auto* decal : decals)
	{
		if (!decal || !decal->HasRenderPass(m_pRenderContext.pass))
			continue;

		if (FAILED(decal->Render(m_pContext.Get(), m_pRenderContext)))
		{
			RestoreRenderState();
			return E_FAIL;
		}
	}

	RestoreRenderState();

	return S_OK;
}

HRESULT CRenderer::Render_HBAO() {
	ZoneScopedN("Render_HBAO");
	TracyD3D11Zone(m_pTracyGpuContext, "HBAO");
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
	ZoneScopedN("Render_Lighting");
	TracyD3D11Zone(m_pTracyGpuContext, "Lighting");
	{
		// Default Texture - Dissolve HBAO
		SPtr<CResTexture2D> WhiteResource = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_WHITE");
		m_pContext->PSSetShaderResources(5, 1, WhiteResource->GetSRV().GetAddressOf());
	}

	{
		ComPtr<ID3D11ShaderResourceView> SRVList[] = {
			m_pResDynTexTargetDiffuse->GetSRV(),
			m_pResDynTexTargetNormal->GetSRV(),
			m_pResDynTexTargetSMRO->GetSRV(),
			m_pResDynTexTargetEmissive->GetSRV(),
			m_pResDynTexTargetHBAO->GetSRV(),
			m_pResDynTexTargetDepth->GetSRV(),
			m_pSRVIrradianceMap.Get(),
			m_pSRVPreFilteredMap.Get(),
			m_pSRVBRDFLookUpMap.Get()
		};

		m_pContext->CSSetShaderResources(0, 9, SRVList->GetAddressOf());

		if (m_bApplyShadow) {
			if (FAILED(CGameInstance::Get().Render_ObjectShadow())) { Unbind_Resources(); return S_OK; }
		}
		else {
			if (FAILED(CGameInstance::Get().Render_ObjectNonShadow())) { Unbind_Resources(); return S_OK; }
		}

		Unbind_Resources();

		m_pResDynTexTargetPreviousRenderView = CGameInstance::Get().Get_CombinedResource();
	}

	return S_OK;
}

HRESULT CRenderer::Render_Alpha() {
	m_pContext->RSSetState(m_pRasterizer->GetRasterizerState().Get());

	ZoneScopedN("Render_Alpha");
	TracyD3D11Zone(m_pTracyGpuContext, "Transparent");
	{
		m_pContext->CopyResource(
			m_pResDynTexTargetPBR->GetTexture().Get(),
			m_pResDynTexTargetPreviousRenderView->GetTexture().Get());
	}
	{
		ID3D11RenderTargetView* pRTVs[1] = { m_pResDynTexTargetPBR->GetRTV().Get() };
		m_pContext->OMSetRenderTargets(1, pRTVs, m_pResDynTexTargetDepth->GetDSV().Get());
		m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());

		// m_pRasterizer Setting - NoCull
		m_pRasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
		m_pContext->RSSetState(m_pRasterizer->GetRasterizerState().Get());
	}
	{
		m_pContext->IASetInputLayout(m_pBlendVertexShader->GetInputLayout().Get());
		m_pContext->VSSetShader(m_pBlendVertexShader->GetVertexShader().Get(), nullptr, 0);
		m_pContext->PSSetShader(m_pBlendPixelShader->GetPixelShader().Get(), nullptr, 0);
	}
	{
		auto pGameCam = CGameInstance::Get().GetActiveCamera();
		if (nullptr == pGameCam) { Unbind_Resources(); return S_OK; }

		if (FAILED(Reset_RenderContext(RENDERPASS::DEFAULT, pGameCam))) { Unbind_Resources(); return S_OK; }

		if (FAILED(Bind_CameraAttribute(pGameCam))) { Unbind_Resources(); return S_OK; }

		if (FAILED(RenderBlend())) { Unbind_Resources(); return S_OK; }

		if (FAILED(RenderSkybox())) { Unbind_Resources(); return S_OK; }

		if (FAILED(RenderCollider())) { Unbind_Resources(); return S_OK; }
	}

	Unbind_Resources();

	m_pResDynTexTargetPreviousRenderView = m_pResDynTexTargetPBR;

	// m_pRasterizer Setting - BackCull
	m_pRasterizer = E::CGameInstance::GetConst().GetResourceFirst<E::CResRasterizerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_BACKCULL);
	m_pContext->RSSetState(m_pRasterizer->GetRasterizerState().Get());

	return S_OK;
}

HRESULT CRenderer::Render_Effect()
{
	ZoneScopedN("Render_Effect");
	TracyD3D11Zone(m_pTracyGpuContext, "Effect");
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
		if (nullptr == m_pResDynTexTargetEffect) {
			MSG_BOX("RTVS");
		}
		auto RTV = m_pResDynTexTargetEffect->GetRTV();
		if (nullptr == RTV) {
			MSG_BOX("RTV");
		}
		ID3D11RenderTargetView* pRTVs[1] = { m_pResDynTexTargetEffect->GetRTV().Get() };
		m_pContext->OMSetRenderTargets(1, pRTVs, m_pResDynTexTargetDepth->GetDSV().Get());
		m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());
	}

	auto pGameCam = CGameInstance::Get().GetActiveCamera();
	if (nullptr == pGameCam) { Unbind_Resources(); return S_OK; }

	if (FAILED(Reset_RenderContext(RENDERPASS::DEFAULT, pGameCam))) { Unbind_Resources(); return S_OK; }

	if (FAILED(Bind_CameraAttribute(pGameCam))) { Unbind_Resources(); return S_OK; }

	if (FAILED(RenderEffect())) { Unbind_Resources(); return S_OK; }

	Unbind_Resources();

	m_pResDynTexTargetPreviousRenderView = m_pResDynTexTargetEffect;

	return S_OK;
}

HRESULT CRenderer::Render_VolumetricEffect() {
	if (m_bApplyVolumetricFog == false && m_bApplyVolumetricCloud == false) return S_OK;

	ZoneScopedN("Render_VolumetricEffect");
	TracyD3D11Zone(m_pTracyGpuContext, "Volumetric");

	if (FAILED(Update_VolumetricConstantBuffer())) { Unbind_Resources(); return S_OK; }

	if (m_bApplyVolumetricCloud) {
		if (FAILED(Render_VolumetricCloud())) { Unbind_Resources(); return S_OK; }
	}
	if (m_bApplyVolumetricFog) {
		if (FAILED(Render_LightIntegration()))		{ Unbind_Resources(); return S_OK; }

		if (FAILED(Render_TemporalBlend()))			{ Unbind_Resources(); return S_OK; }

		if (FAILED(Render_FroxelZAccumulation()))	{ Unbind_Resources(); return S_OK; }

		if (FAILED(Render_VolumetricComposite()))	{ Unbind_Resources(); return S_OK; }
	}
	
	return S_OK;
}

HRESULT CRenderer::Update_VolumetricConstantBuffer() {
	_float2 ScreenSize = CGameInstance::Get().GetClientScreenSize();

	static uint32_t FrameIndex = 1;
	FrameIndex = (FrameIndex % 16) + 1;

	_float JitterX = Get_HaltonSequence(FrameIndex, 2) - 0.5f;
	_float JitterY = Get_HaltonSequence(FrameIndex, 3) - 0.5f;
	_float JitterZ = Get_HaltonSequence(FrameIndex, 5) - 0.5f;

	auto ActiveCam = CGameInstance::Get().GetActiveCamera();
	if (nullptr == ActiveCam) return E_FAIL;

	
	if (m_bApplyVolumetricCloud) {
		{
			D3D11_MAPPED_SUBRESOURCE MRES{};
			if (SUCCEEDED(m_pContext->Map(m_pVolumetricCloudTAACBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
			{

				_float ProjJitterX = JitterX * (2.f / ScreenSize.x);
				_float ProjJitterY = JitterY * (2.f / ScreenSize.y);

				_matrix CurrentProj = ActiveCam->GetProj();
				CurrentProj.r[2] = XMVectorSetX(CurrentProj.r[2], XMVectorGetX(CurrentProj.r[2]) + ProjJitterX);
				CurrentProj.r[2] = XMVectorSetY(CurrentProj.r[2], XMVectorGetY(CurrentProj.r[2]) + ProjJitterY);
				XMMATRIX matInvProj = XMMatrixInverse(nullptr, CurrentProj);

				m_pCloudTAAInfo.g_mCloudJitterInvProj = matInvProj;
				m_pCloudTAAInfo.g_mCloudPrevViewProj = m_mPreviousViewMatrix * m_mPreviousProjMatrix;

				m_mPreviousViewMatrix = ActiveCam->GetView();
				m_mPreviousProjMatrix = ActiveCam->GetProj();

				memcpy(MRES.pData, &m_pCloudTAAInfo, sizeof(CB_CLOUDTAA));
				m_pContext->Unmap(m_pVolumetricCloudTAACBuffer->GetCBuffer().Get(), 0);
			}
			m_pContext->CSSetConstantBuffers(9, 1, m_pVolumetricCloudTAACBuffer->GetCBuffer().GetAddressOf());
		}
		{
			D3D11_MAPPED_SUBRESOURCE MRES{};
			if (SUCCEEDED(m_pContext->Map(m_pVolumetricCloudCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES))) {
				CB_VOLUMECLOUD cbVolumeCloud{};

				cbVolumeCloud.g_fWindDirection = m_pCloudInfo.g_fWindDirection;
				cbVolumeCloud.g_fWindTimeAccumulation = m_pCloudInfo.g_fWindTimeAccumulation;

				cbVolumeCloud.g_fCloudColor = m_pCloudInfo.g_fCloudColor;
				cbVolumeCloud.g_fCloudBrightness = m_pCloudInfo.g_fCloudBrightness;

				cbVolumeCloud.g_fCloudCoverage = m_pCloudInfo.g_fCloudCoverage;
				cbVolumeCloud.g_fCloudDensity = m_pCloudInfo.g_fCloudDensity;
				cbVolumeCloud.g_fCloudScattering = m_pCloudInfo.g_fCloudScattering;

				cbVolumeCloud.g_fBaseCloudNoiseScale = m_pCloudInfo.g_fBaseCloudNoiseScale;
				cbVolumeCloud.g_fDetailCloudNoiseScale = m_pCloudInfo.g_fDetailCloudNoiseScale;

				cbVolumeCloud.g_fCloudMinHeight = m_pCloudInfo.g_fCloudMinHeight;
				cbVolumeCloud.g_fCloudMaxHeight = m_pCloudInfo.g_fCloudMaxHeight;
				cbVolumeCloud.g_fCloudLODDistance = m_pCloudInfo.g_fCloudLODDistance;


				_float ProjJitterX = JitterX * (2.f / ScreenSize.x);
				_float ProjJitterY = JitterY * (2.f / ScreenSize.y);
				cbVolumeCloud.g_fCloudJitterOffset = _float3(ProjJitterX, ProjJitterY, 0.f);

				if (auto LightHandle = CGameInstance::Get().Get_MainDirectionalLightData().m_pLightHandle) {
					if (auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value()))
						cbVolumeCloud.g_fCloudLightDirection = LightOBJ->Get_LightDirection();
					else
						cbVolumeCloud.g_fCloudLightDirection = _float3(0.f, -1.f, 0.f);
				}
				cbVolumeCloud.g_fLightAbsorption = m_pCloudInfo.g_fLightAbsorption;

				memcpy(MRES.pData, &cbVolumeCloud, sizeof(CB_VOLUMECLOUD));
				m_pContext->Unmap(m_pVolumetricCloudCBuffer->GetCBuffer().Get(), 0);
			}
			m_pContext->CSSetConstantBuffers(13, 1, m_pVolumetricCloudCBuffer->GetCBuffer().GetAddressOf());
		}
	}
	
	if (m_bApplyVolumetricFog) {
		{
			const _float NearZ = ActiveCam->GetNear();
			const _float FarZ = ActiveCam->GetFar();

			const _float VolumeFarZ = std::min(FarZ, static_cast<_float>(VOLUME_MAXFAR));

			D3D11_MAPPED_SUBRESOURCE MRES{};
			if (SUCCEEDED(m_pContext->Map(m_pVolumetricFroxelCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
			{
				CB_FROXEL cbFroxel{};

				cbFroxel.g_fFroxelGridSize = { FROXELX, FROXELY, FROXELZ };
				cbFroxel.g_fSliceDepthRatio = std::powf(VolumeFarZ / NearZ, 1.f / static_cast<_float>(FROXELZ));

				cbFroxel.g_fFullScreenResolution = { ScreenSize.x, ScreenSize.y };
				cbFroxel.g_fHalfScreenResolution = { ScreenSize.x * 0.5f, ScreenSize.y * 0.5f };

				cbFroxel.g_fNearZ = NearZ;
				cbFroxel.g_fFarZ = VolumeFarZ;
				cbFroxel.g_fAnalyticBlendStart = std::max(NearZ, VolumeFarZ - 40.f);
				cbFroxel.g_fAnalyticBlendEnd = VolumeFarZ;

				cbFroxel.g_fJitterOffset = _float3(JitterX, JitterY, JitterZ);
				XMStoreFloat4x4(&cbFroxel.g_mPreviousViewProj, m_mPreviousCamViewProj);

				memcpy(MRES.pData, &cbFroxel, sizeof(CB_FROXEL));
				m_pContext->Unmap(m_pVolumetricFroxelCBuffer->GetCBuffer().Get(), 0);
			}
			m_pContext->PSSetConstantBuffers(10, 1, m_pVolumetricFroxelCBuffer->GetCBuffer().GetAddressOf());
			m_pContext->CSSetConstantBuffers(10, 1, m_pVolumetricFroxelCBuffer->GetCBuffer().GetAddressOf());
		}
		{
			D3D11_MAPPED_SUBRESOURCE MRES{};
			if (SUCCEEDED(m_pContext->Map(m_pVolumetricVFogCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
			{
				CB_VLFOG cbVLFog = m_pFogInfo;

				if (auto LightHandle = CGameInstance::Get().Get_MainDirectionalLightData().m_pLightHandle) {
					if (auto LightOBJ = CGameInstance::Get().GetGameObjectByHandleT<CLight>(LightHandle.value()))
						cbVLFog.g_fFogLightDirection = LightOBJ->Get_LightDirection();
					else
						cbVLFog.g_fFogLightDirection = _float3(0.f, 0.f, 0.f);
				}

				cbVLFog.g_fFogTime = std::fmod(m_fTimeAccumulation, 4096.f);
				memcpy(MRES.pData, &cbVLFog, sizeof(CB_VLFOG));
				m_pContext->Unmap(m_pVolumetricVFogCBuffer->GetCBuffer().Get(), 0);
			}
			m_pContext->PSSetConstantBuffers(11, 1, m_pVolumetricVFogCBuffer->GetCBuffer().GetAddressOf());
			m_pContext->CSSetConstantBuffers(11, 1, m_pVolumetricVFogCBuffer->GetCBuffer().GetAddressOf());
		}
		{
			CB_CSM cbCSM{};

			const CSM_DATA& CascadeShadowLightData = CGameInstance::Get().Get_MainDirectionalLightData();
			if (m_bApplyShadow && CascadeShadowLightData.m_pLightHandle && CascadeShadowLightData.m_pShadowSRV) {
				m_pCSMShadowMapTexture = CascadeShadowLightData.m_pShadowSRV.Get();
				for (int i = 0; i < 4; ++i)
				{
					cbCSM.g_mShadowViewProj[i] = CGameInstance::Get().Get_CascadeShadowViewProj(i);
				}
				cbCSM.g_fCascadeSplits = CGameInstance::Get().Get_CascadeShadowSplits();
				cbCSM.g_fShadowMapSize = XMFLOAT2(CSM_SHADOW_MAPSIZE, CSM_SHADOW_MAPSIZE);
				cbCSM.g_fShadowBias = XMFLOAT2(0.0005f, 0.0f);
			}
			else  m_pCSMShadowMapTexture = nullptr;

			D3D11_MAPPED_SUBRESOURCE MRES{};
			if (SUCCEEDED(m_pContext->Map(m_pVolumetricCSMCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES))) {
				memcpy(MRES.pData, &cbCSM, sizeof(CB_CSM));
				m_pContext->Unmap(m_pVolumetricCSMCBuffer->GetCBuffer().Get(), 0);
			}
			m_pContext->CSSetConstantBuffers(12, 1, m_pVolumetricCSMCBuffer->GetCBuffer().GetAddressOf());
		}
	}

	return S_OK;
}

HRESULT CRenderer::Render_VolumetricCloud() {
	if (!m_bApplyVolumetricCloud) return S_OK;

	_float2 ScreenSize = CGameInstance::Get().GetClientScreenSize();
	_float	ThreadCount = 16;

	static uint32_t CurrentIndex = 0;
	uint32_t PrevIndex = (CurrentIndex + 1) % 2;

	{
		m_pContext->CSSetShader(m_pVolumetricCloudCS->GetComputeShader().Get(), nullptr, 0);

		ID3D11ShaderResourceView* pSRVs[7] = {
			m_pResDynTexTargetPreviousRenderView->GetSRV().Get(),
			nullptr,
			m_pVolumeTexture.Get(),
			m_pResDynTexTargetDepth->GetSRV().Get(),
			m_pWeatherMapTexture.Get(),
			m_pBlueNoiseTexture.Get(),
			m_pCloudCurlNoiseTexture.Get()
		};
		m_pContext->CSSetShaderResources(0, 7, pSRVs);

		ID3D11UnorderedAccessView* pUAVs[1] = { m_pVolumetricCloudTex->GetUAV().Get() };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pUAVs, nullptr);
	}
	{
		m_pContext->Dispatch((ScreenSize.x + ThreadCount - 1) / ThreadCount, (ScreenSize.y + ThreadCount - 1) / ThreadCount, 1);

		ID3D11ShaderResourceView* pNullSRVs[1] = { nullptr };
		m_pContext->CSSetShaderResources(0, 1, pNullSRVs);

		ID3D11UnorderedAccessView* pNullUAVs[1] = { nullptr };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pNullUAVs, nullptr);
	}

	if (m_bApplyVolumetricCloudTAA) {
		m_pResDynTexTargetPreviousRenderView = m_pVolumetricCloudTex;

		ID3D11ShaderResourceView* pNullSRVs[3] = { nullptr };
		m_pContext->CSSetShaderResources(2, 3, pNullSRVs);

		return S_OK;
	}
	{
		m_pContext->CSSetShader(m_pVolumetricCloudTAACS->GetComputeShader().Get(), nullptr, 0);

		ID3D11ShaderResourceView* pSRVs[2] = {
			m_pVolumetricCloudTex->GetSRV().Get(),
			m_pVolumetricCloudTAATex[PrevIndex]->GetSRV().Get(),
		};
		m_pContext->CSSetShaderResources(0, 2, pSRVs);

		ID3D11UnorderedAccessView* pUAVs[1] = { m_pVolumetricCloudTAATex[CurrentIndex]->GetUAV().Get()};
		m_pContext->CSSetUnorderedAccessViews(0, 1, pUAVs, nullptr);
	}
	{
		m_pContext->Dispatch((ScreenSize.x + ThreadCount - 1) / ThreadCount, (ScreenSize.y + ThreadCount - 1) / ThreadCount, 1);

		ID3D11ShaderResourceView* pNullSRVs[6] = { nullptr };
		m_pContext->CSSetShaderResources(0, 6, pNullSRVs);

		ID3D11UnorderedAccessView* pNullUAVs[1] = { nullptr };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pNullUAVs, nullptr);
	}
	
	m_pResDynTexTargetPreviousRenderView = m_pVolumetricCloudTAATex[CurrentIndex];
	CurrentIndex = PrevIndex;

	return S_OK;
}

HRESULT CRenderer::Render_LightIntegration() {
	{
		m_pContext->CSSetShader(m_pLightIntegrationCS->GetComputeShader().Get(), nullptr, 0);

		CGameInstance::Get().Bind_VolumetricLocalLightResources();

		ID3D11ShaderResourceView* pSRVs[6] = { nullptr };
		pSRVs[1] = m_pBlueNoiseTexture.Get();
		pSRVs[2] = m_pVolumeTexture.Get();
		pSRVs[4] = m_pCSMShadowMapTexture.Get();

		m_pContext->CSSetShaderResources(0, 6, pSRVs);

		ID3D11UnorderedAccessView* pUAVs[1] = { m_pVoxelLighting.pUAV.Get() };
		m_pContext->CSSetUnorderedAccessViews(1, 1, pUAVs, nullptr);
	}
	{
		uint32_t FroxelX = (FROXELX + 7) / 8;
		uint32_t FroxelY = (FROXELY + 7) / 8;
		uint32_t FroxelZ = (FROXELZ + 7) / 8;

		m_pContext->Dispatch(FroxelX, FroxelY, FroxelZ);

		CGameInstance::Get().UnBind_VolumetricLocalLightResources();

		ID3D11ShaderResourceView* pNullSRVs[6] = { nullptr };
		m_pContext->CSSetShaderResources(0, 6, pNullSRVs);

		ID3D11UnorderedAccessView* pNullUAVs[1] = { nullptr };
		m_pContext->CSSetUnorderedAccessViews(1, 1, pNullUAVs, nullptr);
	}

	return S_OK;
}

HRESULT CRenderer::Render_TemporalBlend() {
	{
		m_pContext->CSSetShader(m_pTemporalBlendedCS->GetComputeShader().Get(), nullptr, 0);

		ID3D11ShaderResourceView* pSRVs[2] = { m_pVoxelLighting.pSRV.Get(), m_pPreviousVolumeTex.pSRV.Get() };
		m_pContext->CSSetShaderResources(5, 2, pSRVs);

		ID3D11UnorderedAccessView* pUAVs[1] = { m_pBlendedVolumeTex.pUAV.Get() };
		m_pContext->CSSetUnorderedAccessViews(1, 1, pUAVs, nullptr);
	}
	{
		uint32_t FroxelX = (FROXELX + 7) / 8;
		uint32_t FroxelY = (FROXELY + 7) / 8;
		uint32_t FroxelZ = (FROXELZ + 7) / 8;

		m_pContext->Dispatch(FroxelX, FroxelY, FroxelZ);

		ID3D11ShaderResourceView* pNullSRVs[2] = { nullptr, nullptr };
		m_pContext->CSSetShaderResources(5, 2, pNullSRVs);

		ID3D11UnorderedAccessView* pNullUAVs[1] = { nullptr };
		m_pContext->CSSetUnorderedAccessViews(1, 1, pNullUAVs, nullptr);
	}
	return S_OK;
}

HRESULT CRenderer::Render_FroxelZAccumulation()
{
	{
		m_pContext->CSSetShader(m_pFroxelAccumulationCS->GetComputeShader().Get(), nullptr, 0);

		ID3D11ShaderResourceView* pSRVs[1] = { m_pBlendedVolumeTex.pSRV.Get() };
		m_pContext->CSSetShaderResources(3, 1, pSRVs);

		ID3D11UnorderedAccessView* pUAVs[1] = { m_pVoxelAccumulated.pUAV.Get() };
		m_pContext->CSSetUnorderedAccessViews(1, 1, pUAVs, nullptr);
	}
	{
		uint32_t FroxelX = (FROXELX + 7) / 8;
		uint32_t FroxelY = (FROXELY + 7) / 8;

		m_pContext->Dispatch(FroxelX, FroxelY, 1);

		ID3D11ShaderResourceView* pNullSRVs[1] = { nullptr };
		m_pContext->CSSetShaderResources(3, 1, pNullSRVs);

		ID3D11UnorderedAccessView* pNullUAVs[1] = { nullptr };
		m_pContext->CSSetUnorderedAccessViews(1, 1, pNullUAVs, nullptr);
	}
	return S_OK;
}

HRESULT CRenderer::Render_VolumetricComposite() {
	ID3D11RenderTargetView* pRTVs[1] = { m_pResDynTexTargetVolumetric->GetRTV().Get() };
	m_pContext->OMSetRenderTargets(1, pRTVs, nullptr);
	m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());

	_float4 ClearColor = { 0.f, 0.f, 1.f, 1.f };
	m_pContext->ClearRenderTargetView(pRTVs[0], reinterpret_cast<const _float*>(&ClearColor));

	m_pContext->VSSetShader(m_pVolumetricCompositeVS->GetVertexShader().Get(), nullptr, 0);
	m_pContext->PSSetShader(m_pVolumetricCompositePS->GetPixelShader().Get(), nullptr, 0);

	m_pContext->IASetInputLayout(m_pVolumetricCompositeVS->GetInputLayout().Get());

	ID3D11Buffer* vertexBuffers[] = { m_pFullscreenVIBuffer->GetVertexBuffer().Get() };
	uint32_t strides[] = { m_pFullscreenVIBuffer->GetVertexStride() };
	uint32_t offsets[] = { 0 };

	m_pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	m_pContext->IASetIndexBuffer(m_pFullscreenVIBuffer->GetIndexBuffer().Get(), m_pFullscreenVIBuffer->GetIndexFormat(), 0);
	m_pContext->IASetPrimitiveTopology(m_pFullscreenVIBuffer->GetPrimitiveType());

	// Bind Shader Resource
	{
		ID3D11ShaderResourceView* pSRVs[3] = { nullptr };
		pSRVs[0] = m_pResDynTexTargetDepth->GetSRV().Get();
		pSRVs[1] = m_pResDynTexTargetPreviousRenderView->GetSRV().Get();
		pSRVs[2] = m_pVoxelAccumulated.pSRV.Get();
		//pSRVs[3] = m_pBlueNoiseTexture.Get();
		m_pContext->PSSetShaderResources(0, 3, pSRVs);
	}

	// Draw On OffScreen
	m_pContext->DrawIndexed(m_pFullscreenVIBuffer->GetNumIndices(), 0, 0);

	Unbind_Resources();

	m_pResDynTexTargetPreviousRenderView = m_pResDynTexTargetVolumetric;

	{
		// 이전 프레임 정보 갱신
		std::swap(m_pPreviousVolumeTex, m_pBlendedVolumeTex);

		auto ActiveCam = CGameInstance::Get().GetActiveCamera();
		m_mPreviousCamViewProj = ActiveCam->GetView() * ActiveCam->GetProj();
	}

	return S_OK;
}

HRESULT CRenderer::Render_PostProcess() {
	if (m_bApplyFilter == false) return S_OK;
	TracyD3D11Zone(m_pTracyGpuContext, "Post Process");

	if (FAILED(Update_PostProcessConstantBuffer())) { Unbind_Resources(); return S_OK; }

	// 잠시 봉인
	if (m_bMotionBlurEnabled &&
		FAILED(Render_PostProcess_MotionBlur())) { Unbind_Resources(); return S_OK; }

	if (FAILED(Render_PostProcess_Focusing())) { Unbind_Resources(); return S_OK; }

	if (FAILED(Render_PostProcess_LensFlare())) { Unbind_Resources(); return S_OK; }

	if (FAILED(Render_PostProcess_Bloom())) { Unbind_Resources(); return S_OK; }

	if (FAILED(Render_PostProcess_Filter())) { Unbind_Resources(); return S_OK; }

	return S_OK;
}

HRESULT CRenderer::Render_PostProcess_MotionBlur()
{
	ZoneScopedN("Render_PostProcess_MotionBlur");
	{
		ID3D11RenderTargetView* pRTV = m_pResDynTexTargetMotionBlur->GetRTV().Get();
		m_pContext->OMSetRenderTargets(1, &pRTV, nullptr);
		m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());

		m_pContext->IASetInputLayout(m_pFullscreenVS->GetInputLayout().Get());
		m_pContext->VSSetShader(m_pFullscreenVS->GetVertexShader().Get(), nullptr, 0);
		m_pContext->PSSetShader(m_pMotionBlurPixelShader->GetPixelShader().Get(), nullptr, 0);

		ID3D11Buffer* pVertexBuffer = m_pFullscreenVIBuffer->GetVertexBuffer().Get();
		uint32_t iStride = m_pFullscreenVIBuffer->GetVertexStride();
		uint32_t iOffset = 0;
		m_pContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &iStride, &iOffset);
		m_pContext->IASetIndexBuffer(m_pFullscreenVIBuffer->GetIndexBuffer().Get(), m_pFullscreenVIBuffer->GetIndexFormat(), 0);
		m_pContext->IASetPrimitiveTopology(m_pFullscreenVIBuffer->GetPrimitiveType());

		ID3D11ShaderResourceView* pSRVs[2] = {
			m_pResDynTexTargetPreviousRenderView->GetSRV().Get(),
			m_pResDynTexTargetDepth->GetSRV().Get()
		};
		m_pContext->PSSetShaderResources(0, 2, pSRVs);

		m_pContext->DrawIndexed(m_pFullscreenVIBuffer->GetNumIndices(), 0, 0);

		Unbind_Resources();
		m_pResDynTexTargetPreviousRenderView = m_pResDynTexTargetMotionBlur;
	}

	return S_OK;
}

HRESULT	CRenderer::Update_PostProcessConstantBuffer() {

	D3D11_MAPPED_SUBRESOURCE MRES{};
	if (SUCCEEDED(m_pContext->Map(m_pPostProcessCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
	{
		CB_POSTPROCESS cbPPBuffer = m_pPostProcessBuffer;	// 0.01같은 작은 값에도 Intensity가 너무 많이 먹어서 조정
		cbPPBuffer.g_fBlurIntensity = m_pPostProcessBuffer.g_fBlurIntensity / 100.f;
		cbPPBuffer.g_fChromaticIntensity = m_pPostProcessBuffer.g_fChromaticIntensity / 100.f;
		cbPPBuffer.g_fDistortionIntensity = m_pPostProcessBuffer.g_fDistortionIntensity / 100.f;
		cbPPBuffer.g_fVignetteIntensity = m_pPostProcessBuffer.g_fVignetteIntensity;

		memcpy(MRES.pData, &cbPPBuffer, sizeof(CB_POSTPROCESS));
		m_pContext->Unmap(m_pPostProcessCBuffer->GetCBuffer().Get(), 0);
	}
	ID3D11Buffer* PostProcessBuffer = m_pPostProcessCBuffer->GetCBuffer().Get();
	m_pContext->CSSetConstantBuffers(11, 1, &PostProcessBuffer);

	return S_OK;
}

HRESULT CRenderer::Render_PostProcess_Focusing() {
	ZoneScopedN("Render_PostProcess_FocusingDepth");
	m_pContext->ClearDepthStencilView(m_pResDynTexTargetFocusingDepthMap->GetDSV().Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	if (!m_pOutlineTargetHandle)	return S_OK;

	auto pOutlineObject = CGameInstance::Get().GetGameObjectByHandle(m_pOutlineTargetHandle.value());
	if (nullptr == pOutlineObject || pOutlineObject->GetPendingDestroy())	return S_OK;

	{
		ID3D11DepthStencilState* pDSS = nullptr;
		m_pContext->OMSetDepthStencilState(pDSS, 0);

		SPtr<CResDepthStencilState> DepthWriteState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHWRITE");
		if (nullptr == DepthWriteState) return S_OK;

		m_pContext->OMSetDepthStencilState(DepthWriteState->GetDepthStencilState().Get(), 0);
	}
	{
		ID3D11DepthStencilView* pFocusingDSV = m_pResDynTexTargetFocusingDepthMap->GetDSV().Get();

		m_pContext->OMSetRenderTargets(0, nullptr, pFocusingDSV);
		m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());

		m_pContext->PSSetShader(nullptr, nullptr, 0);
	}

	// [LSY] 이 패스는 별도의 외곽선 Depth DSV와 DS_DEPTHWRITE를 바인딩한다.
	// 성공 여부와 관계없이 다음 PostProcess가 이 상태를 물려받지 않도록 공통으로 해제한다.
	// Unbind_Resources()는 RenderTarget은 해제하지만 DepthStencilState는 복구하지 않으므로
	// DepthStencilState까지 이 함수에서 명시적으로 기본 상태로 되돌린다.
	const auto CleanupFocusingDepthPass = [this]()
	{
		ID3D11RenderTargetView* pNullRTV = nullptr;
		m_pContext->OMSetRenderTargets(1, &pNullRTV, nullptr);
		m_pContext->OMSetDepthStencilState(nullptr, 0);
	};

	{
		auto pGameCam = CGameInstance::Get().GetActiveCamera();
		if (nullptr == pGameCam)
		{
			CleanupFocusingDepthPass();
			return S_OK;
		}

		if (FAILED(Reset_RenderContext(RENDERPASS::DEPTH, pGameCam)))
		{
			Unbind_Resources();
			CleanupFocusingDepthPass();
			return S_OK;
		}

		if (FAILED(Bind_CameraAttribute(pGameCam)))
		{
			Unbind_Resources();
			CleanupFocusingDepthPass();
			return S_OK;
		}

		// [LSY] 기존 외곽선 경로는 CPU Skinning 인스턴스 배치에서 대상 Handle을 찾아
		// 해당 인스턴스 하나만 외곽선 Depth Map에 그린다. 이 경로가 성공(S_OK)하면
		// 기존 동작을 그대로 사용하고 아래 직접 렌더 경로는 실행하지 않는다.
		const HRESULT outlineResult = CGameInstance::Get().Render_OutlineInstance(
			m_pContext.Get(),
			m_pRenderContext,
			m_pOutlineTargetHandle.value());

		if (outlineResult == S_FALSE)
		{
			// [LSY] S_FALSE는 오류가 아니라 대상 Handle이 지원 대상 인스턴스 배치에
			// 없다는 뜻이다. 정적/비인스턴싱 오브젝트도 외곽선을 사용할 수 있도록
			// 오브젝트가 DEPTH 패스를 명시적으로 지원할 때만 같은 Depth Map에 직접 그린다.
			// Render()에는 DEPTH RenderContext가 전달되므로 구현체는 Pixel Shader나
			// 불필요한 Material 바인딩 없이 깊이만 기록해야 한다.
			if (!pOutlineObject->HasRenderPass(RENDERPASS::DEPTH))
			{
				CleanupFocusingDepthPass();
				return S_OK;
			}

			if (FAILED(pOutlineObject->Render(
				m_pContext.Get(), m_pRenderContext)))
			{
				Unbind_Resources();
				CleanupFocusingDepthPass();
				return S_OK;
			}
		}
		else if (FAILED(outlineResult))
		{
			// [LSY] 인스턴싱 배치에서 대상을 찾았지만 실제 Depth 렌더에 실패한 경우다.
			// 이때 직접 렌더로 재시도하면 동일 오브젝트가 일부만 중복 기록될 수 있으므로
			// fallback하지 않고 이번 프레임의 외곽선만 생략한다.
			Unbind_Resources();
			CleanupFocusingDepthPass();
			return S_OK;
		}
	}

	CleanupFocusingDepthPass();


	return S_OK;
}

HRESULT CRenderer::Render_PostProcess_LensFlare() {
	if (m_fCurrentLifeTime >= m_fExpandDuration) return S_OK;
	ZoneScopedN("Render_PostProcess_LensFlare");
	{
		ID3D11RenderTargetView* NullRTV[1] = { nullptr };
		m_pContext->OMSetRenderTargets(1, NullRTV, nullptr);

		m_pContext->CSSetShader(m_pLensFlareComputeShader->GetComputeShader().Get(), nullptr, 0);

		ID3D11UnorderedAccessView* pUAVs[1] = { m_pResDynTexTargetLensFlare->GetUAV().Get() };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pUAVs, nullptr);

		ID3D11ShaderResourceView* pSRVsOrigin[1] = { m_pResDynTexTargetPreviousRenderView->GetSRV().Get() };
		m_pContext->CSSetShaderResources(0, 1, pSRVsOrigin);
	}
	{
		_float2 ScreenSize = CGameInstance::Get().GetClientScreenSize();

		D3D11_MAPPED_SUBRESOURCE MRES{};
		if (SUCCEEDED(m_pContext->Map(m_pLensFlareCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
		{
			CB_LENSFLARE cbLensFlare{};

			cbLensFlare.FlareCenterUV = { m_fScreenPosition.x / ScreenSize.x, m_fScreenPosition.y / ScreenSize.y };
			cbLensFlare.FlareCurrentLifeTime = m_fCurrentLifeTime;
			cbLensFlare.FlareMaxLifeTime = m_fExpandDuration;
			cbLensFlare.RingStartScale = 0.3f;
			cbLensFlare.RingEndScale = m_fRingScale;
			cbLensFlare.AspectRatio = ScreenSize.x / ScreenSize.y;
			cbLensFlare.RingBaseAlpha = m_fChromaticRingAlpha;
			cbLensFlare.RainbowSaturation = 0.5f;
			cbLensFlare.FlareEnabled = 1.f;
			cbLensFlare.TextureSize = { ScreenSize.x, ScreenSize.y };

			memcpy(MRES.pData, &cbLensFlare, sizeof(cbLensFlare));
			m_pContext->Unmap(m_pLensFlareCBuffer->GetCBuffer().Get(), 0);
		}
		ID3D11Buffer* LensFlareBuffer = m_pLensFlareCBuffer->GetCBuffer().Get();
		m_pContext->CSSetConstantBuffers(12, 1, &LensFlareBuffer);

		m_pContext->Dispatch((ScreenSize.x + 15) / 16, (ScreenSize.y + 15) / 16, 1);
	}

	Unbind_Resources();

	m_pResDynTexTargetPreviousRenderView = m_pResDynTexTargetLensFlare;

	return S_OK;
}

HRESULT CRenderer::Render_PostProcess_Bloom() {
	ZoneScopedN("Render_PostProcess_Bloom");
	_float2 ScreenSize = CGameInstance::Get().GetClientScreenSize();

	uint32_t ScreenX = static_cast<uint32_t>(ScreenSize.x);
	uint32_t ScreenY = static_cast<uint32_t>(ScreenSize.y);

	uint32_t HalfScreenX = ScreenX / 2;
	uint32_t HalfScreenY = ScreenY / 2;

	uint32_t QuarterScreenX = ScreenX / 4;
	uint32_t QuarterScreenY = ScreenY / 4;

	{	// FullScale -> HalfScale
		if (FAILED(Update_TexelSize(1.f / ScreenSize.x, 1.f / ScreenSize.y)))																				 return E_FAIL;
		if (FAILED(Render_BrightPass(m_pResDynTexTargetBloom_HalfScaleA, m_pResDynTexTargetPreviousRenderView, HalfScreenX, HalfScreenY)))					 return E_FAIL;
	}

	{	// HalfScale -> QuarterScale
		if (FAILED(Update_TexelSize(1.f / (ScreenSize.x / 2.f), 1.f / (ScreenSize.y / 2.f))))																 return E_FAIL;
		if (FAILED(Render_DownSamplePass(m_pResDynTexTargetBloom_QuarterScaleA, m_pResDynTexTargetBloom_HalfScaleA, QuarterScreenX, QuarterScreenY)))		 return E_FAIL;
	}

	{	// QuarterScale Blur
		if (FAILED(Update_TexelSize(1.f / (ScreenSize.x / 4.f), 1.f / (ScreenSize.y / 4.f))))																 return E_FAIL;
		if (FAILED(Render_VerticalBlurPass(m_pResDynTexTargetBloom_QuarterScaleB, m_pResDynTexTargetBloom_QuarterScaleA, QuarterScreenX, QuarterScreenY)))	 return E_FAIL;
		if (FAILED(Render_HorizontalBlurPass(m_pResDynTexTargetBloom_QuarterScaleA, m_pResDynTexTargetBloom_QuarterScaleB, QuarterScreenX, QuarterScreenY))) return E_FAIL;
	}

	{	// HalfScale Blur
		if (FAILED(Update_TexelSize(1.f / (ScreenSize.x / 2.f), 1.f / (ScreenSize.y / 2.f))))																 return E_FAIL;
		if (FAILED(Render_VerticalBlurPass(m_pResDynTexTargetBloom_HalfScaleB, m_pResDynTexTargetBloom_HalfScaleA, HalfScreenX, HalfScreenY)))				 return E_FAIL;
		if (FAILED(Render_HorizontalBlurPass(m_pResDynTexTargetBloom_HalfScaleA, m_pResDynTexTargetBloom_HalfScaleB, HalfScreenX, HalfScreenY)))			 return E_FAIL;
	}

	{	// Combine
		if (FAILED(Render_UpSampleCombinePass(m_pResDynTexTargetBloom_HalfScaleB, m_pResDynTexTargetBloom_HalfScaleA, m_pResDynTexTargetBloom_QuarterScaleA, HalfScreenX, HalfScreenY)))	return E_FAIL;
		if (FAILED(Render_CombinedPass(m_pResDynTexTargetBloomResult, m_pResDynTexTargetPreviousRenderView, m_pResDynTexTargetBloom_HalfScaleB, ScreenX, ScreenY)))							return E_FAIL;
	}

	{	// Radial Blur
		if (FAILED(Render_RadialBlur(m_pResDynTexTargetFinalResult, m_pResDynTexTargetBloomResult, ScreenX, ScreenY)))								 return E_FAIL;
	}

	m_pResDynTexTargetPreviousRenderView = m_pResDynTexTargetFinalResult;

	Unbind_Resources();

	return S_OK;
}

HRESULT CRenderer::Render_PostProcess_Filter() {
	ZoneScopedN("Render_PostProcess_Filter");
	{
		ID3D11RenderTargetView* NullRTV[1] = { nullptr };
		m_pContext->OMSetRenderTargets(1, NullRTV, nullptr);

		m_pContext->CSSetShader(m_pPostProcessComputeShader->GetComputeShader().Get(), nullptr, 0);

		ID3D11UnorderedAccessView* pUAVs[1] = { m_pResDynTexTargetPostProcess->GetUAV().Get() };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pUAVs, nullptr);

		ID3D11ShaderResourceView* pPostProcessSRVList[5] = {
			m_pResDynTexTargetPreviousRenderView->GetSRV().Get(),
			nullptr,
			m_pLookUpTableTexture.Get(),
			m_pResDynTexTargetFocusingDepthMap->GetSRV().Get(),
			m_pResDynTexTargetDepth->GetSRV().Get()
		};

		m_pContext->CSSetShaderResources(0, 5, pPostProcessSRVList);
	}
	{
		_float2 ScreenSize = CGameInstance::Get().GetClientScreenSize();

		m_pContext->Dispatch((ScreenSize.x + 15) / 16, (ScreenSize.y + 15) / 16, 1);
	}

	Unbind_Resources();

	m_pResDynTexTargetPreviousRenderView = m_pResDynTexTargetPostProcess;

	return S_OK;
}
HRESULT CRenderer::Render_UI3D() {
	ZoneScopedN("Render_UserInterface3D");
	if (!m_bUI3DPanelActive)
		return S_OK;
	TracyD3D11Zone(m_pTracyGpuContext, "UI 3D");

	ComPtr<ID3D11SamplerState> previousLinearClamp;
	m_pContext->PSGetSamplers(1, 1, previousLinearClamp.GetAddressOf());

	ID3D11ShaderResourceView* nullSRV = nullptr;
	m_pContext->PSSetShaderResources(0, 1, &nullSRV);
	ID3D11RenderTargetView* uiRTV = m_pResDynTexTargetUI3D->GetRTV().Get();
	m_pContext->OMSetRenderTargets(1, &uiRTV, nullptr);
	const _float clearColor[4]{ 0.f, 0.f, 0.f, 0.f };
	m_pContext->ClearRenderTargetView(uiRTV, clearColor);
	m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());

	// Ordinary UI shaders sample their textures through LinearClamp (s1).
	// Post-process passes may leave that slot empty, so bind the UI states
	// explicitly before drawing the shop into the transparent RTT.
	auto linearClamp = CGameInstance::Get().GetResourceFirst<CResSamplerState>(
		TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_CLAMP);
	auto noCull = CGameInstance::Get().GetResourceFirst<CResRasterizerState>(
		TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL);
	if (linearClamp)
	{
		ID3D11SamplerState* sampler = linearClamp->GetSamplerState().Get();
		m_pContext->PSSetSamplers(1, 1, &sampler);
	}
	if (noCull)
		m_pContext->RSSetState(noCull->GetRasterizerState().Get());

	auto* uiCamera = CGameInstance::Get().GetCamera("UI");
	if (uiCamera &&
		SUCCEEDED(Reset_RenderContext(RENDERPASS::DEFAULT, uiCamera)) &&
		SUCCEEDED(Bind_CameraAttribute(uiCamera)))
	{
		RenderUI3D();
	}

	// UAV_PostProcess is SRV/UAV-only and GetRTV() is null. Preserve it in the
	// dedicated renderable target before compositing the physical world quad.
	m_pContext->OMSetRenderTargets(0, nullptr, nullptr);
	m_pContext->CopyResource(
		m_pResDynTexTargetUI3DComposite->GetTexture().Get(),
		m_pResDynTexTargetPreviousRenderView->GetTexture().Get());
	ID3D11RenderTargetView* sceneRTV =
		m_pResDynTexTargetUI3DComposite->GetRTV().Get();
	ID3D11DepthStencilView* sceneDSV = m_pResDynTexTargetDepth->GetDSV().Get();
	m_pContext->OMSetRenderTargets(1, &sceneRTV, sceneDSV);
	m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());

	// The RTT contents use the orthographic UI camera, but the physical quad
	// must always be projected by the gameplay camera.  Do not depend on the
	// camera left in the render context by the preceding RTT pass.
	auto* gameCamera = CGameInstance::Get().GetCamera("PlayerCamera");
	if (!gameCamera)
		gameCamera = CGameInstance::Get().GetActiveCamera();
	if (gameCamera &&
		SUCCEEDED(Reset_RenderContext(RENDERPASS::DEFAULT, gameCamera)) &&
		SUCCEEDED(Bind_CameraAttribute(gameCamera)))
	{
		// Use the regular unit quad for a world-space panel.  The fullscreen
		// quad already spans -1..1 clip space and doubles the intended world
		// dimensions when it is transformed by a world matrix.
		const auto& viBuffer = CGameInstance::Get().
			GetResourceFirst<CResQuadTexBuffer>(
				TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");
		if (!viBuffer)
		{
			Unbind_Resources();
			ID3D11SamplerState* restoreSampler = previousLinearClamp.Get();
			m_pContext->PSSetSamplers(1, 1, &restoreSampler);
			return E_FAIL;
		}
		m_pContext->IASetInputLayout(m_pUI3DVertexShader->GetInputLayout().Get());
		m_pContext->VSSetShader(m_pUI3DVertexShader->GetVertexShader().Get(), nullptr, 0);
		m_pContext->PSSetShader(m_pUI3DPixelShader->GetPixelShader().Get(), nullptr, 0);
		ID3D11Buffer* vertexBuffer = viBuffer->GetVertexBuffer().Get();
		UINT stride = viBuffer->GetVertexStride();
		UINT offset = 0;
		m_pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		m_pContext->IASetIndexBuffer(
			viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		m_pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		auto perObjectBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(
			TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerObject");
		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (perObjectBuffer && SUCCEEDED(m_pContext->Map(
			perObjectBuffer->GetCBuffer().Get(), 0,
			D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			CB_PER_OBJECT perObject{};
			const _matrix world = XMLoadFloat4x4(&m_UI3DPanelWorld);
			XMStoreFloat4x4(&perObject.matWorld, world);
			XMStoreFloat4x4(&perObject.matWVP,
				world * gameCamera->GetView() * gameCamera->GetProj());
			memcpy(mapped.pData, &perObject, sizeof(perObject));
			m_pContext->Unmap(perObjectBuffer->GetCBuffer().Get(), 0);
			ID3D11Buffer* objectCB = perObjectBuffer->GetCBuffer().Get();
			m_pContext->VSSetConstantBuffers(0, 1, &objectCB);
		}

		auto perUIBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(
			TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerUI");
		if (perUIBuffer && SUCCEEDED(m_pContext->Map(
			perUIBuffer->GetCBuffer().Get(), 0,
			D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			CB_PER_UI perUI{};
			perUI.color = { 1.f, 1.f, 1.f, 0.8f };
			memcpy(mapped.pData, &perUI, sizeof(perUI));
			m_pContext->Unmap(perUIBuffer->GetCBuffer().Get(), 0);
			ID3D11Buffer* uiCB = perUIBuffer->GetCBuffer().Get();
			m_pContext->VSSetConstantBuffers(7, 1, &uiCB);
			m_pContext->PSSetConstantBuffers(7, 1, &uiCB);
		}

		auto alphaBlend = CGameInstance::Get().GetResourceFirst<CResBlendState>(
			TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_BLEND");
		auto depthState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(
			TAG_RES_GRP_PERMANENT_STATE,
			m_bUI3DPanelIgnoreDepth ? "DS_NO_DEPTHSTENCIL" : "DS_ALPHA_BLEND_DEPTH");
		if (alphaBlend)
			m_pContext->OMSetBlendState(alphaBlend->GetBlendState().Get(), nullptr, 0xffffffff);
		if (noCull)
			m_pContext->RSSetState(noCull->GetRasterizerState().Get());
		if (depthState)
			m_pContext->OMSetDepthStencilState(depthState->GetDepthStencilState().Get(), 0);
		if (linearClamp)
		{
			ID3D11SamplerState* sampler = linearClamp->GetSamplerState().Get();
			m_pContext->PSSetSamplers(1, 1, &sampler);
		}
		ID3D11ShaderResourceView* uiSRV = m_pResDynTexTargetUI3D->GetSRV().Get();
		m_pContext->PSSetShaderResources(0, 1, &uiSRV);
		m_pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}

	Unbind_Resources();
	// Normal screen HUD now consumes the scene containing the world panel.
	m_pResDynTexTargetPreviousRenderView = m_pResDynTexTargetUI3DComposite;
	ID3D11SamplerState* restoreSampler = previousLinearClamp.Get();
	m_pContext->PSSetSamplers(1, 1, &restoreSampler);

	return S_OK;
}
HRESULT CRenderer::Render_UserInterface() {
	auto pUICame = CGameInstance::Get().GetCamera("UI");
	if (nullptr == pUICame) return S_OK;

	m_pRenderContext.matProj = pUICame->GetProj();
	m_pRenderContext.matView = pUICame->GetView();
	m_pRenderContext.matViewProj = m_pRenderContext.matView * m_pRenderContext.matProj;
	m_pRenderContext.eye = pUICame->GetTransform().GetLoadedPostion();

	ZoneScopedN("Render_UserInterface");
	TracyD3D11Zone(m_pTracyGpuContext, "UI");
	{
		{
			m_pContext->CopyResource(
				m_pResDynTexTargetUI->GetTexture().Get(),
				m_pResDynTexTargetPreviousRenderView->GetTexture().Get());
		}
		ID3D11RenderTargetView* pRTVs[1] = { m_pResDynTexTargetUI->GetRTV().Get() };
		m_pContext->OMSetRenderTargets(1, pRTVs, nullptr);
		m_pContext->RSSetViewports(1, &m_pBackBufferViewPort->GetViewPort());

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

		auto UICamera = CGameInstance::Get().GetCamera("UI");

		if (nullptr == UICamera) { Unbind_Resources(); return S_OK; }

		if (FAILED(Reset_RenderContext(RENDERPASS::DEFAULT, UICamera))) { Unbind_Resources(); return S_OK; }

		if (FAILED(Bind_CameraAttribute(UICamera))) { Unbind_Resources(); return S_OK; }

		if (FAILED(RenderUI())) { Unbind_Resources(); return S_OK; }

		Unbind_Resources();

		m_pResDynTexTargetPreviousRenderView = m_pResDynTexTargetUI;
	}

	return S_OK;
}

HRESULT CRenderer::Render_FullScreen()
{
	ZoneScopedN("DrawFullscreen");
	TracyD3D11Zone(m_pTracyGpuContext, "Final Fullscreen");
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
	for (auto& pRenderObject : m_pRenderObject[ETOUI(RENDERGROUP::PRIORITY)])
	{
		if (pRenderObject->HasRenderPass(m_pRenderContext.pass))
		{
			pRenderObject->Render(m_pContext.Get(), m_pRenderContext);
		}
	}

	return S_OK;
}

HRESULT CRenderer::RenderNonBlend() {
	ZoneScopedN("RenderNonBlend");
	for (auto& pRenderObject : m_pRenderObject[ETOUI(RENDERGROUP::NONBLEND)])
	{
		if (pRenderObject->HasRenderPass(m_pRenderContext.pass))
		{
			pRenderObject->Render(m_pContext.Get(), m_pRenderContext);
		}
	}

	return S_OK;
}

HRESULT CRenderer::RenderNonBlend_Instanced() {
	ZoneScopedN("RenderNonBlend_Instanced");


	for (auto& pRenderObject : m_pRenderObject[ETOUI(RENDERGROUP::NONBLEND_INSTANCED)])
	{
		if (pRenderObject->HasRenderPass(m_pRenderContext.pass))
		{
			pRenderObject->Render(m_pContext.Get(), m_pRenderContext);
		}
	}

	return S_OK;

}

HRESULT CRenderer::RenderMapMesh()
{
	ZoneScopedN("RenderMapMesh");

	auto& gameInstance = CGameInstance::Get();
	const auto stencilWrite = gameInstance.GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_MAPMESH_DECAL_WRITE");
	if (!stencilWrite)
		return E_FAIL;

	ComPtr<ID3D11DepthStencilState> previousDepthState{};
	UINT previousStencilRef = 0;
	m_pContext->OMGetDepthStencilState(previousDepthState.GetAddressOf(), &previousStencilRef);
	m_pContext->OMSetDepthStencilState(stencilWrite->GetDepthStencilState().Get(), STENCIL_MASK::DECAL_RECEIVER);

	HRESULT result = S_OK;
	for (auto* renderObject : m_pRenderObject[ETOUI(RENDERGROUP::MAPMESH)])
	{
		if (!renderObject || !renderObject->HasRenderPass(m_pRenderContext.pass))
			continue;

		if (FAILED(renderObject->Render(m_pContext.Get(), m_pRenderContext)))
		{
			result = E_FAIL;
			break;
		}
	}

	m_pContext->OMSetDepthStencilState(previousDepthState.Get(), previousStencilRef);

	return result;
}

HRESULT CRenderer::RenderBlend()
{
	ZoneScopedN("RenderBlend");

	auto BlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_EFFECT");
	m_pContext->OMSetBlendState(BlendState->GetBlendState().Get(), nullptr, 0xffffffff);

	for (auto& pRenderObject : m_pRenderObject[ETOUI(RENDERGROUP::BLEND)])
	{
		if (pRenderObject->HasRenderPass(m_pRenderContext.pass))
		{
			pRenderObject->Render(m_pContext.Get(), m_pRenderContext);
		}
	}

	{
		E::CGameInstance::Get().Render3DFont();
	}

	BlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_BLEND_NONE");
	m_pContext->OMSetBlendState(BlendState->GetBlendState().Get(), nullptr, 0xffffffff);

	return S_OK;
}

HRESULT CRenderer::RenderLight() {
	for (auto& pRenderObject : m_pRenderObject[ETOUI(RENDERGROUP::LIGHT)])
	{
		if (pRenderObject->HasRenderPass(m_pRenderContext.pass))
		{
			pRenderObject->Render(m_pContext.Get(), m_pRenderContext);
		}
	}
	return S_OK;
}

HRESULT CRenderer::RenderSkybox()
{
	ZoneScopedN("RenderSkybox");
	for (auto& pRenderObject : m_pRenderObject[ETOUI(RENDERGROUP::SKYBOX)])
	{
		if (pRenderObject->HasRenderPass(m_pRenderContext.pass))
		{
			pRenderObject->Render(m_pContext.Get(), m_pRenderContext);
		}
	}

	return S_OK;
}

HRESULT CRenderer::RenderEffect()
{
	auto BlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_EFFECT");
	m_pContext->OMSetBlendState(BlendState->GetBlendState().Get(), nullptr, 0xffffffff);


	for (auto& pRenderObject : m_pRenderObject[ETOUI(RENDERGROUP::EFFECT)])
	{
		if (pRenderObject->HasRenderPass(m_pRenderContext.pass))
		{
			pRenderObject->Render(m_pContext.Get(), m_pRenderContext);
		}
	}

	BlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_BLEND_NONE");
	m_pContext->OMSetBlendState(BlendState->GetBlendState().Get(), nullptr, 0xffffffff);


	return S_OK;
}

HRESULT CRenderer::RenderCollider()
{
	ZoneScopedN("RenderCollider");

	for (auto& pRenderObject : m_pRenderObject[ETOUI(RENDERGROUP::COLLIDER)])
	{
		if (pRenderObject->HasRenderPass(m_pRenderContext.pass))
		{
			pRenderObject->Render(m_pContext.Get(), m_pRenderContext);
		}
	}

	return S_OK;
}

HRESULT CRenderer::RenderUI3D() {
	ZoneScopedN("RenderUI3D");
	auto& renderList = m_pRenderObject[ETOUI(RENDERGROUP::UI3D)];
	std::stable_sort(renderList.begin(), renderList.end(),
		[](const IRenderable* lhs, const IRenderable* rhs)
		{
			return static_cast<const CUIObject*>(lhs)->GetWeight() <
				static_cast<const CUIObject*>(rhs)->GetWeight();
		});

	auto Alphablend = E::CGameInstance::Get().GetResourceFirst<E::CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_BLEND");
	m_pContext->OMSetBlendState(Alphablend->GetBlendState().Get(), nullptr, 0xffffffff);
	auto noDepth = E::CGameInstance::Get().GetResourceFirst<E::CResDepthStencilState>(
		TAG_RES_GRP_PERMANENT_STATE, "DS_NO_DEPTHSTENCIL");
	m_pContext->OMSetDepthStencilState(noDepth->GetDepthStencilState().Get(), 0);

	for (auto& pRenderObject : renderList)
	{
		if (pRenderObject->HasRenderPass(m_pRenderContext.pass))
		{
			pRenderObject->Render(m_pContext.Get(), m_pRenderContext);
		}
	}
	E::CGameInstance::Get().FontLateDraw(RENDERGROUP::UI3D);

	auto Nonblend = E::CGameInstance::Get().GetResourceFirst<E::CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_BLEND_NONE");
	m_pContext->OMSetBlendState(Nonblend->GetBlendState().Get(), nullptr, 0xffffffff);

	return S_OK;
}

HRESULT CRenderer::RenderUI()
{
	auto& renderList = m_pRenderObject[ETOUI(RENDERGROUP::UI)];

	constexpr int LATE_UI_WEIGHT = 1000;

	std::stable_sort(
		renderList.begin(),
		renderList.end(),
		[](const IRenderable* lhs, const IRenderable* rhs)
		{
			const auto* l = static_cast<const CUIObject*>(lhs);
			const auto* r = static_cast<const CUIObject*>(rhs);

			return l->GetWeight() < r->GetWeight();
		});

	// 정렬된 목록에서 Weight >= 1000이 시작되는 위치
	const auto lateBegin = std::lower_bound(
		renderList.begin(),
		renderList.end(),
		LATE_UI_WEIGHT,
		[](const IRenderable* pRenderable, int weight)
		{
			const auto* pUI =
				static_cast<const CUIObject*>(pRenderable);

			return pUI->GetWeight() < weight;
		});

	auto noDepth =
		E::CGameInstance::Get()
		.GetResourceFirst<E::CResDepthStencilState>(
			TAG_RES_GRP_PERMANENT_STATE,
			"DS_NO_DEPTHSTENCIL");

	m_pContext->OMSetDepthStencilState(
		noDepth->GetDepthStencilState().Get(),
		0);

	auto alphaBlend =
		E::CGameInstance::Get()
		.GetResourceFirst<E::CResBlendState>(
			TAG_RES_GRP_PERMANENT_STATE,
			"BS_ALPHA_BLEND");

	m_pContext->OMSetBlendState(
		alphaBlend->GetBlendState().Get(),
		nullptr,
		0xffffffff);

	const auto RenderRange =
		[this](auto begin, auto end)
		{
			for (auto iter = begin; iter != end; ++iter)
			{
				IRenderable* pRenderObject = *iter;

				if (pRenderObject &&
					pRenderObject->HasRenderPass(m_pRenderContext.pass))
				{
					pRenderObject->Render(
						m_pContext.Get(),
						m_pRenderContext);
				}
			}
		};

	// Weight < 1000
	RenderRange(renderList.begin(), lateBegin);

	// 기존 LateDraw 폰트
	E::CGameInstance::Get().FontLateDraw(RENDERGROUP::UI);

	// Weight >= 1000
	RenderRange(lateBegin, renderList.end());

	auto nonBlend =
		E::CGameInstance::Get()
		.GetResourceFirst<E::CResBlendState>(
			TAG_RES_GRP_PERMANENT_STATE,
			"BS_BLEND_NONE");

	m_pContext->OMSetBlendState(
		nonBlend->GetBlendState().Get(),
		nullptr,
		0xffffffff);

	return S_OK;

}

_float CRenderer::Get_HaltonSequence(uint32_t _FrameIndex, uint32_t _Base) {
	_float result = 0.f;
	_float fraction = 1.f / static_cast<_float>(_Base);
	uint32_t i = _FrameIndex;

	while (i > 0) {
		result += fraction * (i % _Base);
		i /= _Base;
		fraction /= static_cast<_float>(_Base);
	}
	return result;
}

#pragma endregion

VOID	CRenderer::RendererGUI() {
	ImGui::Begin("Renderer Controller");

	ImGui::TextDisabled("Shader Effect Controller");
	ImGui::BeginChild("##Inspector", ImVec2(0.f, 120.f), true);
	{
		if (m_bApplyEnvLight ? ImGui::Button("EnviromentLight OFF", ImVec2(-FLT_MIN, 20)) : ImGui::Button("EnviromentLight ON", ImVec2(-FLT_MIN, 20))) {
			m_bApplyEnvLight = !m_bApplyEnvLight;
		}
		if (m_bApplyFilter ? ImGui::Button("PostProcess OFF", ImVec2(-FLT_MIN, 20)) : ImGui::Button("PostProcess ON", ImVec2(-FLT_MIN, 20))) {
			m_bApplyFilter = !m_bApplyFilter;
		}
		if (m_bApplyVolumetricFog ? ImGui::Button("Volumetric Fog OFF", ImVec2(-FLT_MIN, 20)) : ImGui::Button("Volumetric Fog ON", ImVec2(-FLT_MIN, 20))) {
			m_bApplyVolumetricFog = !m_bApplyVolumetricFog;
		}
		if (m_bApplyVolumetricCloud ? ImGui::Button("Volumetric Cloud OFF", ImVec2(-FLT_MIN, 20)) : ImGui::Button("Volumetric Cloud ON", ImVec2(-FLT_MIN, 20))) {
			m_bApplyVolumetricCloud = !m_bApplyVolumetricCloud;
		}
		if (m_bApplyShadow ? ImGui::Button("Shadow OFF", ImVec2(-FLT_MIN, 20)) : ImGui::Button("Shadow ON", ImVec2(-FLT_MIN, 20))) {
			m_bApplyShadow = !m_bApplyShadow;
		}
	}
	ImGui::EndChild();

	if (m_bApplyEnvLight) {
		_float TextToSlotDistance = 130.f;
		if (ImGui::CollapsingHeader("Enviroment Option")) {
			_bool DirtyFlag = false;
			m_pEnvLightInfo = CGameInstance::Get().Get_EnviromentLight();

			ImGui::TextUnformatted("EnvLight Intensity");
			ImGui::SameLine(TextToSlotDistance);
			DirtyFlag |= ImGui::DragFloat("##Intensity", &m_pEnvLightInfo.m_fEnviromentIntensity, 0.001f, 0.f, 1.f, "%.3f");

			ImGui::TextUnformatted("FillLight Intensity");
			ImGui::SameLine(TextToSlotDistance);
			DirtyFlag |= ImGui::DragFloat("##FillLight", &m_pEnvLightInfo.m_fFillLightBrightness, 0.001f, 0.f, 1.f, "%.3f");

			ImGui::TextUnformatted("DirectLight Intensity");
			ImGui::SameLine(TextToSlotDistance);
			DirtyFlag |= ImGui::DragFloat("##DirectLight", &m_pEnvLightInfo.m_fDirectLightBrightness, 0.001f, 0.f, 1.f, "%.3f");

			if (DirtyFlag) 	CGameInstance::Get().Set_EnviromentLight(m_pEnvLightInfo);
		}
	}

	if (m_bApplyFilter) {
		ImGui::Separator();
		_float TextToSlotDistance = 120.f;
		if (ImGui::CollapsingHeader("PostProcess")) {
			ImGui::TextUnformatted("Blur Intensity");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##BlurIntensity", &m_pPostProcessBuffer.g_fBlurIntensity, 0.01f, -10.f, 10.f, "%.2f");

			ImGui::TextUnformatted("Distortion Intensity");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##DistortionIntensity", &m_pPostProcessBuffer.g_fDistortionIntensity, 0.01f, -10.f, 10.f, "%.2f");

			ImGui::TextUnformatted("Chromatic Intensity");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##ChromaticIntensity", &m_pPostProcessBuffer.g_fChromaticIntensity, 0.01f, -10.f, 10.f, "%.2f");

			ImGui::TextUnformatted("Vignette Intensity");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##VignetteIntensity", &m_pPostProcessBuffer.g_fVignetteIntensity, 0.01f, 0.f, 10.f, "%.2f");
		}
	}
	if (m_bApplyVolumetricFog) {
		_float TextToSlotDistance = 110.f;
		ImGui::Separator();
		if (ImGui::CollapsingHeader("Volumetric Fog CoreOption")) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 0.75f));
			ImGui::PopStyleColor();

			ImGui::TextUnformatted("FogColor");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::ColorEdit3("##FogColor", (float*)&m_pFogInfo.g_fFogColor);

			ImGui::TextUnformatted("FogIntensity");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##FogIntensity", &m_pFogInfo.g_fFogIntensity, 0.01f, 0.f, 10.f, "%.2f");

			ImGui::TextUnformatted("FogDensity");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##FogDensity", &m_pFogInfo.g_fFogDensity, 0.001f, 0.f, 5.f, "%.5f");

			ImGui::TextUnformatted("FogNoiseScale");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##FogNoiseScale", &m_pFogInfo.g_fFogNoiseScale, 0.001f, 0.001f, 0.5f, "%.3f");

			ImGui::TextUnformatted("FogScattering");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##FogScattering", &m_pFogInfo.g_fFogScattering, 0.01f, 0.f, 1.f, "%.2f");

			ImGui::TextUnformatted("FogBrightness");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##FogBrightness", &m_pFogInfo.g_fFogBaseBrightness, 0.001f, 0.f, 1.f, "%.3f");
		}
		if (ImGui::CollapsingHeader("Volumetric Fog LightOption")) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 0.75f));
			ImGui::PopStyleColor();

			ImGui::TextUnformatted("FogLightColor");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::ColorEdit3("##FogLightColor", (float*)&m_pFogInfo.g_fFogLightColor);

			ImGui::TextUnformatted("FogLightDirection");
			ImGui::SameLine(TextToSlotDistance);

			_float LightDirection[3] = { m_pFogInfo.g_fFogLightDirection.x, m_pFogInfo.g_fFogLightDirection.y, m_pFogInfo.g_fFogLightDirection.z };
			if (ImGui::DragFloat3("##LightDirection", LightDirection, 0.01f, -1.f, 1.f)) {
				XMVECTOR vDir = XMVectorSet(LightDirection[0], LightDirection[1], LightDirection[2], 0.f);
				if (XMVector3LengthSq(vDir).m128_f32[0] > 0.0001f) {
					vDir = XMVector3Normalize(vDir);
					XMStoreFloat3((XMFLOAT3*)&m_pFogInfo.g_fFogLightDirection, vDir);

					LightDirection[0] = m_pFogInfo.g_fFogLightDirection.x;
					LightDirection[1] = m_pFogInfo.g_fFogLightDirection.y;
					LightDirection[2] = m_pFogInfo.g_fFogLightDirection.z;
				}
			}
		}
		if (ImGui::CollapsingHeader("Volumetric Fog Height")) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 0.75f));
			ImGui::PopStyleColor();

			ImGui::TextUnformatted("FogBaseHeight");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##FogBaseHeight", &m_pFogInfo.g_fFogBaseHeight, 0.25f, -500.f, 500.f, "%.1f");

			ImGui::TextUnformatted("FogMaxHeight");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##FogMaxHeight", &m_pFogInfo.g_fFogMaxHeight, 0.25f, -500.f, 500.f, "%.1f");

			ImGui::TextUnformatted("FogHeightFallOff");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##FogHeightFallOff", &m_pFogInfo.g_fFogHeightFallOff, 0.25f, 0.f, 10.f, "%.2f");
		}
		if (ImGui::CollapsingHeader("Volumetric Fog Distance")) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 0.75f));
			ImGui::TextDisabled("Volumetric Fog Distance");
			ImGui::PopStyleColor();

			ImGui::TextUnformatted("FogStartDistance");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##FogStartDistance", &m_pFogInfo.g_fFogStartDistance, 0.5f, 0.f, 100.f, "%.1f");

			ImGui::TextUnformatted("FogEndDistance");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##FogEndDistance", &m_pFogInfo.g_fFogEndDistance, 1.f, m_pFogInfo.g_fFogStartDistance + 0.1f, 1000.f, "%.1f");
		}
	}
	if (m_bApplyVolumetricCloud) {
		ImGui::Separator();
		_float TextToSlotDistance = 120.f;
		if (ImGui::CollapsingHeader("Volumetric Cloud WindOption")) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 0.75f));
			ImGui::PopStyleColor();

			ImGui::TextUnformatted("CloudWindDirection");
			ImGui::SameLine(TextToSlotDistance);

			_float3 WindDir = m_pCloudInfo.g_fWindDirection;
			_float WindDirection[3] = { WindDir.x, WindDir.y, WindDir.z };
			if (ImGui::DragFloat3("##WindDirection", WindDirection, 0.01f, -1.f, 1.f)) {
				XMVECTOR vDir = XMVectorSet(WindDirection[0], WindDirection[1], WindDirection[2], 0.f);
				if (XMVector3LengthSq(vDir).m128_f32[0] > 0.0001f) {
					XMStoreFloat3((XMFLOAT3*)&WindDir, XMVector3Normalize(vDir));

					m_pCloudInfo.g_fWindDirection = WindDir;
				}
			}
		}

		if (ImGui::CollapsingHeader("Volumetric Cloud CoreOption")) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 0.75f));
			ImGui::PopStyleColor();

			ImGui::TextUnformatted("CloudColor");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::ColorEdit3("##CloudColor", (float*)&m_pCloudInfo.g_fCloudColor);

			ImGui::TextUnformatted("CloudBrightness");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##CloudBrightness", &m_pCloudInfo.g_fCloudBrightness, 0.05f, 0.f, 10.f, "%.2f");

			ImGui::TextUnformatted("CloudCoverage");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##CloudCoverage", &m_pCloudInfo.g_fCloudCoverage, 0.005f, 0.f, 3.f, "%.3f");

			ImGui::TextUnformatted("CloudDensity");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##CloudDensity", &m_pCloudInfo.g_fCloudDensity, 0.01f, 0.f, 5.f, "%.2f");

			ImGui::TextUnformatted("CloudScattering");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##CloudScattering", &m_pCloudInfo.g_fCloudScattering, 0.01f, -3.f, 3.f, "%.2f");

			ImGui::TextUnformatted("LightAbsorption");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##LightAbsorption", &m_pCloudInfo.g_fLightAbsorption, 0.001f, 0.f, 0.1f, "%.4f");
		}
		if (ImGui::CollapsingHeader("Volumetric Cloud SubOption")) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 0.75f));
			ImGui::PopStyleColor();

			ImGui::TextUnformatted("BaseCloudNoise");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##BaseCloudNoise", &m_pCloudInfo.g_fBaseCloudNoiseScale, 0.00001f, 0.f, 0.01f, "%.6f");

			ImGui::TextUnformatted("DetailCloudNoise");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##DetailCloudNoise", &m_pCloudInfo.g_fDetailCloudNoiseScale, 0.0001f, 0.f, 0.05f, "%.5f");
		}
		if (ImGui::CollapsingHeader("Volumetric Cloud Distribution")) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 0.75f));
			ImGui::PopStyleColor();

			ImGui::TextUnformatted("MinHeight");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##MinHeight", &m_pCloudInfo.g_fCloudMinHeight, 10.f, 0.f, 10000.f, "%.0f");

			ImGui::TextUnformatted("MaxHeight");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##MaxHeight", &m_pCloudInfo.g_fCloudMaxHeight, 10.f, 0.f, 20000.f, "%.0f");

			ImGui::TextUnformatted("LODDistance");
			ImGui::SameLine(TextToSlotDistance);
			ImGui::DragFloat("##LODDistance", &m_pCloudInfo.g_fCloudLODDistance, 500.f, 1000.f, 200000.f, "%.0f");
		}
		if (m_bApplyVolumetricCloudTAA ? ImGui::Button("Cloud TAA OFF", ImVec2(-FLT_MIN, 20)) : ImGui::Button("Cloud TAA ON", ImVec2(-FLT_MIN, 20))) {
			m_bApplyVolumetricCloudTAA = !m_bApplyVolumetricCloudTAA;
		}
	}
	if (m_bApplyShadow) {
		ImGui::Separator();
	}
	ImGui::End();
}

#ifdef _DEBUG
HRESULT CRenderer::Initialize_Debugging()
{
	m_pDebugRenderBuffer = CGameInstance::Get().GetResourceFirst<CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");
	if (!m_pDebugRenderBuffer)		return E_FAIL;

	m_pDebugVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTex");
	if (!m_pDebugVertexShader)		return E_FAIL;

	m_pDebugPixelShader  = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTex");
	if (!m_pDebugPixelShader)		return E_FAIL;

	XMFLOAT2	vViewportSize = { m_pBackBufferViewPort->GetViewPort().Width , m_pBackBufferViewPort->GetViewPort().Height };
	XMFLOAT2    vDebugViewSize = { vViewportSize.x / 4.f, vViewportSize.y / 4.f };
	XMMATRIX    mDebugViewScaleMatrix = XMMatrixScaling(vDebugViewSize.x, vDebugViewSize.y, 1.f);

	XMFLOAT2    vDebugViewStartPoint = { vDebugViewSize.x * 0.5f - vViewportSize.x * 0.5f, -vDebugViewSize.y * 0.5f + vViewportSize.y * 0.5f };

	m_pResDynTexTargetList.push_back(m_pResDynTexTargetDiffuse);
	m_pResDynTexTargetList.push_back(m_pResDynTexTargetNormal);

	m_pResDynTexTargetList.push_back(m_pResDynTexTargetSMRO);
	m_pResDynTexTargetList.push_back(m_pResDynTexTargetEmissive);

	m_pResDynTexTargetList.push_back(m_pResDynTexTargetUI);

	m_pResDynTexTargetList.push_back(m_pResDynTexTargetUI3D);

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
	if (CGameInstance::Get().KeyDown(DIK_F7))	m_bRenderDebugScreen = !m_bRenderDebugScreen;

	if (!m_bRenderDebugScreen) return S_OK;
	TracyD3D11Zone(m_pTracyGpuContext, "Debug Overlay");

	auto ActiveCam = CGameInstance::Get().GetActiveCamera();

	XMFLOAT2	vViewportSize = { m_pBackBufferViewPort->GetViewPort().Width, m_pBackBufferViewPort->GetViewPort().Height };

	XMMATRIX    m_WorldMatrix, m_ViewMatrix, m_ProjMatrix;
	m_ViewMatrix = XMMatrixIdentity();
	m_ProjMatrix = XMMatrixOrthographicLH(vViewportSize.x, vViewportSize.y, 0.f, 1.f);

	m_pContext->IASetInputLayout(m_pDebugVertexShader->GetInputLayout().Get());
	m_pContext->VSSetShader(m_pDebugVertexShader->GetVertexShader().Get(), nullptr, 0);
	m_pContext->PSSetShader(m_pDebugPixelShader->GetPixelShader().Get(), nullptr, 0);

	ID3D11Buffer* vertexBuffers[] = { m_pDebugRenderBuffer->GetVertexBuffer().Get() };
	uint32_t strides[] = { m_pDebugRenderBuffer->GetVertexStride() };
	uint32_t offsets[] = { 0 };

	m_pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	m_pContext->IASetIndexBuffer(m_pDebugRenderBuffer->GetIndexBuffer().Get(), m_pDebugRenderBuffer->GetIndexFormat(), 0);
	m_pContext->IASetPrimitiveTopology(m_pDebugRenderBuffer->GetPrimitiveType());

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
        m_pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, pCbPerObject->GetCBuffer().GetAddressOf());
        m_pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, pCbPerObject->GetCBuffer().GetAddressOf());
	
        if (IDX == 8) {
            m_pContext->PSSetShaderResources(0, 1, m_pBackBufferSRV.GetAddressOf());
        }
        else {
            m_pContext->PSSetShaderResources(0, 1, m_pResDynTexTargetList[IDX]->GetSRV().GetAddressOf());
        }
        
        m_pContext->DrawIndexed(m_pDebugRenderBuffer->GetNumIndices(), 0, 0);
    }
    return S_OK;
}
#endif
VOID CRenderer::Render_ChromaticRing(XMVECTOR _WorldPosition, _float _Duration, _float _Scale){
	auto ActiveCam = CGameInstance::Get().GetActiveCamera();
	if (nullptr == ActiveCam) return;

	XMMATRIX ViewMat = ActiveCam->GetView();
	XMMATRIX ProjMat = ActiveCam->GetProj();

	_float2 ScreenSize = CGameInstance::Get().GetClientScreenSize();

	XMVECTOR CurrentPos = XMVectorSetW(_WorldPosition, 1.f);

	XMMATRIX ViewProj = ViewMat * ProjMat;
	XMVECTOR ClipPos = XMVector3TransformCoord(CurrentPos, ViewProj);

	_float NDC_X = XMVectorGetX(ClipPos);
	_float NDC_Y = XMVectorGetY(ClipPos);
	_float NDC_Z = XMVectorGetZ(ClipPos);

	if (NDC_Z > 1.f || NDC_Z < 0.f) return;

	m_fScreenPosition.x = ((NDC_X + 1.f) / 2.f) * ScreenSize.x;
	m_fScreenPosition.y = ((1.f - NDC_Y) / 2.f) * ScreenSize.y;

	m_fExpandDuration = _Duration;
	m_fCurrentLifeTime = 0.f;
	m_fRingScale = _Scale;
}

VOID CRenderer::Clear_OutlineEffect() {
	m_pOutlineTargetHandle = std::nullopt;

	m_pContext->ClearDepthStencilView(m_pResDynTexTargetFocusingDepthMap->GetDSV().Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
}

#pragma region BLOOMHELPER
HRESULT	CRenderer::Update_TexelSize(_float _Width, _float _Height){
	CB_BLOOM	cbBloomBuffer{};

	cbBloomBuffer.g_fTexelSize = { _Width, _Height };

	D3D11_MAPPED_SUBRESOURCE BLURMRES{};
	if (FAILED(m_pContext->Map(m_pBloomCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &BLURMRES)))	return E_FAIL;
	{
		memcpy(BLURMRES.pData, &cbBloomBuffer, sizeof(CB_BLOOM));
		m_pContext->Unmap(m_pBloomCBuffer->GetCBuffer().Get(), 0);
	}

	m_pContext->CSSetConstantBuffers(10, 1, m_pBloomCBuffer->GetCBuffer().GetAddressOf());

	return S_OK;
}

HRESULT CRenderer::Render_BrightPass(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _OriginTexture, uint32_t _ScreenX, uint32_t _ScreenY) {
	{
		if (nullptr == _OutPut || nullptr == _OriginTexture) return E_FAIL;

		auto OutputUAV = _OutPut->GetUAV();
		if (nullptr == OutputUAV) return E_FAIL;

		m_pContext->CSSetShader(m_pBrightPassComputeShader->GetComputeShader().Get(), nullptr, 0);

		ID3D11UnorderedAccessView* pUAVs[1] = { OutputUAV.Get() };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pUAVs, nullptr);

		ID3D11ShaderResourceView* pOriginTex[1] = { _OriginTexture->GetSRV().Get() };
		m_pContext->CSSetShaderResources(0, 1, pOriginTex);
	}
	{
		m_pContext->Dispatch((_ScreenX + 7) / 8, (_ScreenY + 7) / 8, 1);

		ID3D11UnorderedAccessView* pNullUAVs[1] = { nullptr };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pNullUAVs, nullptr);

		ID3D11ShaderResourceView* NULLSRV[1] = { nullptr };
		m_pContext->CSSetShaderResources(0, 1, NULLSRV);
	}

	return S_OK;
}

HRESULT CRenderer::Render_VerticalBlurPass(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _BlurPassTexture, uint32_t _ScreenX, uint32_t _ScreenY) {
	{
		if (nullptr == _OutPut || nullptr == _BlurPassTexture) return E_FAIL;

		auto OutputUAV = _OutPut->GetUAV();
		if (nullptr == OutputUAV) return E_FAIL;

		m_pContext->CSSetShader(m_pVerticalBlurComputeShader->GetComputeShader().Get(), nullptr, 0);

		ID3D11UnorderedAccessView* pUAVs[1] = { OutputUAV.Get() };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pUAVs, nullptr);

		ID3D11ShaderResourceView* pBloomTex[1] = { _BlurPassTexture->GetSRV().Get() };
		m_pContext->CSSetShaderResources(1, 1, pBloomTex);
	}
	{
		m_pContext->Dispatch((_ScreenX + 7) / 8, (_ScreenY + 7) / 8, 1);

		ID3D11UnorderedAccessView* pNullUAVs[1] = { nullptr };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pNullUAVs, nullptr);

		ID3D11ShaderResourceView* NULLSRV[1] = { nullptr };
		m_pContext->CSSetShaderResources(1, 1, NULLSRV);
	}

	return S_OK;
}

HRESULT CRenderer::Render_HorizontalBlurPass(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _BlurPassTexture, uint32_t _ScreenX, uint32_t _ScreenY) {
	{
		if (nullptr == _OutPut || nullptr == _BlurPassTexture) return E_FAIL;

		auto OutputUAV = _OutPut->GetUAV();
		if (nullptr == OutputUAV) return E_FAIL;

		m_pContext->CSSetShader(m_pHorizontalBlurComputeShader->GetComputeShader().Get(), nullptr, 0);

		ID3D11UnorderedAccessView* pUAVs[1] = { OutputUAV.Get() };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pUAVs, nullptr);

		ID3D11ShaderResourceView* pBloomTex[1] = { _BlurPassTexture->GetSRV().Get() };
		m_pContext->CSSetShaderResources(1, 1, pBloomTex);
	}
	{
		m_pContext->Dispatch((_ScreenX + 7) / 8, (_ScreenY + 7) / 8, 1);

		ID3D11UnorderedAccessView* pNullUAVs[1] = { nullptr };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pNullUAVs, nullptr);

		ID3D11ShaderResourceView* NULLSRV[1] = { nullptr };
		m_pContext->CSSetShaderResources(1, 1, NULLSRV);
	}

	return S_OK;
}

HRESULT CRenderer::Render_UpSampleCombinePass(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _HalfBloomTex, const SPtr<CResDynamicTexture2D>& _QuarterBloomTex, uint32_t _ScreenX, uint32_t _ScreenY) {
	{
		if (nullptr == _OutPut || nullptr == _HalfBloomTex || nullptr == _QuarterBloomTex) return E_FAIL;

		auto OutputUAV = _OutPut->GetUAV();
		if (nullptr == OutputUAV) return E_FAIL;

		m_pContext->CSSetShader(m_pUpSampleComputeShader->GetComputeShader().Get(), nullptr, 0);

		ID3D11UnorderedAccessView* pUAVs[1] = { OutputUAV.Get() };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pUAVs, nullptr);

		ID3D11ShaderResourceView* pBloomTex[2] = { _HalfBloomTex->GetSRV().Get(), _QuarterBloomTex->GetSRV().Get() };
		m_pContext->CSSetShaderResources(0, 2, pBloomTex);
	}
	{
		m_pContext->Dispatch((_ScreenX + 7) / 8, (_ScreenY + 7) / 8, 1);

		ID3D11UnorderedAccessView* pNullUAVs[1] = { nullptr };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pNullUAVs, nullptr);

		ID3D11ShaderResourceView* NULLSRV[2] = { nullptr, nullptr };
		m_pContext->CSSetShaderResources(0, 2, NULLSRV);
	}

	return S_OK;
}

HRESULT CRenderer::Render_DownSamplePass(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _SrcTex, uint32_t _ScreenX, uint32_t _ScreenY) {
	{
		m_pContext->CSSetShader(m_pDownSampleComputeShader->GetComputeShader().Get(), nullptr, 0);

		ID3D11UnorderedAccessView* pUAVs[1] = { _OutPut->GetUAV().Get() };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pUAVs, nullptr);

		ID3D11ShaderResourceView* pBloomSRV[1] = { _SrcTex->GetSRV().Get() };
		m_pContext->CSSetShaderResources(0, 1, pBloomSRV);

		m_pContext->Dispatch((_ScreenX + 7) / 8, (_ScreenY + 7) / 8, 1);

		ID3D11UnorderedAccessView* pNullUAVs[1] = { nullptr };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pNullUAVs, nullptr);

		ID3D11ShaderResourceView* NULLSRV[1] = { nullptr };
		m_pContext->CSSetShaderResources(0, 1, NULLSRV);
	}

	return S_OK;
}

HRESULT CRenderer::Render_CombinedPass(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _OriginTexture, const SPtr<CResDynamicTexture2D>& _BlurPassTexture, uint32_t _ScreenX, uint32_t _ScreenY) {
	{
		m_pContext->CSSetShader(m_pBloomPassComputeShader->GetComputeShader().Get(), nullptr, 0);

		ID3D11UnorderedAccessView* pUAVs[1] = { _OutPut->GetUAV().Get() };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pUAVs, nullptr);

		ID3D11ShaderResourceView* pBloomSRV[2] = { _OriginTexture->GetSRV().Get(), _BlurPassTexture->GetSRV().Get() };
		m_pContext->CSSetShaderResources(0, 2, pBloomSRV);
	}
	{
		m_pContext->Dispatch((_ScreenX + 7) / 8, (_ScreenY + 7) / 8, 1);

		ID3D11UnorderedAccessView* pNullUAVs[1] = { nullptr };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pNullUAVs, nullptr);

		ID3D11ShaderResourceView* pNULLSRV[2] = { nullptr, nullptr };
		m_pContext->CSSetShaderResources(0, 2, pNULLSRV);
	}

	return S_OK;
}
HRESULT CRenderer::Render_RadialBlur(const SPtr<CResDynamicTexture2D>& _OutPut, const SPtr<CResDynamicTexture2D>& _OriginTexture, uint32_t _ScreenX, uint32_t _ScreenY) {
	{
		m_pContext->CSSetShader(m_pRadialBlurComputeShader->GetComputeShader().Get(), nullptr, 0);

		ID3D11UnorderedAccessView* pUAVs[1] = { _OutPut->GetUAV().Get() };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pUAVs, nullptr);

		ID3D11ShaderResourceView* pBloomSRV[1] = { _OriginTexture->GetSRV().Get() };
		m_pContext->CSSetShaderResources(0, 1, pBloomSRV);
	}
	{
		m_pContext->Dispatch((_ScreenX + 7) / 8, (_ScreenY + 7) / 8, 1);

		ID3D11UnorderedAccessView* pNullUAVs[1] = { nullptr };
		m_pContext->CSSetUnorderedAccessViews(0, 1, pNullUAVs, nullptr);

		ID3D11ShaderResourceView* pNULLSRV[1] = { nullptr };
		m_pContext->CSSetShaderResources(0, 1, pNULLSRV);
	}

	return S_OK;
}
#pragma endregion

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
	ZoneScopedN("BuildCurrentHizBuffer");
	TracyD3D11Zone(m_pTracyGpuContext, "Hi-Z Build");
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
