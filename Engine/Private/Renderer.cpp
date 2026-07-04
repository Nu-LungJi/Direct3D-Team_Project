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
            m_pFilteredTex2D = res;
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

HRESULT CRenderer::Draw()
{
    ZoneScopedN("Draw");

    RENDER_CTX ctx{};

    CCameraObject* pShadowCamera{};
    _bool bApplyShadow = false;

    if (bApplyShadow)
    {
        //draw shadow texture
        {
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
                pShadowCamera = CGameInstance::Get().GetCamera("Shadow");
                if (pShadowCamera)
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

                    if (FAILED(RenderNonBlend(ctx)))
                    {
                        return E_FAIL;
                    }
                }
            }
        }
    } // bApplyShadow

    // MRT Draw
    {
        // Diffuse + Normal + TODO
        ID3D11RenderTargetView* pRTVs[2] = { m_pResDynTexTargetDiffuse->GetRTV().Get(), m_pResDynTexTargetNormal->GetRTV().Get() };
        m_pContext->OMSetRenderTargets(2, pRTVs, m_pBackBufferDSV.Get());
        m_pContext->RSSetViewports(1, &m_pBackBufferVP->GetViewPort());

        ctx.pass = RENDERPASS::DEFAULT;
        _float4 clearColor = { 0.f, 0.f, 1.f, 1.f };
        m_pContext->ClearRenderTargetView(pRTVs[0], reinterpret_cast<const float*>(&clearColor));
        m_pContext->ClearRenderTargetView(pRTVs[1], reinterpret_cast<const float*>(&clearColor));
        m_pContext->ClearDepthStencilView(m_pBackBufferDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        {
            auto pGameCam = CGameInstance::Get().GetActiveCamera();
            if (!pGameCam)
            {
                //MSG_BOX("Renderer Draw(), Activated Camera is Null");
                return S_OK;
            }
            ctx.matProj = pGameCam->GetProj();
            ctx.matView = pGameCam->GetView();
            ctx.matViewProj = ctx.matView * ctx.matProj;
            ctx.eye = pGameCam->GetTransform().GetLoadedPostion();

            auto pCbPerPass = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PASS);
            D3D11_MAPPED_SUBRESOURCE mappedSubResource;
            if (SUCCEEDED(m_pContext->Map(pCbPerPass->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
            {
                CB_PER_PASS cbPerPass{};
                XMStoreFloat4x4(&cbPerPass.matProj, pGameCam->GetProj());
                XMStoreFloat4x4(&cbPerPass.matView, pGameCam->GetView());
                XMStoreFloat4x4(&cbPerPass.matViewProj, pGameCam->GetView() * pGameCam->GetProj());
                XMStoreFloat4x4(&cbPerPass.matInvView, XMMatrixInverse(nullptr, pGameCam->GetView()));
                XMStoreFloat4x4(&cbPerPass.matInvViewProj, XMMatrixInverse(nullptr, XMLoadFloat4x4(&cbPerPass.matViewProj)));
                cbPerPass.vCamPos = pGameCam->GetTransform().GetPosition();

                if (pShadowCamera)
                {
                    XMStoreFloat4x4(&cbPerPass.matShadowLightViewProj, pShadowCamera->GetView() * pShadowCamera->GetProj());
                }


                memcpy(mappedSubResource.pData, &cbPerPass, sizeof(cbPerPass));
                m_pContext->Unmap(pCbPerPass->GetCBuffer().Get(), 0);
            }
            m_pContext->VSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
            m_pContext->PSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
            m_pContext->GSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());

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

        //UI
        {
            if (auto pUICame = CGameInstance::Get().GetCamera("UI"))
            {
                {
                    ctx.matProj = pUICame->GetProj();
                    ctx.matView = pUICame->GetView();
                    ctx.matViewProj = ctx.matView * ctx.matProj;
                    ctx.eye = pUICame->GetTransform().GetLoadedPostion();

                    auto pCbPerPass = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PASS);
                    D3D11_MAPPED_SUBRESOURCE mappedSubResource;
                    if (SUCCEEDED(m_pContext->Map(pCbPerPass->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
                    {
                        CB_PER_PASS cbPerPass{};
                        XMStoreFloat4x4(&cbPerPass.matProj, pUICame->GetProj());
                        XMStoreFloat4x4(&cbPerPass.matView, pUICame->GetView());
                        XMStoreFloat4x4(&cbPerPass.matViewProj, pUICame->GetView() * pUICame->GetProj());
                        XMStoreFloat4x4(&cbPerPass.matInvView, XMMatrixInverse(nullptr, pUICame->GetView()));
                        //XMStoreFloat4x4(&cbPerFrame.matInvViewProj, XMMatrixInverse(nullptr, XMLoadFloat4x4(&cbPerFrame.matViewProj)));
                        cbPerPass.vCamPos = pUICame->GetTransform().GetPosition();
                        memcpy(mappedSubResource.pData, &cbPerPass, sizeof(cbPerPass));
                        m_pContext->Unmap(pCbPerPass->GetCBuffer().Get(), 0);
                    }
                    m_pContext->VSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
                    m_pContext->PSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
                    m_pContext->GSSetConstantBuffers(1, 1, pCbPerPass->GetCBuffer().GetAddressOf());
                }
                if (FAILED(RenderUI(ctx)))
                {
                    return E_FAIL;
                }
            }
        }

        // unbind rt
        {
            ID3D11RenderTargetView* pRTVs[2] = { nullptr, nullptr };
            m_pContext->OMSetRenderTargets(2, pRTVs, nullptr);
        }
    }
    
    // offscreen combined 
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

        {
            ID3D11ShaderResourceView* pSRVs[1] = { m_pResDynTexTargetDiffuse->GetSRV().Get() };
            m_pContext->PSSetShaderResources(0, 1, pSRVs);
        }
        {
            ID3D11ShaderResourceView* pSRVs[1] = { m_pResDynTexTargetNormal->GetSRV().Get() };
            m_pContext->PSSetShaderResources(1, 1, pSRVs);
        }
        // binding shadow map
        {
            ID3D11ShaderResourceView* pShadowSRVs[1] = { m_pShadowTex2D->GetSRV().Get() };
            m_pContext->PSSetShaderResources(4, 1, pShadowSRVs);
        }

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
    }
    
    {
        ID3D11RenderTargetView* pRTVs[3] = { nullptr,nullptr,nullptr };
        m_pContext->OMSetRenderTargets(3, pRTVs, nullptr);
    }

    // PostProcess
    if (FAILED(RenderPostProcess(ctx)))
    {
        return E_FAIL;
    }

    {
        m_pLastTex2DBeforeFullScreenDraw = ApplyFilter ? m_pFilteredTex2D : m_pOffScreenTex2D;
    }

    // draw fullscreen
    {
        if (FAILED(DrawFullscreen()))
        {
            return E_FAIL;
        }
    }

    return S_OK;
}

void CRenderer::FrameEnd()
{
    for (auto& vecRenderables : m_RenderObject)
    {
        vecRenderables.clear();
    }
}

HRESULT CRenderer::DrawFullscreen()
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

HRESULT CRenderer::RenderPostProcess(const RENDER_CTX& ctx){
    ID3D11RenderTargetView* pRTVs[1] = { m_pFilteredTex2D->GetRTV().Get() };
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
