#include "pch.h"
#include "ResQuadColBuffer.h"
#include "GameInstance.h"

NS_USING(Engine)

CResQuadColBuffer::CResQuadColBuffer(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
    : CResQuadBuffer(sPath, pDevice, pContext)
{
}

CResQuadColBuffer::~CResQuadColBuffer()
{
}

HRESULT CResQuadColBuffer::Load(const std::any& arg)
{
    if (m_eState == STATE::LOADED)
    {
        return S_OK;
    }
    m_eState = STATE::LOADING;

    {
        constexpr uint32_t iNumVertices = 4;
        VTX_COL vertices[iNumVertices] =
        {
            {E::_float3(-0.5f, 0.5f, 0.f),  m_colors[0]},
            {E::_float3(0.5f, 0.5f, 0.f),  m_colors[1]},
            {E::_float3(0.5f, -0.5f, 0.f),  m_colors[2]},
            {E::_float3(-0.5f, -0.5f, 0.f),  m_colors[3]}
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
        constexpr uint32_t iNumIndices = 6;
        uint16_t indices[iNumIndices] = { 0, 1, 2, 0, 2, 3 };

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

HRESULT CResQuadColBuffer::Unload(const std::any& arg)
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

SPtr<CResQuadColBuffer> CResQuadColBuffer::Create()
{
    return ToSPtr(new CResQuadColBuffer{ "", CGameInstance::Get().GetGraphicDevice() , CGameInstance::Get().GetGraphicDeviceContext() });
}
