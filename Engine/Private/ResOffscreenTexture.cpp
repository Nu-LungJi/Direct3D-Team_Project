#include "pch.h"
#include "ResOffscreenTexture.h"
#include "GameInstance.h"
NS_USING(Engine)

CResOffscreenTexture::CResOffscreenTexture(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
    : CResTexture{ sPath, pDevice, pContext } 
{
}

CResOffscreenTexture::~CResOffscreenTexture()
{
}

HRESULT CResOffscreenTexture::Load(const std::any& arg)
{
    auto descArg = std::any_cast<DESC>(&arg);
    if (!descArg)
    {
        return E_FAIL;
    }

    if (m_eState == STATE::LOADED)
    {
        return S_OK;
    }

    m_eState = STATE::LOADING;

    {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = descArg->iWidth;
        desc.Height = descArg->iHeight;
        desc.Format = descArg->eFormat;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;


        if (FAILED(m_pDevice->CreateTexture2D(&desc, 0, m_pTexture.GetAddressOf())))
        {
            m_eState = STATE::LOADFAIL;
            MSG_BOX("CResOffscreenTexture CreateTexture2D");
            return E_FAIL;
        }

        if (FAILED(m_pDevice->CreateShaderResourceView(m_pTexture.Get(), 0, &m_pSRV)))
        {
            m_eState = STATE::LOADFAIL;
            MSG_BOX("CResOffscreenTexture CreateShaderResourceView");
            return E_FAIL;
        }

        if (FAILED(m_pDevice->CreateRenderTargetView(m_pTexture.Get(), 0, &m_pRTV)))
        {
            m_eState = STATE::LOADFAIL;
            MSG_BOX("CResOffscreenTexture CreateRenderTargetView");
            return E_FAIL;
        }

        if (FAILED(m_pDevice->CreateUnorderedAccessView(m_pTexture.Get(), 0, &m_pUAV)))
        {
            m_eState = STATE::LOADFAIL;
            MSG_BOX("CResOffscreenTexture CreateUnorderedAccessView");
            return E_FAIL;
        }
    }

    m_eState = STATE::LOADED;
    return S_OK;
}

HRESULT CResOffscreenTexture::Unload(const std::any& arg)
{
    return S_OK;
}

SPtr<CResOffscreenTexture> CResOffscreenTexture::Create()
{
    return ToSPtr(new CResOffscreenTexture{ "", CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext() });
}
