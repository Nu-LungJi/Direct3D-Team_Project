#include "pch.h"
#include "ResDynamicTexture2D.h"
#include "GameInstance.h"

NS_USING(Engine)

CResDynamicTexture2D::CResDynamicTexture2D(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
    : CResTexture{ sPath, pDevice, pContext } 
{
}

CResDynamicTexture2D::~CResDynamicTexture2D()
{
}

HRESULT CResDynamicTexture2D::CreateSRV(const D3D11_SHADER_RESOURCE_VIEW_DESC& desc, size_t idx)
{
	return m_pDevice->CreateShaderResourceView(m_pTexture.Get(), &desc, &m_pSRV[idx]);
}

HRESULT CResDynamicTexture2D::CreateSRV(size_t idx)
{
	return m_pDevice->CreateShaderResourceView(m_pTexture.Get(), 0, &m_pSRV[idx]);
}

HRESULT CResDynamicTexture2D::CreateUAV(const D3D11_UNORDERED_ACCESS_VIEW_DESC& desc, size_t idx)
{
    return m_pDevice->CreateUnorderedAccessView(m_pTexture.Get(), &desc, &m_pUAV[idx]);
}

HRESULT CResDynamicTexture2D::CreateUAV(size_t idx)
{
    return m_pDevice->CreateUnorderedAccessView(m_pTexture.Get(), 0, &m_pUAV[idx]);
}

HRESULT CResDynamicTexture2D::CreateDSV(const D3D11_DEPTH_STENCIL_VIEW_DESC& desc, size_t idx)
{
    return m_pDevice->CreateDepthStencilView(m_pTexture.Get(), &desc, &m_pDSV[idx]);
}

HRESULT CResDynamicTexture2D::CreateDSV( size_t idx)
{
    return m_pDevice->CreateDepthStencilView(m_pTexture.Get(), 0, &m_pDSV[idx]);
}

HRESULT CResDynamicTexture2D::CreateRTV(const D3D11_RENDER_TARGET_VIEW_DESC& desc, size_t idx)
{
    return m_pDevice->CreateRenderTargetView(m_pTexture.Get(), &desc, &m_pRTV[idx]);
}

HRESULT CResDynamicTexture2D::CreateRTV(size_t idx)
{
    return m_pDevice->CreateRenderTargetView(m_pTexture.Get(), 0, &m_pRTV[idx]);
}

HRESULT CResDynamicTexture2D::Load(const std::any& arg)
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
        auto param2 = descArg->texSubResource.pSysMem != nullptr ? &descArg->texSubResource : nullptr;
        auto hr = m_pDevice->CreateTexture2D(&descArg->texDesc, param2, m_pTexture.GetAddressOf());
        if (FAILED(hr))
        {
            m_eState = STATE::LOADFAIL;
            MSG_BOX("CResDynamicTexture2D CreateTexture2D");
            return E_FAIL;
        }

        m_pRTV.resize(descArg->texDesc.ArraySize);
        m_pDSV.resize(descArg->texDesc.ArraySize);
        m_pSRV.resize(descArg->texDesc.ArraySize);
        m_pUAV.resize(descArg->texDesc.ArraySize);
    }

    m_eState = STATE::LOADED;
    return S_OK;
}

HRESULT CResDynamicTexture2D::Unload(const std::any& arg)
{
    return S_OK;
}

SPtr<CResDynamicTexture2D> CResDynamicTexture2D::Create()
{
    return ToSPtr(new CResDynamicTexture2D{"", CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext() });
}
