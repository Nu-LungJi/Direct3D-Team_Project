#include "pch.h"
#include "ResModelMesh.h"

#include <fstream>

NS_USING(Engine)

CResModelMesh::CResModelMesh(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
    : CResVIBuffer{ sPath, pDevice,pContext }
{
}

CResModelMesh::~CResModelMesh()
{
}

HRESULT CResModelMesh::Load(const std::any& arg)
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
    auto ptr = descArg->ptr;
    auto eType = descArg->eType;
    auto& pModel = descArg->pModel;
    auto& PreTransformMatrix = descArg->PreTransformMatrix;

    {
        if (FAILED(Ready_AnimMesh(pModel, ptr, descArg->iRecordSize)))
            return E_FAIL;
    }
   

    m_eState = STATE::LOADED;
    return S_OK;
}

HRESULT CResModelMesh::Unload(const std::any& arg)
{

    m_eState = STATE::UNLOAD;
    return S_OK;
}

HRESULT CResModelMesh::Ready_AnimMesh(CResModel* pModel, _char* pPoint, uint32_t iRecordSize)
{
	_char* const pRecordBegin = pPoint;
	_char* const pRecordEnd = pRecordBegin + iRecordSize;
    uint32_t materialIndex = 0;
    memcpy(&materialIndex, pPoint, sizeof(uint32_t));
    pPoint += sizeof(uint32_t);

    uint32_t vCount = 0;
    memcpy(&vCount, pPoint, sizeof(uint32_t));
    pPoint += sizeof(uint32_t);

    uint32_t iCount = 0;
    memcpy(&iCount, pPoint, sizeof(uint32_t));
    pPoint += sizeof(uint32_t);

    const VTXANIMMESH* pVertexData =
        reinterpret_cast<const VTXANIMMESH*>(pPoint);
    pPoint += sizeof(VTXANIMMESH) * vCount;

    const uint32_t* pIndexData =
        reinterpret_cast<const uint32_t*>(pPoint);
    pPoint += sizeof(uint32_t) * iCount;

    memcpy(&m_iNumBones, pPoint, sizeof(uint32_t));
    pPoint += sizeof(uint32_t);

    uint32_t BoneIndicesCount = 0;
    memcpy(&BoneIndicesCount, pPoint, sizeof(uint32_t));
    pPoint += sizeof(uint32_t);

    uint32_t BoneMatricesCount = 0;
    memcpy(&BoneMatricesCount, pPoint, sizeof(uint32_t));
    pPoint += sizeof(uint32_t);

    uint32_t OffsetMatricesCount = 0;
    memcpy(&OffsetMatricesCount, pPoint, sizeof(uint32_t));
    pPoint += sizeof(uint32_t);

    m_BoneIndices.resize(BoneIndicesCount);
    memcpy(m_BoneIndices.data(), pPoint, sizeof(uint32_t) * BoneIndicesCount);
    pPoint += sizeof(uint32_t) * BoneIndicesCount;

    m_BoneMatrices.resize(BoneMatricesCount);
    memcpy(m_BoneMatrices.data(), pPoint, sizeof(_float4x4) * BoneMatricesCount);
    pPoint += sizeof(_float4x4) * BoneMatricesCount;

    m_OffsetMatrices.resize(OffsetMatricesCount);
    memcpy(m_OffsetMatrices.data(), pPoint, sizeof(_float4x4) * OffsetMatricesCount);
    pPoint += sizeof(_float4x4) * OffsetMatricesCount;

    m_iMaterialIndex = materialIndex;
    m_iNumVertices = vCount;
    m_iNumIndices = iCount;
    m_iIndexStride = sizeof(uint32_t);
    m_eIndexFormat = DXGI_FORMAT_R32_UINT;
    m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    m_iVertexStride = sizeof(VTXANIMMESH);

    D3D11_BUFFER_DESC VertexBufferDesc{};
    VertexBufferDesc.ByteWidth = m_iNumVertices * sizeof(VTXANIMMESH);
    VertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    VertexBufferDesc.StructureByteStride = m_iVertexStride;

    D3D11_SUBRESOURCE_DATA VertexInitialData{};
    VertexInitialData.pSysMem = pVertexData;

    if (FAILED(CreateVertexBuffer(VertexBufferDesc, &VertexInitialData)))
    {
        m_eState = STATE::LOADFAIL;
        return E_FAIL;
    }
    {
        CResStructuredBuffer::DESC skinInputDesc{};
        skinInputDesc.iNumElements = m_iNumVertices;
        skinInputDesc.iStructureByteStride = sizeof(VTXANIMMESH);
        skinInputDesc.iBindFlags = D3D11_BIND_SHADER_RESOURCE;
        skinInputDesc.pInitialData = pVertexData;
        m_pSkinningInputBuffer = CResStructuredBuffer::Create();
        if (!m_pSkinningInputBuffer || FAILED(m_pSkinningInputBuffer->Load(skinInputDesc)))
            return E_FAIL;


    }

    D3D11_BUFFER_DESC IndexBufferDesc{};
    IndexBufferDesc.ByteWidth = m_iNumIndices * m_iIndexStride;
    IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    IndexBufferDesc.StructureByteStride = m_iIndexStride;

    D3D11_SUBRESOURCE_DATA IndexInitialData{};
    IndexInitialData.pSysMem = pIndexData;

    if (FAILED(CreateIndexBuffer(IndexBufferDesc, &IndexInitialData)))
    {
        m_eState = STATE::LOADFAIL;
        return E_FAIL;
    }

    return S_OK;
}
SPtr<CResStructuredBuffer> CResModelMesh::GetSkinningInputBuffer() const
{
    return m_pSkinningInputBuffer;
}

HRESULT CResModelMesh::EnsureSkinnedVertexBuffer(uint32_t iInstanceCapacity)
{
    if (iInstanceCapacity == 0 || m_iNumVertices == 0)
        return E_INVALIDARG;

    if (m_pSkinnedVertexBuffer && m_iSkinnedVertexInstanceCapacity >= iInstanceCapacity)
        return S_OK;

    CResStructuredBuffer::DESC desc{};
    desc.iNumElements = m_iNumVertices * iInstanceCapacity;
    desc.iStructureByteStride = sizeof(_float4) * 4;
    desc.iBindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

    auto pNewBuffer = CResStructuredBuffer::Create();
    if (!pNewBuffer || FAILED(pNewBuffer->Load(desc)))
        return E_FAIL;

    m_pSkinnedVertexBuffer = std::move(pNewBuffer);
    m_iSkinnedVertexInstanceCapacity = iInstanceCapacity;
    return S_OK;
}
SPtr<CResStructuredBuffer> CResModelMesh::GetSkinnedVertexBuffer() const
{
    return m_pSkinnedVertexBuffer;
}
SPtr<CResModelMesh> CResModelMesh::Create()
{
    return ToSPtr(new CResModelMesh{ "",CGameInstance::Get().GetGraphicDevice(),CGameInstance::Get().GetGraphicDeviceContext() });
}
