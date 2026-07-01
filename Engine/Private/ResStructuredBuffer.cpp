#include "pch.h"
#include "ResStructuredBuffer.h"
#include "GameInstance.h"

NS_USING(Engine)

CResStructuredBuffer::CResStructuredBuffer(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
    : CResource{ "" }
    , m_pDevice{ pDevice }
    , m_pContext{ pContext }
{
}

CResStructuredBuffer::~CResStructuredBuffer()
{
    Unload();
}

HRESULT CResStructuredBuffer::Load(const std::any& arg)
{

    auto argDesc = std::any_cast<DESC>(&arg);
    if (!argDesc)
    {
        return E_FAIL;
    }

    if (m_eState == STATE::LOADED)
    {
        return S_OK;
    }
    m_eState = STATE::LOADING;

    //ID3D11Buffer 생성을 위한 구조화 버퍼 고유 Desc 설정
    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.ByteWidth = argDesc->iNumElements * argDesc->iStructureByteStride;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT; // GPU 내부 전용 (계산 셰이더 쓰기/읽기)
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED; 
    bufferDesc.StructureByteStride = argDesc->iStructureByteStride;    

    // 초기 데이터 세팅 처리 (DynamicBuffer와 동일한 방식 적용)
    D3D11_SUBRESOURCE_DATA subData{};
    D3D11_SUBRESOURCE_DATA* pSubData = nullptr;
    if (argDesc->pInitialData != nullptr)
    {
        subData.pSysMem = argDesc->pInitialData;
        pSubData = &subData;
    }

    // 버퍼 생성
    if (FAILED(m_pDevice->CreateBuffer(&bufferDesc, pSubData, m_pBuffer.GetAddressOf())))
    {
        return E_FAIL;
    }

    //  셰이더 리소스 뷰 생성 (정점 셰이더(VS) 읽기용 SRV)
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = argDesc->iNumElements;

    if (FAILED(m_pDevice->CreateShaderResourceView(m_pBuffer.Get(), &srvDesc, m_pSRV.GetAddressOf())))
    {
        return E_FAIL;
    }

    //  순서 없는 액세스 뷰 생성 (계산 셰이더(CS) 쓰기용 UAV)
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN; 
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = argDesc->iNumElements;
    uavDesc.Buffer.Flags = 0;

    if (FAILED(m_pDevice->CreateUnorderedAccessView(m_pBuffer.Get(), &uavDesc, m_pUAV.GetAddressOf())))
    {
        return E_FAIL;
    }

    m_eState = STATE::LOADED;
    return S_OK;
}

HRESULT CResStructuredBuffer::Unload(const std::any& arg)
{
    m_pBuffer.Reset();
    m_pSRV.Reset();
    m_pUAV.Reset();
    m_eState = STATE::UNLOAD;
    return S_OK;
}

SPtr<CResStructuredBuffer> CResStructuredBuffer::Create()
{
    // 기존 팩토리와 완전히 동일하게 그래픽 디바이스와 컨텍스트 주입
    return ToSPtr(new CResStructuredBuffer{ CGameInstance::Get().GetGraphicDevice(), CGameInstance::Get().GetGraphicDeviceContext() });
}