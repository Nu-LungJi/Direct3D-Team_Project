#include "pch.h"
#include "ResCubeColBuffer.h"
#include "GameInstance.h"

NS_USING(Engine)

CResCubeColBuffer::CResCubeColBuffer(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
    : CResCubeBuffer(sPath, pDevice, pContext)
{
}

CResCubeColBuffer::~CResCubeColBuffer()
{
}

HRESULT CResCubeColBuffer::Load(const std::any& arg)
{
    if (m_eState == STATE::LOADED)
    {
        return S_OK;
    }
    m_eState = STATE::LOADING;

    {
        constexpr uint32_t iNumVertices = 8;
        VTX_COL vertices[iNumVertices] = {
            {{-0.5f, -0.5f, -0.5f}, (_float4)Colors::Red},
            {{-0.5f,  0.5f, -0.5f}, (_float4)Colors::Red},
            {{ 0.5f,  0.5f, -0.5f}, (_float4)Colors::Red},
            {{ 0.5f, -0.5f, -0.5f}, (_float4)Colors::Red},
            {{-0.5f, -0.5f,  0.5f}, (_float4)Colors::Yellow},
            {{-0.5f,  0.5f,  0.5f}, (_float4)Colors::LightYellow},
            {{ 0.5f,  0.5f,  0.5f}, (_float4)Colors::AliceBlue},
            {{ 0.5f, -0.5f,  0.5f}, (_float4)Colors::Blue},
        };
        D3D11_SUBRESOURCE_DATA vertexInitialData{};
        vertexInitialData.pSysMem = vertices;

        m_iVertexStride = sizeof(VTX_COL);
        m_iNumVertices = iNumVertices;

        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = m_iVertexStride * m_iNumVertices;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.StructureByteStride = m_iVertexStride;
        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.MiscFlags = 0;

        if (FAILED(CreateVertexBuffer(bufferDesc, &vertexInitialData)))
        {
            m_eState = STATE::LOADFAIL;
            return E_FAIL;
        }

    }

    {
        constexpr uint16_t iNumIndices = 36;
        uint16_t indices[iNumIndices] = {
            // 앞면
            0, 1, 2,
            0, 2, 3,

            // 뒷면
            4, 6, 5,
            4, 7, 6,

            // 왼쪽면
            4, 5, 1,
            4, 1, 0,

            // 오른쪽면
            3, 2, 6,
            3, 6, 7,

            // 윗면
            1, 5, 6,
            1, 6, 2,

            // 밑면
            4, 0, 3,
            4, 3, 7
        };

        D3D11_SUBRESOURCE_DATA indexInitialData{};
        indexInitialData.pSysMem = indices;

        m_iIndexStride = sizeof(uint16_t);
        m_iNumIndices = iNumIndices;
        m_eIndexFormat = DXGI_FORMAT_R16_UINT;

        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = m_iIndexStride * m_iNumIndices;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.StructureByteStride = m_iIndexStride;
        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.MiscFlags = 0;

        if (FAILED(CreateIndexBuffer(bufferDesc, &indexInitialData)))
        {
            m_eState = STATE::LOADFAIL;
            return E_FAIL;
        }
    }

    m_eState = STATE::LOADED;
    return S_OK;
}

HRESULT CResCubeColBuffer::Unload(const std::any& arg)
{
    m_iNumIndices = {};
    m_iNumVertices = {};
    m_iVertexStride = {};
    m_iIndexStride = {};
    m_eIndexFormat = {};

    m_pVB.Reset();
    m_pIB.Reset();
    m_eState = STATE::UNLOAD;
    return S_OK;
}

SPtr<CResCubeColBuffer> CResCubeColBuffer::Create()
{
    return ToSPtr(new CResCubeColBuffer{ "", CGameInstance::Get().GetGraphicDevice() , CGameInstance::Get().GetGraphicDeviceContext() });
}
