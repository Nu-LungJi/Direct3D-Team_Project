#include "pch.h"
#include "Renderer.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"
#include "MyGFSDK_SSAO.h"

NS_USING(Engine)
CRenderer::CRenderer(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext) : m_pDevice{ pDevice } , m_pContext{ pContext } { }
CRenderer::~CRenderer() {   }

void CRenderer::UpdateGUI()
{
    ImGui::Begin("Renderer");

    ImGui::End();

#ifdef _DEBUG
    PostProcessGUI();
#endif
}

HRESULT CRenderer::Initialize()
{
    m_pBackBufferDSV = CGameInstance::Get().GetBackBufferDSV();
    m_pBackBufferRTV = CGameInstance::Get().GetBackBufferRTV();
    m_pBackBufferVP  = CGameInstance::Get().GetResourceFirst<CResViewPort>(TAG_RES_GRP_PERMANENT_VP, "VP_BackBuffer");

    if (FAILED(InitializeGFSDK_SSAO()))
    {
        return E_FAIL;
    }

    if (FAILED(InitializeOffscreen()))
    {
        return E_FAIL;
    }

    if (FAILED(InitializeShadow()))
    {
        return E_FAIL;
    }

    if (FAILED(InitializeFullscreen()))
    {
        return E_FAIL;
    }

    if (FAILED(InitializeBaseTarget()))
    {
        return E_FAIL;
    }

    if (FAILED(InitializeTargetPBR()))
    {
        return E_FAIL;
    }

    if (FAILED(InitializeBlendTarget()))
    {
        return E_FAIL;
    }

    if (FAILED(InitilizePostProcess()))
    {
        return E_FAIL;
    }

#ifdef _DEBUG
    if (FAILED(Initialize_Debugging()))
    {
        return E_FAIL;
    }


    D3D11_TEXTURE2D_DESC backBufferDesc;
    CGameInstance::Get().GetBackBufferTexture()->GetDesc(&backBufferDesc);

    D3D11_TEXTURE2D_DESC copyDesc = backBufferDesc;
    copyDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; 
    copyDesc.CPUAccessFlags = 0;
    copyDesc.Usage = D3D11_USAGE_DEFAULT;

    if (FAILED(m_pDevice->CreateTexture2D(&copyDesc, nullptr, m_pBackBufferCopyTexture.GetAddressOf()))) return E_FAIL;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = copyDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    if (FAILED(m_pDevice->CreateShaderResourceView(m_pBackBufferCopyTexture.Get(), &srvDesc, m_pBackBufferCopySRV.GetAddressOf()))) return E_FAIL;
#endif

    return S_OK;
}

HRESULT CRenderer::InitializeOffscreen()
{
    if (m_pOffScreenVertexShader = CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_Deferred")) {
        if (FAILED(m_pOffScreenVertexShader->Load()))   return E_FAIL;
    }

    if (m_pOffScreenPixelShader  = CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_Deferred")) {
        if (FAILED(m_pOffScreenPixelShader->Load()))    return E_FAIL;
    }

    m_pOffScreenTex2D = Generate_RenderTarget("DynTex2D_Offscreen", DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    if (nullptr == m_pOffScreenTex2D)                   return E_FAIL;

    return S_OK;
}

HRESULT CRenderer::InitializeShadow()
{
    UINT iShadowWidth   = 2048 * 2;
    UINT iShadowHeight  = 2048 * 2;

    m_pShadowTex2D = Generate_DepthStencil_RenderTarget("DynTex2D_Shadow", DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, iShadowWidth, iShadowHeight);
    if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_VP, "VP_Shadow", E::CResViewPort::Create()))
    {
        D3D11_VIEWPORT Desc{};
        Desc.TopLeftX = 0.f;
        Desc.TopLeftY = 0.f;
        Desc.Width = static_cast<float>(iShadowWidth);
        Desc.Height = static_cast<float>(iShadowHeight);
        Desc.MinDepth = 0.f;
        Desc.MaxDepth = 1.f;
        if (FAILED(res->Load(Desc)))
        {
            return E_FAIL;
        }
        m_pShadowVP = res;
    }
    
    return S_OK;
}

HRESULT CRenderer::InitializeFullscreen()
{
    if (auto res = CGameInstance::Get().AddResource(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_FullscreenTex", E::CResQuadFullscreenTexBuffer::Create()))
    {
        if (FAILED(res->Load()))    return E_FAIL;
    }

    if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_FullscreenQuad", "./ShaderFiles/FullscreenQuad/FullscreenQuad.hlsl"))
    {
        if (FAILED(res->Load()))    return E_FAIL;
    }

    if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_FullscreenQuad", "./ShaderFiles/FullscreenQuad/FullscreenQuad.hlsl"))
    {
        if (FAILED(res->Load()))    return E_FAIL;
    }

    if (m_pFullscreenVS = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_FullscreenQuad"))
    {
        if (FAILED(m_pFullscreenVS->Load()))    return E_FAIL;
    }

    if (m_pFullscreenPS = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_FullscreenQuad"))
    {
        if (FAILED(m_pFullscreenPS->Load()))    return E_FAIL;
    }

    if (m_pFullscreenVIBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResVIBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_FullscreenTex"))
    {
        if (FAILED(m_pFullscreenVIBuffer->Load()))    return E_FAIL;
    }

    return S_OK;
}

HRESULT CRenderer::InitializeBaseTarget() {
    m_pResDynTexTargetDiffuse = Generate_RenderTarget("DynTex2D_Target_Diffuse", DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    if (nullptr == m_pResDynTexTargetDiffuse)       return E_FAIL;

    m_pResDynTexTargetSMRO = Generate_RenderTarget("DynTex2D_Target_SMRO", DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    if (nullptr == m_pResDynTexTargetSMRO)          return E_FAIL;

    m_pResDynTexTargetEmissive = Generate_RenderTarget("DynTex2D_Target_Emissive", DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    if (nullptr == m_pResDynTexTargetEmissive)      return E_FAIL;

    m_pResDynTexTargetNormal = Generate_RenderTarget("DynTex2D_Target_Normal", DXGI_FORMAT_R16G16B16A16_UNORM, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    if (nullptr == m_pResDynTexTargetNormal)        return E_FAIL;

    m_pResDynTexTargetDepth = Generate_DepthStencil_RenderTarget("DynTex2D_Target_Depth", DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT, DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
    if (nullptr == m_pResDynTexTargetDepth)        return E_FAIL;

    return S_OK;
}

HRESULT CRenderer::InitializeTargetPBR() 
{
    m_pResDynTexTargetPBR = Generate_RenderTarget("DynTex2D_Target_PBR", DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    if (nullptr == m_pResDynTexTargetPBR)        return E_FAIL;

    if (m_pPBRVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_PBR"))//CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_PBR", "./ShaderFiles/FullscreenQuad/FullscreenQuad.hlsl"))
    {
        if (FAILED(m_pPBRVertexShader->Load()))    return E_FAIL;
    }

    if (m_pPBRPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PBR"))
    {
        if (FAILED(m_pPBRPixelShader->Load()))    return E_FAIL;
    }
    return S_OK;
}

HRESULT CRenderer::InitializeBlendTarget() {
    if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_PBR_BLEND", "../../Engine/ShaderFiles/PBR/VS_PBR.hlsl"))
    {
        if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "VSMain_Blend", .sTarget = "vs_5_0" })))  return E_FAIL;
    }
    if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PBR_BLEND", "../../Engine/ShaderFiles/PBR/PS_PBR.hlsl"))
    {
        if (FAILED(res->Load(CResShader::DESC{ .sEntryPoint = "PSMain_Blend", .sTarget = "ps_5_0" })))  return E_FAIL;
    }

    if (m_pBlendVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_PBR_BLEND"))
    {
        if (FAILED(m_pBlendVertexShader->Load())) return E_FAIL;
    }
    if (m_pBlendPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PBR_BLEND"))
    {
        if (FAILED(m_pBlendPixelShader->Load())) return E_FAIL;
    }
    return S_OK;
}

HRESULT CRenderer::InitilizePostProcess(){
    // PostProcess
    m_pResDynTexTargetPostProcess = Generate_RenderTarget("DynTex2D_PostProcess", DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    {
        //auto vClientScreenSize = CGameInstance::Get().GetClientScreenSize();
        //
        //if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_TEXTURE, "DynTex2D_PostProcess", E::CResDynamicTexture2D::Create()))
        //{
        //    CResDynamicTexture2D::DESC Desc{};
        //    Desc.texDesc = {
        //        .Width = (UINT)vClientScreenSize.x,
        //        .Height = (UINT)vClientScreenSize.y,
        //        .MipLevels = 1,
        //        .ArraySize = 1,
        //        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        //        .SampleDesc = {.Count = 1, .Quality = 0 },
        //        .Usage = D3D11_USAGE_DEFAULT,
        //        .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
        //        .CPUAccessFlags = 0,
        //        .MiscFlags = 0
        //    };
        //    if (FAILED(res->Load(Desc)))
        //    {
        //        return E_FAIL;
        //    }
        //    if (FAILED(res->CreateSRV()))
        //    {
        //        return E_FAIL;
        //    }
        //    if (FAILED(res->CreateRTV()))
        //    {
        //        return E_FAIL;
        //    }
        //    
        //}
    }
    // PostProcess ConstantBuffer Create
    if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PostProcess", E::CResCBuffer::Create()))
    {
        if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(POSTPROCESS) })))    return E_FAIL;
    }
    // LUT Texture Create
    if (FAILED(CreateWICTextureFromFile(m_pDevice.Get(), L"./Resources/Engine/Texture/PostProcess/LUT_Fuji.png", nullptr, m_pLUTTexture.GetAddressOf()))) {
        MSG_BOX("Cannot Create LUT Texture File.");
        assert(0);
    }
    
    m_pPostProcessPS = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PostProcess_Filter");

    return S_OK;
}

HRESULT CRenderer::InitializeGFSDK_SSAO()
{
    m_pGFSDK_SSAO = CMyGFSDK_SSAO::Create();
    if (!m_pGFSDK_SSAO)
    {
        return E_FAIL;
    }
    return S_OK;
}


HRESULT CRenderer::AddRenderObject(RENDERGROUP eRenderGroup, IRenderable* pRenderObject)
{
    if (eRenderGroup >= RENDERGROUP::END ||
        nullptr == pRenderObject)
        return E_FAIL;

    m_RenderObject[ETOUI(eRenderGroup)].push_back(pRenderObject);
    return S_OK;
}

SPtr<CResDynamicTexture2D> CRenderer::Generate_RenderTarget(const StringID& _sResTag, DXGI_FORMAT _Format, uint32_t _BindFlags, uint32_t _TexWidth, uint32_t _TexHeight) {
    auto vClientScreenSize = CGameInstance::Get().GetClientScreenSize();

    if (_TexWidth == 0)     _TexWidth  = vClientScreenSize.x;
    if (_TexHeight == 0)    _TexHeight = vClientScreenSize.y;

    if (auto Resource = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_TEXTURE, _sResTag, E::CResDynamicTexture2D::Create()))
    {
        CResDynamicTexture2D::DESC Desc{};
        Desc.texDesc = {
            .Width          = _TexWidth,
            .Height         = _TexHeight,
            .MipLevels      = 1,
            .ArraySize      = 1,
            .Format         = _Format,
            .SampleDesc     = {.Count = 1, .Quality = 0 },
            .Usage          = D3D11_USAGE_DEFAULT,
            .BindFlags      = _BindFlags,
            .CPUAccessFlags = 0,
            .MiscFlags      = 0
        };
        if (FAILED(Resource->Load(Desc)))    return nullptr;

        if (FAILED(Resource->CreateSRV()))   return nullptr;

        if (FAILED(Resource->CreateRTV()))   return nullptr;

        return Resource;
    }

    return nullptr;
}
SPtr<CResDynamicTexture2D> CRenderer::Generate_DepthStencil_RenderTarget(const StringID& _sResTag, DXGI_FORMAT _TexFormat, DXGI_FORMAT _DSVFormat, DXGI_FORMAT _SRVFormat, uint32_t _TexWidth, uint32_t _TexHeight){
    auto vClientScreenSize = CGameInstance::Get().GetClientScreenSize();

    if (_TexWidth == 0)     _TexWidth = vClientScreenSize.x;
    if (_TexHeight == 0)    _TexHeight = vClientScreenSize.y;

    if (auto Resource = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_TEXTURE, _sResTag, E::CResDynamicTexture2D::Create()))
    {
        CResDynamicTexture2D::DESC Desc{};
        Desc.texDesc        = {
            .Width          = _TexWidth,
            .Height         = _TexHeight,
            .MipLevels      = 1,
            .ArraySize      = 1,
            .Format         = _TexFormat,
            .SampleDesc     = {.Count = 1, .Quality = 0 },
            .Usage          = D3D11_USAGE_DEFAULT,
            .BindFlags      = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,
            .CPUAccessFlags = 0,
            .MiscFlags      = 0
        };

        CResDynamicTexture2D::DESC DynTex2DDesc{};
        DynTex2DDesc.texDesc = Desc.texDesc;
        if (FAILED(Resource->Load(DynTex2DDesc))) return nullptr;

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format                      = _DSVFormat;
        dsvDesc.ViewDimension               = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice          = 0;
        if (FAILED(Resource->CreateDSV(dsvDesc))) return nullptr;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format                      = _SRVFormat;
        srvDesc.ViewDimension               = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels         = 1;
        srvDesc.Texture2D.MostDetailedMip   = 0;
        if (FAILED(Resource->CreateSRV(srvDesc))) return nullptr;

        return Resource;
    }

    return nullptr;
}

HRESULT CRenderer::Draw() {
    ZoneScopedN("Draw");

    RENDER_CTX ctx{};

    _bool bApplyShadow = false;
    if (bApplyShadow)
        if (FAILED(Render_ShadowMap(ctx)))  return E_FAIL;

    // MRT Draw
    
    // DepthMap
    if (FAILED(Render_DepthMap(ctx)))  return E_FAIL;

    // Diffuse + Normal + SMRO + Emissive
    if (FAILED(Render_NonAlpha(ctx)))  return E_FAIL;
    
    // PBR
    if (FAILED(RenderPBR(ctx)))             return E_FAIL;

    // Trensparent
    if (FAILED(Render_Alpha(ctx)))          return E_FAIL;

    // Combined
    if (FAILED(Render_OffScreen(ctx)))      return E_FAIL;

    // PostProcess
    if (FAILED(RenderPostProcess(ctx)))     return E_FAIL;

    {
        m_pLastTex2DBeforeFullScreenDraw = ApplyFilter ? m_pResDynTexTargetPostProcess : m_pOffScreenTex2D;
    }

    // FullScreen : Final
    if (FAILED(Render_FullScreen()))           return E_FAIL;

    //{
    //    auto pUICame = CGameInstance::Get().GetCamera("UI");
    //    if (nullptr == pUICame) return S_OK;
    //
    //    ctx.matProj = pUICame->GetProj();
    //    ctx.matView = pUICame->GetView();
    //    ctx.matViewProj = ctx.matView * ctx.matProj;
    //    ctx.eye = pUICame->GetTransform().GetLoadedPostion();
    //
    //    if (FAILED(Bind_CameraAttribute(pUICame)))
    //    {
    //        return E_FAIL;
    //    }
    //    if (FAILED(RenderUI(ctx)))
    //    {
    //        return E_FAIL;
    //    }
    //}

#ifdef _DEBUG
    if (FAILED(Render_Debugging(ctx)))     return E_FAIL;
#endif

    return S_OK;
}

void CRenderer::FrameEnd()
{
    for (auto& vecRenderables : m_RenderObject)
    {
        vecRenderables.clear();
    }
}

HRESULT CRenderer::Render_ShadowMap(RENDER_CTX& ctx){
    // UnBind Shadow Map
    {
        ID3D11ShaderResourceView* pShadowSRVs[1] = { nullptr };
        m_pContext->PSSetShaderResources(4, 1, pShadowSRVs);
    }

    // RenderTarget/DepthStencil Setting + ViewPort Setting
    {
        ID3D11RenderTargetView* pRTVs[1] = { nullptr };
        m_pContext->OMSetRenderTargets(1, pRTVs, m_pShadowTex2D->GetDSV().Get());
        m_pContext->ClearDepthStencilView(m_pShadowTex2D->GetDSV().Get(), D3D11_CLEAR_DEPTH, 1.f, 0);
        m_pContext->RSSetViewports(1, &m_pShadowVP->GetViewPort());
    }

    ctx.pass = RENDERPASS::SHADOW;

    {
        auto pShadowCamera = CGameInstance::Get().GetCamera("Shadow");
        if (nullptr != pShadowCamera)
        {
            ctx.matProj = pShadowCamera->GetProj();
            ctx.matView = pShadowCamera->GetView();
            ctx.matViewProj = ctx.matView * ctx.matProj;
            ctx.eye = pShadowCamera->GetTransform().GetLoadedPostion();

            auto pCbPerPass = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PASS);
            D3D11_MAPPED_SUBRESOURCE mappedSubResource;
            if (SUCCEEDED(m_pContext->Map(pCbPerPass->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
            {
                CB_PER_PASS cbPerPass{};
                XMStoreFloat4x4(&cbPerPass.matProj, pShadowCamera->GetProj());
                XMStoreFloat4x4(&cbPerPass.matView, pShadowCamera->GetView());
                XMStoreFloat4x4(&cbPerPass.matViewProj, pShadowCamera->GetView() * pShadowCamera->GetProj());
                XMStoreFloat4x4(&cbPerPass.matInvView, XMMatrixInverse(nullptr, pShadowCamera->GetView()));
                XMStoreFloat4x4(&cbPerPass.matInvViewProj, XMMatrixInverse(nullptr, XMLoadFloat4x4(&cbPerPass.matViewProj)));
                cbPerPass.vCamPos = pShadowCamera->GetTransform().GetPosition();

                memcpy(mappedSubResource.pData, &cbPerPass, sizeof(cbPerPass));
                m_pContext->Unmap(pCbPerPass->GetCBuffer().Get(), 0);
            }
            m_pContext->VSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
            m_pContext->PSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
            m_pContext->GSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());

            if (FAILED(RenderNonBlend(ctx)))    return E_FAIL;
        }
    }
    
    return S_OK;
}
HRESULT CRenderer::Render_DepthMap(RENDER_CTX& ctx) {
    ZoneScopedN("Render_DepthMap");
    {
        ID3D11RenderTargetView* pRTVs[1] = { m_pResDynTexTargetDepth->GetRTV().Get() };
        m_pContext->OMSetRenderTargets(1, pRTVs, m_pResDynTexTargetDepth->GetDSV().Get());
        m_pContext->RSSetViewports(1, &m_pBackBufferVP->GetViewPort());

        //_float4 clearColor = { 0.f, 0.f, 1.f, 1.f };
        //m_pContext->ClearRenderTargetView(pRTVs[0], reinterpret_cast<const float*>(&clearColor));
        m_pContext->ClearDepthStencilView(m_pResDynTexTargetDepth->GetDSV().Get(), D3D11_CLEAR_DEPTH, 1.f, 0);

        auto pGameCam = CGameInstance::Get().GetActiveCamera();
        if (nullptr == pGameCam)    return S_OK;

        ctx.matProj = pGameCam->GetProj();
        ctx.matView = pGameCam->GetView();
        ctx.matViewProj = ctx.matView * ctx.matProj;
        ctx.eye = pGameCam->GetTransform().GetLoadedPostion();

        if (FAILED(Bind_CameraAttribute(pGameCam)))
        {
            return E_FAIL;
        }
        if (FAILED(RenderNonBlend(ctx)))    return E_FAIL;
    }
    return S_OK;
}
HRESULT CRenderer::Render_NonAlpha(RENDER_CTX& ctx) {
    ZoneScopedN("Render_NonAlpha");
    {
        ID3D11RenderTargetView* pRTVs[4] = {
            m_pResDynTexTargetDiffuse->GetRTV().Get(),
            m_pResDynTexTargetNormal->GetRTV().Get(), 
            m_pResDynTexTargetSMRO->GetRTV().Get(),
            m_pResDynTexTargetEmissive->GetRTV().Get(),
        };
        m_pContext->OMSetRenderTargets(4, pRTVs, m_pResDynTexTargetDepth->GetDSV().Get());
        m_pContext->RSSetViewports(1, &m_pBackBufferVP->GetViewPort());

        SPtr<CResDepthStencilState> DepthState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHWRITE");
        m_pContext->OMSetDepthStencilState(DepthState->GetDepthStencilState().Get(), 0);

        ctx.pass = RENDERPASS::DEFAULT;
        _float4 clearColor = { 0.f, 0.f, 1.f, 1.f };
        m_pContext->ClearRenderTargetView(pRTVs[0], reinterpret_cast<const float*>(&clearColor));
        m_pContext->ClearRenderTargetView(pRTVs[1], reinterpret_cast<const float*>(&clearColor));
        m_pContext->ClearRenderTargetView(pRTVs[2], reinterpret_cast<const float*>(&clearColor));
        m_pContext->ClearRenderTargetView(pRTVs[3], reinterpret_cast<const float*>(&clearColor));
     
        auto pGameCam = CGameInstance::Get().GetActiveCamera();
        if (nullptr == pGameCam)    return S_OK;

        ctx.matProj = pGameCam->GetProj();
        ctx.matView = pGameCam->GetView();
        ctx.matViewProj = ctx.matView * ctx.matProj;
        ctx.eye = pGameCam->GetTransform().GetLoadedPostion();

        if (FAILED(Bind_CameraAttribute(pGameCam)))
        {
            return E_FAIL;
        }
    }

    if (FAILED(RenderPriority(ctx)))
    {
        return E_FAIL;
    }

    if (FAILED(RenderNonBlend(ctx)))
    {
        return E_FAIL;
    }
    if (FAILED(RenderLight(ctx)))
    {
        return E_FAIL;
    }
    
    // UnBind RenderTargets
    {
        ID3D11RenderTargetView* pRTVs[4] = { nullptr, nullptr, nullptr, nullptr };
        m_pContext->OMSetRenderTargets(4, pRTVs, nullptr);
    }

    return S_OK;
}
HRESULT CRenderer::RenderPBR(const RENDER_CTX& ctx) {
    ID3D11RenderTargetView* pRTVs[1] = { m_pResDynTexTargetPBR->GetRTV().Get() };
    m_pContext->OMSetRenderTargets(1, pRTVs, nullptr);
    m_pContext->RSSetViewports(1, &m_pBackBufferVP->GetViewPort());

    _float4 clearColor = { 0.f, 0.f, 1.f, 1.f };
    m_pContext->ClearRenderTargetView(pRTVs[0], reinterpret_cast<const float*>(&clearColor));
    m_pContext->ClearDepthStencilView(m_pBackBufferDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
    //SPtr<CResBlendState> BlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_BLEND");
    //m_pContext->OMSetBlendState(BlendState->GetBlendState().Get(), nullptr, 0xffffffff);

    SPtr<CResDepthStencilState> DepthState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHWRITE");
    m_pContext->OMSetDepthStencilState(DepthState->GetDepthStencilState().Get(), 0);

    const auto& FullScreenBuffer = m_pFullscreenVIBuffer;
    m_pContext->VSSetShader(m_pPBRVertexShader->GetVertexShader().Get(), nullptr, 0);
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

    // Draw On PBRScreen
    m_pContext->DrawIndexed(FullScreenBuffer->GetNumIndices(), 0, 0);

    // UnBind RenderTargets
    {
        ID3D11RenderTargetView* pRTVs[4] = { nullptr, nullptr, nullptr, nullptr };
        m_pContext->OMSetRenderTargets(4, pRTVs, nullptr);

        ID3D11ShaderResourceView* pSRVs[1] = { nullptr };
        m_pContext->PSSetShaderResources(0, 1, pSRVs);
        m_pContext->PSSetShaderResources(1, 1, pSRVs);
        m_pContext->PSSetShaderResources(2, 1, pSRVs);
        m_pContext->PSSetShaderResources(3, 1, pSRVs);
        m_pContext->PSSetShaderResources(4, 1, pSRVs);
    }
    return S_OK;
}
HRESULT CRenderer::RenderPBR2(const RENDER_CTX& ctx){
    ID3D11RenderTargetView* pRTVs[1] = { m_pBackBufferRTV.Get() };
    m_pContext->OMSetRenderTargets(1, pRTVs, nullptr);
    m_pContext->RSSetViewports(1, &m_pBackBufferVP->GetViewPort());

    SPtr<CResDepthStencilState> DepthState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHWRITE");
    m_pContext->OMSetDepthStencilState(DepthState->GetDepthStencilState().Get(), 0);

    const auto& FullScreenBuffer = m_pFullscreenVIBuffer;
    m_pContext->VSSetShader(m_pBlendVertexShader->GetVertexShader().Get(), nullptr, 0);
    m_pContext->PSSetShader(m_pBlendPixelShader->GetPixelShader().Get(), nullptr, 0);

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

    // Draw On PBRScreen
    m_pContext->DrawIndexed(FullScreenBuffer->GetNumIndices(), 0, 0);

    // UnBind RenderTargets
    {
        ID3D11RenderTargetView* pRTVs[4] = { nullptr, nullptr, nullptr, nullptr };
        m_pContext->OMSetRenderTargets(4, pRTVs, nullptr);

        ID3D11ShaderResourceView* pSRVs[1] = { nullptr };
        m_pContext->PSSetShaderResources(0, 1, pSRVs);
        m_pContext->PSSetShaderResources(1, 1, pSRVs);
        m_pContext->PSSetShaderResources(2, 1, pSRVs);
        m_pContext->PSSetShaderResources(3, 1, pSRVs);
        m_pContext->PSSetShaderResources(4, 1, pSRVs);
    }

    return S_OK;
}
HRESULT CRenderer::Render_Alpha(RENDER_CTX& ctx) {
    ZoneScopedN("Render_Alpha");
    {
        ID3D11RenderTargetView* pBackBufferRTVs[1] = { m_pResDynTexTargetPBR->GetRTV().Get()};
        m_pContext->OMSetRenderTargets(1, pBackBufferRTVs, m_pResDynTexTargetDepth->GetDSV().Get());
        m_pContext->RSSetViewports(1, &m_pBackBufferVP->GetViewPort());

        SPtr<CResDepthStencilState> DepthState = CGameInstance::Get().GetResourceFirst<CResDepthStencilState>(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHREAD");
        m_pContext->OMSetDepthStencilState(DepthState->GetDepthStencilState().Get(), 0);
    }

    if (FAILED(RenderBlend(ctx)))
    {
        return E_FAIL;
    }
    
    if (FAILED(RenderSkybox(ctx)))
    {
        return E_FAIL;
    }

    if (FAILED(RenderCollider(ctx)))
    {
        return E_FAIL;
    }

    if (FAILED(RenderParticle(ctx)))
    {
        return E_FAIL;
    }
   
    // UnBind RenderTargets
    {
        ID3D11ShaderResourceView* pSRVs[1] = { nullptr };
        m_pContext->PSSetShaderResources(0, 1, pSRVs);
        m_pContext->PSSetShaderResources(1, 1, pSRVs);
        m_pContext->PSSetShaderResources(2, 1, pSRVs);
        m_pContext->PSSetShaderResources(3, 1, pSRVs);
        m_pContext->PSSetShaderResources(4, 1, pSRVs);
    }

    m_pContext->CopyResource(m_pBackBufferCopyTexture.Get(), CGameInstance::Get().GetBackBufferTexture().Get());
   
    return S_OK;
}
HRESULT CRenderer::Render_OffScreen(RENDER_CTX& ctx) {
    ZoneScopedN("Render_OffScreen");
    {
        ID3D11RenderTargetView* pRTVs[1] = { m_pOffScreenTex2D->GetRTV().Get() };
        m_pContext->OMSetRenderTargets(1, pRTVs, nullptr);
        m_pContext->RSSetViewports(1, &m_pBackBufferVP->GetViewPort());

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
            ID3D11ShaderResourceView* pSRVs[1] = { m_pResDynTexTargetPBR->GetSRV().Get() };
            m_pContext->PSSetShaderResources(0, 1, pSRVs);
        }
        // Draw On OffScreen
        m_pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);

        // Unbind Shader Resource : Diffuse
        {
            ID3D11ShaderResourceView* pNullSRVs[1] = { nullptr };
            m_pContext->PSSetShaderResources(0, 1, pNullSRVs);
        }
        // Unbind Shader Resource : Normal
        {
            ID3D11ShaderResourceView* pNullSRVs[1] = { nullptr };
            m_pContext->PSSetShaderResources(1, 1, pNullSRVs);
        }
        // Unbind Shader Resource : Shadow
        {
            ID3D11ShaderResourceView* pShadowSRVs[1] = { nullptr };
            m_pContext->PSSetShaderResources(4, 1, pShadowSRVs);
        }
        // UnBind RenderTargets
        {
            ID3D11RenderTargetView* pRTVs[3] = { nullptr, nullptr, nullptr };
            m_pContext->OMSetRenderTargets(3, pRTVs, nullptr);
        }
    }
    
    return S_OK;
}
HRESULT CRenderer::Render_FullScreen()
{
    ZoneScopedN("DrawFullscreen");
        ID3D11RenderTargetView* pBackBufferRTVs[1] = { m_pBackBufferRTV.Get() };
    m_pContext->OMSetRenderTargets(1, pBackBufferRTVs, nullptr);

    _float4 clearColor = { 1.f, 0.f, 1.f, 1.f };
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
        ID3D11ShaderResourceView* pSRVs[1] = { m_pResDynTexTargetNormal->GetSRV().Get() };
        m_pContext->PSSetShaderResources(0, 1, pSRVs);
    }
    else
    {
        ID3D11ShaderResourceView* pSRVs[1] = { m_pOffScreenTex2D->GetSRV().Get() };
        m_pContext->PSSetShaderResources(0, 1, pSRVs);
    }

    //ID3D11ShaderResourceView* pSRVs[1] = { m_pLastTex2DBeforeFullScreenDraw->GetSRV().Get() };
    //m_pContext->PSSetShaderResources(0, 1, pSRVs);

    const auto& sampler = E::CGameInstance::GetConst().GetResourceFirst<E::CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
    m_pContext->PSSetSamplers(0, 1, sampler->GetSamplerState().GetAddressOf());

    m_pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);

    ID3D11ShaderResourceView* pNullSRVs[1] = { nullptr };
    m_pContext->PSSetShaderResources(0, 1, pNullSRVs);

    return S_OK;
}

HRESULT CRenderer::RenderPriority(const RENDER_CTX& ctx)
{
    ZoneScopedN("RenderPriority");
    for (auto& pRenderObject : m_RenderObject[ETOUI(RENDERGROUP::PRIORITY)])
    {
        if (pRenderObject->HasRenderPass(ctx.pass))
        {
            pRenderObject->Render(m_pContext.Get(), ctx);
        }
    }

    return S_OK;
}

HRESULT CRenderer::RenderNonBlend(const RENDER_CTX& ctx)
{
    ZoneScopedN("RenderNonBlend");
    for (auto& pRenderObject : m_RenderObject[ETOUI(RENDERGROUP::NONBLEND)])
    {
        if (pRenderObject->HasRenderPass(ctx.pass))
        {
            pRenderObject->Render(m_pContext.Get(), ctx);
        }
    }

    return S_OK;
}

HRESULT CRenderer::RenderBlend(const RENDER_CTX& ctx)
{
    ZoneScopedN("RenderBlend");

    auto BlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_BLEND");
    m_pContext->OMSetBlendState(BlendState->GetBlendState().Get(), nullptr, 0xffffffff);

    for (auto& pRenderObject : m_RenderObject[ETOUI(RENDERGROUP::BLEND)])
    {
        if (pRenderObject->HasRenderPass(ctx.pass))
        {
            pRenderObject->Render(m_pContext.Get(), ctx);
        }
    }

    BlendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(TAG_RES_GRP_PERMANENT_STATE, "BS_BLEND_NONE");
    m_pContext->OMSetBlendState(BlendState->GetBlendState().Get(), nullptr, 0xffffffff);

    return S_OK;
}

HRESULT CRenderer::RenderLight(const RENDER_CTX& ctx) {
    for (auto& pRenderObject : m_RenderObject[ETOUI(RENDERGROUP::LIGHT)])
    {
        if (pRenderObject->HasRenderPass(ctx.pass))
        {
            pRenderObject->Render(m_pContext.Get(), ctx);
        }
    }
    return S_OK;
}

HRESULT CRenderer::RenderSkybox(const RENDER_CTX& ctx)
{
    ZoneScopedN("RenderSkybox");
    for (auto& pRenderObject : m_RenderObject[ETOUI(RENDERGROUP::SKYBOX)])
    {
        if (pRenderObject->HasRenderPass(ctx.pass))
        {
            pRenderObject->Render(m_pContext.Get(), ctx);
        }
    }

    return S_OK;
}

HRESULT CRenderer::RenderCollider(const RENDER_CTX& ctx)
{
    ZoneScopedN("RenderCollider");

    for (auto& pRenderObject : m_RenderObject[ETOUI(RENDERGROUP::COLLIDER)])
    {
        if (pRenderObject->HasRenderPass(ctx.pass))
        {
            pRenderObject->Render(m_pContext.Get(), ctx);
        }
    }

    return S_OK;
}

HRESULT CRenderer::RenderParticle(const RENDER_CTX& ctx)
{
    //MRT
    //emissive
    const auto& blendState = CGameInstance::Get().GetResourceFirst<CResBlendState>(
        TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_BLEND");

    if (!blendState)
        return E_FAIL;
    if (blendState)
        m_pContext->OMSetBlendState(blendState->GetBlendState().Get(), nullptr, 0xffffffff);

    for (auto& pRenderObject : m_RenderObject[ETOUI(RENDERGROUP::PARTICLE)])
    {
        if (pRenderObject->HasRenderPass(ctx.pass))
        {
            pRenderObject->Render(m_pContext.Get(), ctx);
        }
    }

    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    return S_OK;
}

HRESULT CRenderer::RenderPostProcess(const RENDER_CTX& ctx){
    ID3D11RenderTargetView* pRTVs[1] = { m_pResDynTexTargetPostProcess->GetRTV().Get() };
    m_pContext->OMSetRenderTargets(1, pRTVs, nullptr);
    m_pContext->RSSetViewports(1, &m_pBackBufferVP->GetViewPort());

    auto pCbPostProcess = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PostProcess");

    D3D11_MAPPED_SUBRESOURCE MRES;
    if (SUCCEEDED(m_pContext->Map(pCbPostProcess->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
    {
        POSTPROCESS CBPP{};
        CBPP.DistortionIntensity = m_pDistortionIntensity;
        CBPP.ChromaticIntensity = m_pChromaticIntensity;
        CBPP.VignetteIntensity = m_pVignetteIntensity;
        CBPP.VignetteSmoothness = m_pVignetteSmoothness;

        memcpy(MRES.pData, &CBPP, sizeof(POSTPROCESS));
        m_pContext->Unmap(pCbPostProcess->GetCBuffer().Get(), 0);
    }

    m_pContext->PSSetShader(m_pPostProcessPS->GetPixelShader().Get(), nullptr, 0);

    m_pContext->PSSetConstantBuffers(0, 1, pCbPostProcess->GetCBuffer().GetAddressOf());
    m_pContext->PSSetShaderResources(0, 1, m_pOffScreenTex2D->GetSRV().GetAddressOf());    // Combined Texture
    m_pContext->PSSetShaderResources(1, 1, m_pLUTTexture.GetAddressOf());                  // LUT Texture

    m_pContext->DrawIndexed(m_pFullscreenVIBuffer->GetNumIndices(), 0, 0);

    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    m_pContext->PSSetShaderResources(0, 1, NullSRV);
    m_pContext->PSSetShaderResources(1, 1, NullSRV);
    
    return S_OK;
}

HRESULT CRenderer::RenderUI(const RENDER_CTX& ctx)
{
    for (auto& pRenderObject : m_RenderObject[ETOUI(RENDERGROUP::UI)])
    {
        if (pRenderObject->HasRenderPass(ctx.pass))
        {
            pRenderObject->Render(m_pContext.Get(), ctx);
        }
    }

    {
        E::CGameInstance::Get().FontLateDraw(RENDERGROUP::UI);
    }

    return S_OK;
}



HRESULT CRenderer::Bind_CameraAttribute(CCameraObject* _ActiveCam) {
    auto pCbPerPass = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PASS);
    D3D11_MAPPED_SUBRESOURCE mappedSubResource;
    if (SUCCEEDED(m_pContext->Map(pCbPerPass->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
    {
        CB_PER_PASS cbPerPass{};
        XMStoreFloat4x4(&cbPerPass.matProj, _ActiveCam->GetProj());
        XMStoreFloat4x4(&cbPerPass.matView, _ActiveCam->GetView());
        XMStoreFloat4x4(&cbPerPass.matViewProj, _ActiveCam->GetView() * _ActiveCam->GetProj());
        XMStoreFloat4x4(&cbPerPass.matInvView, XMMatrixInverse(nullptr, _ActiveCam->GetView()));
        XMStoreFloat4x4(&cbPerPass.matInvViewProj, XMMatrixInverse(nullptr, XMLoadFloat4x4(&cbPerPass.matViewProj)));
        cbPerPass.vCamPos = _ActiveCam->GetTransform().GetPosition();

        auto pShadowCamera = CGameInstance::Get().GetCamera("Shadow");

        if (nullptr != pShadowCamera) {
            XMStoreFloat4x4(&cbPerPass.matShadowLightViewProj, pShadowCamera->GetView() * pShadowCamera->GetProj());
        }

        memcpy(mappedSubResource.pData, &cbPerPass, sizeof(cbPerPass));
        m_pContext->Unmap(pCbPerPass->GetCBuffer().Get(), 0);
    }
    m_pContext->VSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
    m_pContext->PSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
    m_pContext->GSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());

    return S_OK;
}

#ifdef _DEBUG
VOID CRenderer::PostProcessGUI() {
    ImGui::Begin("PostProcess");

    if (ApplyFilter ? ImGui::Button("PostProcess ON") : ImGui::Button("PostProcess OFF")) {
        ApplyFilter = !ApplyFilter;
    }

    ImGui::InputFloat("DistortionIntensity", &m_pDistortionIntensity);
    ImGui::InputFloat("ChromaticIntensity", &m_pChromaticIntensity);
    ImGui::InputFloat("VignetteIntensity", &m_pVignetteIntensity);
    ImGui::InputFloat("VignetteSmoothness", &m_pVignetteSmoothness);

    ImGui::End();
}
HRESULT CRenderer::Initialize_Debugging()
{
    m_pDebugBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");
    if (!m_pDebugBuffer)
    {
        return E_FAIL;
    }
    m_pDebugVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTex");
    if (FAILED(m_pDebugVertexShader->Load()))
    {
        return E_FAIL;
    }
    if (m_pDebugPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTex")) {
        if (FAILED(m_pDebugPixelShader->Load(CResShader::DESC{ .sEntryPoint = "PSMain_NonAlpha", .sTarget = "ps_5_0" }))) return E_FAIL;
    }

    XMFLOAT2	vViewportSize   = { m_pBackBufferVP->GetViewPort().Width , m_pBackBufferVP->GetViewPort().Height };
    XMFLOAT2    vDebugViewSize  = { vViewportSize.x / 4.f, vViewportSize.y / 4.f };
    XMMATRIX    mDebugViewScaleMatrix   = XMMatrixScaling(vDebugViewSize.x, vDebugViewSize.y, 1.f);

    XMFLOAT2    vDebugViewStartPoint    = { vDebugViewSize.x * 0.5f - vViewportSize.x * 0.5f, -vDebugViewSize.y * 0.5f + vViewportSize.y * 0.5f };

    m_pResDynTexTargetList.push_back(m_pResDynTexTargetDiffuse);
    m_pResDynTexTargetList.push_back(m_pResDynTexTargetNormal);
    m_pResDynTexTargetList.push_back(m_pResDynTexTargetSMRO);
    m_pResDynTexTargetList.push_back(m_pResDynTexTargetEmissive);
    m_pResDynTexTargetList.push_back(m_pResDynTexTargetPBR);
    m_pResDynTexTargetList.push_back(m_pResDynTexTargetPostProcess); 

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

HRESULT CRenderer::Render_Debugging(const RENDER_CTX& ctx) {
    if (CGameInstance::Get().KeyDown(DIK_F3))
        m_bRenderable = !m_bRenderable;

    if (!m_bRenderable) return S_OK;

    XMFLOAT2	vViewportSize = { m_pBackBufferVP->GetViewPort().Width, m_pBackBufferVP->GetViewPort().Height };

    XMMATRIX    m_WorldMatrix, m_ViewMatrix, m_ProjMatrix;
    m_ViewMatrix = XMMatrixIdentity();
    m_ProjMatrix = XMMatrixOrthographicLH(vViewportSize.x, vViewportSize.y, 0.f, 1.f);

    auto pCbPerPass = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PASS);
    D3D11_MAPPED_SUBRESOURCE mappedSubResource;
    if (SUCCEEDED(m_pContext->Map(pCbPerPass->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
    {
        CB_PER_PASS cbPerPass{};

        XMStoreFloat4x4(&cbPerPass.matProj, m_ProjMatrix);
        XMStoreFloat4x4(&cbPerPass.matView, m_ViewMatrix);
        XMStoreFloat4x4(&cbPerPass.matViewProj, m_ViewMatrix * m_ProjMatrix);
        XMStoreFloat4x4(&cbPerPass.matInvView, XMMatrixInverse(nullptr, m_ViewMatrix));
        XMStoreFloat4x4(&cbPerPass.matInvViewProj, XMMatrixInverse(nullptr, XMLoadFloat4x4(&cbPerPass.matViewProj)));
        cbPerPass.vCamPos = XMFLOAT3(0.f, 0.f, -1.f);

        memcpy(mappedSubResource.pData, &cbPerPass, sizeof(cbPerPass));
        m_pContext->Unmap(pCbPerPass->GetCBuffer().Get(), 0);
    }
    auto pPBufferPtr = pCbPerPass->GetCBuffer().GetAddressOf();
    m_pContext->VSSetConstantBuffers(1, 1, pPBufferPtr);
    m_pContext->PSSetConstantBuffers(1, 1, pPBufferPtr);
    m_pContext->GSSetConstantBuffers(1, 1, pPBufferPtr);

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
    D3D11_MAPPED_SUBRESOURCE MRES;

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
        m_pContext->GSSetConstantBuffers(0, 1, pCbPerObject->GetCBuffer().GetAddressOf());
        if (IDX == 8) {
            m_pContext->PSSetShaderResources(0, 1, m_pBackBufferCopySRV.GetAddressOf());
        }
        else {
            m_pContext->PSSetShaderResources(0, 1, m_pResDynTexTargetList[IDX]->GetSRV().GetAddressOf());
        }
        
        m_pContext->DrawIndexed(m_pDebugBuffer->GetNumIndices(), 0, 0);
    }

    return S_OK;
}
#endif

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