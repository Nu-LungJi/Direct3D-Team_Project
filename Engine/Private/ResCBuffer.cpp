#include "pch.h"
#include "ResCBuffer.h"
#include "GameInstance.h"

NS_USING(Engine)

CResCBuffer::CResCBuffer(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
    : CResource{ sPath }
    , m_pDevice{ pDevice }
    , m_pContext{ pContext }
{
}

CResCBuffer::~CResCBuffer()
{
}

HRESULT CResCBuffer::Load(const std::any& arg)
{
    auto desc = std::any_cast<CBUFFER_DESC>(&arg);
    if (!desc)
    {
        return E_FAIL;

    }
    if (m_eState == STATE::LOADED)
    {
        return S_OK;
    }
    m_eState = STATE::LOADING;

    if (FAILED(CreateBuffer(*desc)))
    {
        m_eState = STATE::LOADFAIL;
        return E_FAIL;
    }

    m_eState = STATE::LOADED;
    return S_OK;
}

HRESULT CResCBuffer::Unload(const std::any& arg)
{
    m_eState = STATE::UNLOAD;

    m_pCBuffer.Reset();

    m_byteWidth = {};
    m_usage = {};
    m_bindFlags = {};
    m_cpuAccessFlags = {};
    return S_OK;
}

HRESULT CResCBuffer::CreateBuffer(const CBUFFER_DESC& desc)
{
    m_byteWidth = desc.byteWidth;
    m_usage = desc.usage;
    m_bindFlags = desc.bindFlags;
    m_cpuAccessFlags = desc.cpuAccessFlags;

    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = m_byteWidth;
    bd.Usage = m_usage;
    bd.BindFlags = m_bindFlags;
    bd.CPUAccessFlags = m_cpuAccessFlags;
    bd.MiscFlags = 0;
    bd.StructureByteStride = 0; // constatnt buffer는 필요없음

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = desc.pInitData;

    return m_pDevice->CreateBuffer(&bd, desc.pInitData ? &initData : nullptr, m_pCBuffer.GetAddressOf());
}

SPtr<CResCBuffer> CResCBuffer::Create()
{
    
    auto pInstance = ToSPtr(new CResCBuffer{ "", CGameInstance::Get().GetGraphicDevice() , CGameInstance::Get().GetGraphicDeviceContext() });
    
    return pInstance;
}
