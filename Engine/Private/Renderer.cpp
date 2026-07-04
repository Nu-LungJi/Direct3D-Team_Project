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

    PostProcessGUI();
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

    if (FAILED(InitializeTargetDiffuse()))
    {
        return E_FAIL;
    }

    if (FAILED(InitializeTargetNormal()))
    {
        return E_FAIL;
    }

    if (FAILED(InitilizePostProcess()))
    {
        return E_FAIL;
    }

#ifdef _DEBUG
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
    m_pDebugPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTex");
    if (FAILED(m_pDebugPixelShader->Load(CResShader::DESC{.sEntryPoint = "PSMain_NonAlpha", .sTarget = "ps_5_0"})))
    {
        return E_FAIL;
    }

    if (FAILED(Initialize_Debugging()))
    {
        return E_FAIL;
    }
#endif

    return S_OK;
}

HRESULT CRenderer::InitializeOffscreen()
{
    // offscreenTexture
    {
        auto vClientScreenSize = CGameInstance::Get().GetClientScreenSize();

        if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_TEXTURE, "DynTex2D_Offscreen", E::CResDynamicTexture2D::Create()))
        {
            CResDynamicTexture2D::DESC Desc{};
            Desc.texDesc = {
                .Width = (UINT)vClientScreenSize.x,
                .Height = (UINT)vClientScreenSize.y,
                .MipLevels = 1,
                .ArraySize = 1,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .SampleDesc = {.Count = 1, .Quality = 0 },
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
                .CPUAccessFlags = 0,
                .MiscFlags = 0
            };
            if (FAILED(res->Load(Desc)))
            {
                return E_FAIL;
            }
            if (FAILED(res->CreateSRV()))
            {
                return E_FAIL;
            }
            if (FAILED(res->CreateRTV()))
            {
                return E_FAIL;
            }
            m_pOffScreenTex2D = res;
        }
        
    }

    {
        if (auto res = CGameInstance::Get().AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_OffscreenCombined", "./ShaderFiles/Deferred Rendering/VS_Deferred.hlsl"))
        {
            if (FAILED(res->Load()))
            {
                return E_FAIL;
            }
            m_pOffScreenVertexShader = res;
        }
        if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_OffscreenCombined", "./ShaderFiles/Deferred Rendering/PS_Deferred.hlsl"))
        {
            if (FAILED(res->Load()))
            {
                return E_FAIL;
            }
            m_pOffScreenPixelShader = res;
        }
    } 

    return S_OK;
}

HRESULT CRenderer::InitializeShadow()
{
    UINT iShadowWidth   = 2048 * 2;
    UINT iShadowHeight  = 2048 * 2;

    if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_TEXTURE, "DynTex2D_Shadow", E::CResDynamicTexture2D::Create()))
    {
        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width = iShadowWidth;
        texDesc.Height = iShadowHeight;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags = 0;

        CResDynamicTexture2D::DESC DynTex2DDesc{};
        DynTex2DDesc.texDesc = texDesc;
        if (FAILED(res->Load(DynTex2DDesc)))
        {
            return E_FAIL;
        }

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
        if (FAILED(res->CreateDSV(dsvDesc)))
        {
            return E_FAIL;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        if (FAILED(res->CreateSRV(srvDesc)))
        {
            return E_FAIL;
        }

        m_pShadowTex2D = res;
    }
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
    m_pFullscreenVS         = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_FullscreenQuad");
    m_pFullscreenPS         = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_FullscreenQuad");
    m_pFullscreenVIBuffer   = E::CGameInstance::Get().GetResourceFirst<E::CResVIBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_FullscreenTex");

    return S_OK;
}

HRESULT CRenderer::InitializeTargetDiffuse()
{
    // diffuseTarget
    {
        auto vClientScreenSize = CGameInstance::Get().GetClientScreenSize();

        if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_TEXTURE, "DynTex2D_Target_Diffuse", E::CResDynamicTexture2D::Create()))
        {
            CResDynamicTexture2D::DESC Desc{};
            Desc.texDesc = {
                .Width = (UINT)vClientScreenSize.x,
                .Height = (UINT)vClientScreenSize.y,
                .MipLevels = 1,
                .ArraySize = 1,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .SampleDesc = {.Count = 1, .Quality = 0 },
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
                .CPUAccessFlags = 0,
                .MiscFlags = 0
            };
            if (FAILED(res->Load(Desc)))
            {
                return E_FAIL;
            }
            if (FAILED(res->CreateSRV()))
            {    
                return E_FAIL;
            }
            if (FAILED(res->CreateRTV()))
            {
                return E_FAIL;
            }
            m_pResDynTexTargetDiffuse = res;
        }
    }

    return S_OK;
}

HRESULT CRenderer::InitializeTargetNormal()
{
    // normalTarget
    {
        auto vClientScreenSize = CGameInstance::Get().GetClientScreenSize();

        if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_TEXTURE, "DynTex2D_Target_Normal", E::CResDynamicTexture2D::Create()))
        {
            CResDynamicTexture2D::DESC Desc{};
            Desc.texDesc = {
                .Width = (UINT)vClientScreenSize.x,
                .Height = (UINT)vClientScreenSize.y,
                .MipLevels = 1,
                .ArraySize = 1,
                .Format = DXGI_FORMAT_R16G16B16A16_UNORM,
                .SampleDesc = {.Count = 1, .Quality = 0 },
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
                .CPUAccessFlags = 0,
                .MiscFlags = 0
            };
            if (FAILED(res->Load(Desc)))
            {
                return E_FAIL;
            }
            if (FAILED(res->CreateSRV()))
            {
                return E_FAIL;
            }
            if (FAILED(res->CreateRTV()))
            {
                return E_FAIL;
            }
            m_pResDynTexTargetNormal = res;
        }
    }
    return S_OK;
}

HRESULT CRenderer::InitilizePostProcess(){
    // PostProcess
    {
        auto vClientScreenSize = CGameInstance::Get().GetClientScreenSize();

        if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_TEXTURE, "DynTex2D_PostProcess", E::CResDynamicTexture2D::Create()))
        {
            CResDynamicTexture2D::DESC Desc{};
            Desc.texDesc = {
                .Width = (UINT)vClientScreenSize.x,
                .Height = (UINT)vClientScreenSize.y,
                .MipLevels = 1,
                .ArraySize = 1,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .SampleDesc = {.Count = 1, .Quality = 0 },
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
                .CPUAccessFlags = 0,
                .MiscFlags = 0
            };
            if (FAILED(res->Load(Desc)))
            {
                return E_FAIL;
            }
            if (FAILED(res->CreateSRV()))
            {
                return E_FAIL;
            }
            if (FAILED(res->CreateRTV()))
            {
                return E_FAIL;
            }
            m_pResDynTexTargetPostProcess = res;
        }
    }

    // PixelShader Create
    if (auto res = CGameInstance::Get().AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PostProcess", "./ShaderFiles/PostProcess/PS_PostProcess_Filter.hlsl"))
    {
        if (FAILED(res->Load()))    return E_FAIL;
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
    
    m_pPostProcessPS = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_PostProcess");

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

HRESULT CRenderer::Draw() {
    ZoneScopedN("Draw");

    RENDER_CTX ctx{};

    _bool bApplyShadow = false;
    if (bApplyShadow)
        if (FAILED(Render_ShadowMap(ctx)))  return E_FAIL;

    // MRT Draw
    
    // Diffuse + Normal
    if (FAILED(Render_DiffuseNormal(ctx)))  return E_FAIL;
    
    // Combined
    if (FAILED(Render_OffScreen(ctx)))      return E_FAIL;

    // PostProcess
    if (FAILED(RenderPostProcess(ctx)))     return E_FAIL;

    {
        m_pLastTex2DBeforeFullScreenDraw = ApplyFilter ? m_pResDynTexTargetPostProcess : m_pOffScreenTex2D;
    }

    // FullScreen : Final
    if (FAILED(Render_FullScreen()))           return E_FAIL;

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
HRESULT CRenderer::Render_DiffuseNormal(RENDER_CTX& ctx) {
    ZoneScopedN("Render_DiffuseNormal");
    {
        ID3D11RenderTargetView* pRTVs[2] = { m_pResDynTexTargetDiffuse->GetRTV().Get(), m_pResDynTexTargetNormal->GetRTV().Get() };
        m_pContext->OMSetRenderTargets(2, pRTVs, m_pBackBufferDSV.Get());
        m_pContext->RSSetViewports(1, &m_pBackBufferVP->GetViewPort());

        ctx.pass = RENDERPASS::DEFAULT;
        _float4 clearColor = { 0.f, 0.f, 1.f, 1.f };
        m_pContext->ClearRenderTargetView(pRTVs[0], reinterpret_cast<const float*>(&clearColor));
        m_pContext->ClearRenderTargetView(pRTVs[1], reinterpret_cast<const float*>(&clearColor));
        m_pContext->ClearDepthStencilView(m_pBackBufferDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        auto pGameCam = CGameInstance::Get().GetActiveCamera();
        if (nullptr == pGameCam)    return S_OK;

        ctx.matProj = pGameCam->GetProj();
        ctx.matView = pGameCam->GetView();
        ctx.matViewProj = ctx.matView * ctx.matProj;
        ctx.eye = pGameCam->GetTransform().GetLoadedPostion();

        auto pShadowCamera = CGameInstance::Get().GetCamera("Shadow");

        if (FAILED(Bind_CameraAttribute(pGameCam, pShadowCamera)))
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

    //UI
    {
        auto pUICame = CGameInstance::Get().GetCamera("UI");
        if (nullptr == pUICame) return S_OK;

        ctx.matProj = pUICame->GetProj();
        ctx.matView = pUICame->GetView();
        ctx.matViewProj = ctx.matView * ctx.matProj;
        ctx.eye = pUICame->GetTransform().GetLoadedPostion();

        if (FAILED(Bind_CameraAttribute(pUICame, nullptr)))
        {
            return E_FAIL;
        }
        if (FAILED(RenderUI(ctx)))
        {
            return E_FAIL;
        }
    }

    // UnBind RenderTargets
    {
        ID3D11RenderTargetView* pRTVs[2] = { nullptr, nullptr };
        m_pContext->OMSetRenderTargets(2, pRTVs, nullptr);
    }

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
            ID3D11ShaderResourceView* pSRVs[1] = { m_pResDynTexTargetDiffuse->GetSRV().Get() };
            m_pContext->PSSetShaderResources(0, 1, pSRVs);
        }
        {
            ID3D11ShaderResourceView* pSRVs[1] = { m_pResDynTexTargetNormal->GetSRV().Get() };
            m_pContext->PSSetShaderResources(1, 1, pSRVs);
        }
        {
            ID3D11ShaderResourceView* pShadowSRVs[1] = { m_pShadowTex2D->GetSRV().Get() };
            m_pContext->PSSetShaderResources(4, 1, pShadowSRVs);
        }

        // Bind PixelShader Sampler
        {
            const auto& sampler = E::CGameInstance::GetConst().GetResourceFirst<E::CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
            m_pContext->PSSetSamplers(0, 1, sampler->GetSamplerState().GetAddressOf());
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
        ID3D11ShaderResourceView* pSRVs[1] = { m_pResDynTexTargetNormal->GetSRV().Get() };
        m_pContext->PSSetShaderResources(0, 1, pSRVs);
    }
    else
    {
        ID3D11ShaderResourceView* pSRVs[1] = { m_pLastTex2DBeforeFullScreenDraw->GetSRV().Get() };
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
    for (auto& pRenderObject : m_RenderObject[ETOUI(RENDERGROUP::BLEND)])
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

HRESULT CRenderer::Bind_CameraAttribute(CCameraObject* _ActiveCam, CCameraObject* _ShadowCam) {
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

        if (nullptr != _ShadowCam) {
            XMStoreFloat4x4(&cbPerPass.matShadowLightViewProj, _ShadowCam->GetView() * _ShadowCam->GetProj());
        }

        memcpy(mappedSubResource.pData, &cbPerPass, sizeof(cbPerPass));
        m_pContext->Unmap(pCbPerPass->GetCBuffer().Get(), 0);
    }
    m_pContext->VSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
    m_pContext->PSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
    m_pContext->GSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());

    return S_OK;
}

VOID CRenderer::PostProcessGUI() {
#ifdef _DEBUG
    ImGui::Begin("PostProcess");

    if (ApplyFilter ? ImGui::Button("PostProcess ON") : ImGui::Button("PostProcess OFF")) {
        ApplyFilter = !ApplyFilter;
    }

    ImGui::InputFloat("DistortionIntensity", &m_pDistortionIntensity);
    ImGui::InputFloat("ChromaticIntensity", &m_pChromaticIntensity);
    ImGui::InputFloat("VignetteIntensity", &m_pVignetteIntensity);
    ImGui::InputFloat("VignetteSmoothness", &m_pVignetteSmoothness);

    ImGui::End();
#endif // _DEBUG
}

#ifdef _DEBUG
HRESULT CRenderer::Initialize_Debugging()
{
    uint32_t    iNumView = 4;

    XMFLOAT2	vViewportSize   = { m_pBackBufferVP->GetViewPort().Width , m_pBackBufferVP->GetViewPort().Height };
    XMFLOAT2    vDebugViewSize  = { vViewportSize.x / 4.f, vViewportSize.y / 4.f };
    XMMATRIX    mDebugViewScaleMatrix   = XMMatrixScaling(vDebugViewSize.x, vDebugViewSize.y, 1.f);

    XMFLOAT2    vDebugViewStartPoint    = { vDebugViewSize.x * 0.5f - vViewportSize.x * 0.5f, -vDebugViewSize.y * 0.5f + vViewportSize.y * 0.5f };

    for (uint32_t i = 0; i < iNumView; i++)
    {
        _float fScreenPosX = vDebugViewStartPoint.x + (static_cast<_float>(i % 2) * vDebugViewSize.x);
        _float fScreenPosY = vDebugViewStartPoint.y - (static_cast<_float>(i / 2) * vDebugViewSize.y);

        XMStoreFloat4x4(&m_fDebugWorldMatrix[i], mDebugViewScaleMatrix * XMMatrixTranslation(fScreenPosX, fScreenPosY, 0.f));
    }

    m_pResDynTexTargetList.push_back(m_pResDynTexTargetDiffuse);
    m_pResDynTexTargetList.push_back(m_pResDynTexTargetNormal);
    m_pResDynTexTargetList.push_back(m_pResDynTexTargetPostProcess);
    m_pResDynTexTargetList.push_back(m_pOffScreenTex2D);

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

    for (uint32_t IDX = 0; IDX < 8; ++IDX) {

        if (IDX >= m_pResDynTexTargetList.size() || !m_pResDynTexTargetList[IDX])
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

        m_pContext->PSSetShaderResources(0, 1, m_pResDynTexTargetList[IDX]->GetSRV().GetAddressOf());

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
