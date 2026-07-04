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
        HRESULT     hr = MODEL::STATIC == eType ? Ready_NonAnimMesh(ptr, PreTransformMatrix) : Ready_AnimMesh(pModel, ptr);

        if (FAILED(hr))
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

HRESULT CResModelMesh::Ready_NonAnimMesh(_char* pPoint, _fmatrix PreTransformMatrix)
{
    auto vertexes = std::make_shared<std::vector<VTXMESH>>();
    auto indices = std::make_shared<std::vector<uint32_t>>();

    uint32_t nameLen = *(uint32_t*)pPoint;
    pPoint += sizeof(uint32_t);
  


    std::string name;
    name.resize(nameLen);

    memcpy(name.data(), pPoint, nameLen);
    pPoint += nameLen;
 

    uint32_t materialIndex = *(uint32_t*)pPoint;
    pPoint += sizeof(uint32_t);


    uint32_t vCount = *(uint32_t*)pPoint;
    pPoint += sizeof(uint32_t);


    uint32_t iCount = *(uint32_t*)pPoint;
    pPoint += sizeof(uint32_t);


    vertexes->resize(vCount);
    memcpy(vertexes->data(), pPoint, sizeof(VTXMESH) * vCount);
    pPoint += sizeof(VTXMESH) * vCount;


    indices->resize(iCount);
    memcpy(indices->data(), pPoint, sizeof(uint32_t) * iCount);
    pPoint += sizeof(uint32_t) * iCount;



    strcpy_s(m_szName, name.c_str());
    m_iMaterialIndex = materialIndex;
    //m_iNumVertexBuffers = 1;
    m_iNumVertices = vCount;

    m_iNumIndices = (UINT)indices->size();
    m_iIndexStride = sizeof(uint32_t);
    m_eIndexFormat = DXGI_FORMAT_R32_UINT;
    m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;


    for (size_t i = 0; i < m_iNumVertices; i++)
    {

        XMStoreFloat3(&(*vertexes)[i].vPosition, XMVector3TransformCoord(XMLoadFloat3(&(*vertexes)[i].vPosition), PreTransformMatrix));

        XMStoreFloat3(&(*vertexes)[i].vNormal, XMVector3TransformNormal(XMLoadFloat3(&(*vertexes)[i].vNormal), PreTransformMatrix));


        //----------------------- 더 추가 할 예정 ----------------------------------------------------------------------------------
    }

    //-------------------------------------------------------------------
    m_iVertexStride = sizeof(VTXMESH);
    D3D11_BUFFER_DESC           VertexBufferDesc{};
    VertexBufferDesc.ByteWidth = m_iNumVertices * sizeof(VTXMESH);
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

HRESULT CResModelMesh::Ready_AnimMesh(CResModel* pModel, _char* pPoint)
{
    //m_iVertexStride = sizeof(VTXANIMMESH);
    //D3D11_BUFFER_DESC           VertexBufferDesc{};
    //VertexBufferDesc.ByteWidth = m_iNumVertices * m_iVertexStride;
    //VertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    //VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    //VertexBufferDesc.StructureByteStride = m_iVertexStride;
    //VertexBufferDesc.CPUAccessFlags = 0;
    //VertexBufferDesc.MiscFlags = 0;



    //VTXANIMMESH* pVertices = new VTXANIMMESH[m_iNumVertices];
    //ZeroMemory(pVertices, sizeof(VTXANIMMESH) * m_iNumVertices);

    //for (size_t i = 0; i < m_iNumVertices; i++)
    //{
    //    memcpy(&pVertices[i].vPosition, &pAIMesh->mVertices[i], sizeof(_float3));
    //    memcpy(&pVertices[i].vNormal, &pAIMesh->mNormals[i], sizeof(_float3));
    //    memcpy(&pVertices[i].vTangent, &pAIMesh->mTangents[i], sizeof(_float3));
    //    memcpy(&pVertices[i].vBinormal, &pAIMesh->mBitangents[i], sizeof(_float3));
    //    memcpy(&pVertices[i].vTexcoord, &pAIMesh->mTextureCoords[0][i], sizeof(_float2));
    //}

    //m_iNumBones = pAIMesh->mNumBones;

    //m_BoneMatrices.resize(m_iNumBones);
    //m_OffsetMatrices.reserve(m_iNumBones);

    //for (size_t i = 0; i < m_iNumBones; i++)
    //{
    //    aiBone* pAIBone = pAIMesh->mBones[i];

    //    _float4x4   OffsetMatrix;
    //    memcpy(&OffsetMatrix, &pAIBone->mOffsetMatrix, sizeof(_float4x4));

    //    XMStoreFloat4x4(&OffsetMatrix, XMMatrixTranspose(XMLoadFloat4x4(&OffsetMatrix)));

    //    m_OffsetMatrices.push_back(OffsetMatrix);

    //    int32_t    iBoneIndex = pModel->Get_BoneIndex(pAIBone->mName.C_Str());
    //    if (-1 == iBoneIndex)
    //        return E_FAIL;

    //    m_BoneIndices.push_back(iBoneIndex);

    //    /* pAIBone->mNumWeights : 이 뼈가 영향을 주는 정점의 갯수 */
    //    for (size_t j = 0; j < pAIBone->mNumWeights; j++)
    //    {
    //        if (0 == pVertices[pAIBone->mWeights[j].mVertexId].vBlendWeights.x)
    //        {
    //            pVertices[pAIBone->mWeights[j].mVertexId].vBlendIndices.x = i;
    //            pVertices[pAIBone->mWeights[j].mVertexId].vBlendWeights.x = pAIBone->mWeights[j].mWeight;
    //        }

    //        else if (0 == pVertices[pAIBone->mWeights[j].mVertexId].vBlendWeights.y)
    //        {
    //            pVertices[pAIBone->mWeights[j].mVertexId].vBlendIndices.y = i;
    //            pVertices[pAIBone->mWeights[j].mVertexId].vBlendWeights.y = pAIBone->mWeights[j].mWeight;
    //        }

    //        else if (0 == pVertices[pAIBone->mWeights[j].mVertexId].vBlendWeights.z)
    //        {
    //            pVertices[pAIBone->mWeights[j].mVertexId].vBlendIndices.z = i;
    //            pVertices[pAIBone->mWeights[j].mVertexId].vBlendWeights.z = pAIBone->mWeights[j].mWeight;
    //        }

    //        else if (0 == pVertices[pAIBone->mWeights[j].mVertexId].vBlendWeights.w)
    //        {
    //            pVertices[pAIBone->mWeights[j].mVertexId].vBlendIndices.w = i;
    //            pVertices[pAIBone->mWeights[j].mVertexId].vBlendWeights.w = pAIBone->mWeights[j].mWeight;
    //        }
    //    }
    //}

    //if (0 == m_iNumBones)
    //{
    //    m_iNumBones = 1;

    //    int32_t        iBoneIndex = { -1 };

    //    iBoneIndex = pModel->Get_BoneIndex(m_szName);

    //    if (-1 == iBoneIndex)
    //        return E_FAIL;

    //    _float4x4       OffsetMatrix;
    //    XMStoreFloat4x4(&OffsetMatrix, XMMatrixIdentity());

    //    m_BoneIndices.push_back(iBoneIndex);
    //    m_OffsetMatrices.push_back(OffsetMatrix);
    //    m_BoneMatrices.resize(iBoneIndex);
    //}

    //D3D11_SUBRESOURCE_DATA          VertexInitialData{};
    //VertexInitialData.pSysMem = pVertices;


    //if (FAILED(CreateVertexBuffer(VertexBufferDesc, &VertexInitialData)))
    //{
    //    m_eState = STATE::LOADFAIL;
    //    return E_FAIL;
    //}

    //Safe_Delete_Array(pVertices);

    return S_OK;
}

SPtr<CResModelMesh> CResModelMesh::Create()
{
    return ToSPtr(new CResModelMesh{ "",CGameInstance::Get().GetGraphicDevice(),CGameInstance::Get().GetGraphicDeviceContext() });
}
