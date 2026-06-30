#include "pch.h"
#include "ResTextureCubeMap.h"
#include "GameInstance.h"

NS_USING(Engine)

CResTextureCubeMap::CResTextureCubeMap(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
    : CResTexture{ sPath, pDevice, pContext } 
{
}

CResTextureCubeMap::~CResTextureCubeMap()
{
}

HRESULT CResTextureCubeMap::Load(const std::any& arg)
{
    if (m_eState == STATE::LOADED)
    {
        return S_OK;
    }

    m_eState = STATE::LOADING;
    

    {
        std::wstring path = StringToWString(m_sPath);
        std::wstring ext = std::filesystem::path(path).extension().wstring();

        // 1. DDS만 허용
        if (ext != L".dds")
        {
            MSG_BOX("CubeMap must be DDS");
            m_eState = STATE::LOADFAIL;
            return E_FAIL;
        }

        ComPtr<ID3D11Resource> pResource;

        // 2. DDS 로드 (큐브맵 포함)
        HRESULT hr = DirectX::CreateDDSTextureFromFile(
            m_pDevice.Get(),
            path.c_str(),
            pResource.GetAddressOf(),
            m_pSRV.GetAddressOf()
        );

        if (FAILED(hr))
        {
            MSG_BOX("CreateDDSTextureFromFile Failed");
            m_eState = STATE::LOADFAIL;
            return E_FAIL;
        }

        // 3. Texture2D 얻기
        hr = pResource.As(&m_pTexture);
        if (FAILED(hr))
        {
            MSG_BOX("QueryInterface Texture2D Failed");
            m_eState = STATE::LOADFAIL;
            return E_FAIL;
        }

        // 4. CubeMap 검증
        m_pTexture->GetDesc(&m_Texture2DDesc);

        if (!(m_Texture2DDesc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE))
        {
            MSG_BOX("This DDS is NOT a CubeMap");
            m_eState = STATE::LOADFAIL;
            return E_FAIL;
        }

        // 5. SRV 타입 검증 (디버그용)
#ifdef _DEBUG
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        m_pSRV->GetDesc(&srvDesc);

        if (srvDesc.ViewDimension != D3D11_SRV_DIMENSION_TEXTURECUBE)
        {
            MSG_BOX("SRV is not TextureCube");
            m_eState = STATE::LOADFAIL;
            return E_FAIL;
        }
#endif
    }


    m_eState = STATE::LOADED;

    return S_OK;
}

HRESULT CResTextureCubeMap::Unload(const std::any& arg)
{
    return S_OK;
}

SPtr<CResTextureCubeMap> CResTextureCubeMap::Create(const _string& sPath)
{
    return ToSPtr(new CResTextureCubeMap{ sPath, CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext() });
}
