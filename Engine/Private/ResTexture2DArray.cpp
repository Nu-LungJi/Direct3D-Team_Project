#include "pch.h"
#include "ResTexture2DArray.h"
#include "GameInstance.h"
#include "ResTexture2D.h"
NS_USING(Engine)

CResTexture2DArray::CResTexture2DArray(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
    : CResTexture{ sPath, pDevice, pContext } 
{
}

CResTexture2DArray::~CResTexture2DArray()
{
}

HRESULT CResTexture2DArray::Load(const std::any& arg)
{
    auto argDesc = std::any_cast<DESC>(&arg);
    if (!argDesc)
    {
        MSG_BOX_STR(_wstring{ L"CResTexture2DArray NoDesc Faield Path:" + StringToWString(m_sPath) }.c_str());
        return E_FAIL;
    }

    if (m_eState == STATE::LOADED)
    {
        return S_OK;
    }

    m_eState = STATE::LOADING;

    auto resources = CGameInstance::Get().GetResource(argDesc->textureId.first, argDesc->textureId.second);
    if (resources.empty())
    {
        m_eState = STATE::LOADFAIL;
        MSG_BOX_STR(_wstring{ L"CResTexture2DArray No resources Faield Path:" + StringToWString(m_sPath) }.c_str());
        return E_FAIL;
    }

    std::vector<SPtr<CResTexture2D>> textures;
    for (const auto& resource : resources)
    {
        if (!resource->IsA(CResTexture2D::StaticType))
        {
            m_eState = STATE::LOADFAIL;
            MSG_BOX_STR(_wstring{ L"CResTexture2DArray CResTexture2D IsA Faield Path:" + StringToWString(m_sPath) }.c_str());
            return E_FAIL;
        }
        textures.push_back(std::static_pointer_cast<CResTexture2D>(resource));
    }

    // 기준 텍스처 desc
    D3D11_TEXTURE2D_DESC desc{};
    textures[0]->GetTexture()->GetDesc(&desc);

    // Array용으로 수정
    desc.ArraySize = (UINT)textures.size();
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags = 0;

    // 텍스처 조건 검증 (필수)
    for (size_t i = 1; i < textures.size(); ++i)
    {
        //DXGI_FORMAT_B8G8R8A8_UNORM
        //    DXGI_FORMAT_BC1_UNORM
        D3D11_TEXTURE2D_DESC d{};
        textures[i]->GetTexture()->GetDesc(&d);

        if (d.Width != desc.Width ||
            d.Height != desc.Height ||
            d.Format != desc.Format ||
            d.MipLevels != desc.MipLevels)
        {
            MSG_BOX_STR(_wstring{ L"CResTexture2DArray CResTexture2D Compare Formats Faield Path:" + StringToWString(m_sPath) }.c_str());
            m_eState = STATE::LOADFAIL;
            return E_FAIL;
        }
    }

    // Array 텍스처 생성
    if (FAILED(m_pDevice->CreateTexture2D(&desc, nullptr, m_pTextureArray.GetAddressOf())))
    {
        MSG_BOX_STR(_wstring{ L"CResTexture2DArray Create Faield Path:" + StringToWString(m_sPath) }.c_str());
        m_eState = STATE::LOADFAIL;
        return E_FAIL;
    }

    // 데이터 복사
    for (UINT i = 0; i < desc.ArraySize; ++i)
    {
        for (UINT mip = 0; mip < desc.MipLevels; ++mip)
        {
            m_pContext->CopySubresourceRegion(
                m_pTextureArray.Get(),
                D3D11CalcSubresource(mip, i, desc.MipLevels),
                0, 0, 0,
                textures[i]->GetTexture().Get(),
                mip,
                nullptr
            );
        }
    }

    // SRV 생성
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
    //srvDesc.Texture2DArray.MipLevels = -1;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = desc.ArraySize;

    if (FAILED(m_pDevice->CreateShaderResourceView(
        m_pTextureArray.Get(),
        &srvDesc,
        m_pSRV.GetAddressOf()
    )))
    {
        MSG_BOX_STR(_wstring{ L"CResTexture2DArray Create SRV Faield Path:" + StringToWString(m_sPath) }.c_str());
        m_eState = STATE::LOADFAIL;
        return E_FAIL;
    }

    m_textureId = argDesc->textureId;
    m_eState = STATE::LOADED;

    return S_OK;
}

HRESULT CResTexture2DArray::Unload(const std::any& arg)
{
    return S_OK;
}

SPtr<CResTexture2DArray> CResTexture2DArray::Create()
{
    return ToSPtr(new CResTexture2DArray{"", CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext() });
}
