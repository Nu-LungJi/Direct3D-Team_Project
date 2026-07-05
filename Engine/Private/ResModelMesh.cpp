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
     
        if (FAILED(Ready_AnimMesh(pModel, ptr)))
            return E_FAIL;

        D3D11_BUFFER_DESC BufferDesc{};
        BufferDesc.ByteWidth = sizeof(_float4x4) * 512;
        BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        BufferDesc.MiscFlags = 0;
        BufferDesc.StructureByteStride = 0;


        if (FAILED(CGameInstance::Get().GetGraphicDevice()->CreateBuffer(&BufferDesc, nullptr, m_pCBBones.GetAddressOf())))
        {
            return E_FAIL;
        }

    }
   

    m_eState = STATE::LOADED;
    return S_OK;
}

HRESULT CResModelMesh::Unload(const std::any& arg)
{

    m_eState = STATE::UNLOAD;
    return S_OK;
}


HRESULT CResModelMesh::Ready_AnimMesh(CResModel* pModel, _char* pPoint)
{

    auto vertexes = std::make_shared<std::vector<VTXANIMMESH>>();
    auto indices = std::make_shared<std::vector<uint32_t>>();

    uint32_t materialIndex = *(uint32_t*)pPoint;
    pPoint += sizeof(uint32_t);


    uint32_t vCount = *(uint32_t*)pPoint;
    pPoint += sizeof(uint32_t);


    uint32_t iCount = *(uint32_t*)pPoint;
    pPoint += sizeof(uint32_t);


    vertexes->resize(vCount);
    memcpy(vertexes->data(), pPoint, sizeof(VTXANIMMESH) * vCount);
    pPoint += sizeof(VTXANIMMESH) * vCount;


    indices->resize(iCount);
    memcpy(indices->data(), pPoint, sizeof(uint32_t) * iCount);
    pPoint += sizeof(uint32_t) * iCount;

     m_iNumBones = *(uint32_t*)pPoint;
    pPoint += sizeof(uint32_t);

    uint32_t BoneIndicesCount = *(uint32_t*)pPoint;
    pPoint += sizeof(uint32_t);

    uint32_t BoneMatricesCount = *(uint32_t*)pPoint;
    pPoint += sizeof(uint32_t);

    uint32_t OffsetMatricesCount = *(uint32_t*)pPoint;
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
    //m_iNumVertexBuffers = 1;
    m_iNumVertices = vCount;

    m_iNumIndices = (UINT)indices->size();
    m_iIndexStride = sizeof(uint32_t);
    m_eIndexFormat = DXGI_FORMAT_R32_UINT;
    m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;


    //-------------------------------------------------------------------
    m_iVertexStride = sizeof(VTXANIMMESH);
    D3D11_BUFFER_DESC           VertexBufferDesc{};
    VertexBufferDesc.ByteWidth = m_iNumVertices * sizeof(VTXANIMMESH);
    VertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    VertexBufferDesc.StructureByteStride = m_iVertexStride;
    VertexBufferDesc.CPUAccessFlags = 0;
    VertexBufferDesc.MiscFlags = 0;



    D3D11_SUBRESOURCE_DATA          VertexInitialData{};
    VertexInitialData.pSysMem = vertexes->data();

    if (FAILED(CreateVertexBuffer(VertexBufferDesc, &VertexInitialData)))
    {
        m_eState = STATE::LOADFAIL;
        return E_FAIL;
    }

    //-----------------------------------------------------------------------------------


    D3D11_BUFFER_DESC           IndexBufferDesc{};
    IndexBufferDesc.ByteWidth = m_iNumIndices * m_iIndexStride;
    IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    IndexBufferDesc.StructureByteStride = m_iIndexStride;
    IndexBufferDesc.CPUAccessFlags = 0;
    IndexBufferDesc.MiscFlags = 0;





    D3D11_SUBRESOURCE_DATA          IndexInitialData{};
    IndexInitialData.pSysMem = indices->data();


    if (FAILED(CreateIndexBuffer(IndexBufferDesc, &IndexInitialData)))
    {
        m_eState = STATE::LOADFAIL;
        return E_FAIL;
    }


    return S_OK;

}

SPtr<CResModelMesh> CResModelMesh::Create()
{
    return ToSPtr(new CResModelMesh{ "",CGameInstance::Get().GetGraphicDevice(),CGameInstance::Get().GetGraphicDeviceContext() });
}
