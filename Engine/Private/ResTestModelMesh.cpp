#include "pch.h"
#include "ResTestModelMesh.h"
#include "ResTestModelBone.h"
#ifdef _DEBUG
#ifdef new
#pragma push_macro("new")
#undef new
#define RESTORE_NEW_MACRO
#endif
#endif

#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#ifdef _DEBUG
#ifdef RESTORE_NEW_MACRO
#pragma pop_macro("new")
#undef RESTORE_NEW_MACRO
#endif
#endif

#include <fstream>

NS_USING(Engine)

CResTestModelMesh::CResTestModelMesh(const _string& sPath, ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: CResVIBuffer{ sPath, pDevice,pContext }
{
}

CResTestModelMesh::~CResTestModelMesh()
{
}

HRESULT CResTestModelMesh::Load(const std::any& arg)
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
    auto& pAIMesh = descArg->pAIMesh;
    auto& PreTransformMatrix = descArg->PreTransformMatrix;
    auto& pModel = descArg->pModel;
    auto& eType = descArg->eType;

    {
        strcpy_s(m_szName, pAIMesh->mName.C_Str());
        m_iMaterialIndex = pAIMesh->mMaterialIndex;
        //m_iNumVertexBuffers = 1;
        m_iNumVertices = 1;
        m_iNumVertices = pAIMesh->mNumVertices;

        m_iNumIndices = pAIMesh->mNumFaces * 3;
        m_iIndexStride = sizeof(uint32_t);
        m_eIndexFormat = DXGI_FORMAT_R32_UINT;
        m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

        #pragma region VERTEX_BUFFER
                HRESULT     hr = MODEL::NONANIM == eType ?
                    Ready_NonAnimMesh(pAIMesh, PreTransformMatrix) : Ready_AnimMesh(pModel, pAIMesh);

        if (FAILED(hr))
            return E_FAIL;


        #pragma endregion

    }

    



    {
#pragma region INDEX_BUFFER
        D3D11_BUFFER_DESC           IndexBufferDesc{};
        IndexBufferDesc.ByteWidth = m_iNumIndices * m_iIndexStride;
        IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        IndexBufferDesc.StructureByteStride = m_iIndexStride;
        IndexBufferDesc.CPUAccessFlags = 0;
        IndexBufferDesc.MiscFlags = 0;

        uint32_t* pIndices = new uint32_t[m_iNumIndices];
        ZeroMemory(pIndices, sizeof(uint32_t)* m_iNumIndices);

        uint32_t        iNumIndices = {};

        for (size_t i = 0; i < pAIMesh->mNumFaces; i++)
        {
            pIndices[iNumIndices++] = pAIMesh->mFaces[i].mIndices[0];
            pIndices[iNumIndices++] = pAIMesh->mFaces[i].mIndices[1];
            pIndices[iNumIndices++] = pAIMesh->mFaces[i].mIndices[2];
        }

        D3D11_SUBRESOURCE_DATA          IndexInitialData{};
        IndexInitialData.pSysMem = pIndices;


        if (FAILED(CreateIndexBuffer(IndexBufferDesc, &IndexInitialData)))
        {
            m_eState = STATE::LOADFAIL;
            return E_FAIL;
        }
        Safe_Delete_Array(pIndices);
#pragma endregion
    }

    D3D11_BUFFER_DESC BufferDesc{};
    BufferDesc.ByteWidth = sizeof(_float4x4) * 512;
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    BufferDesc.MiscFlags = 0;
    BufferDesc.StructureByteStride = 0;

    if (FAILED(CGameInstance::Get().GetGraphicDevice()->CreateBuffer(
        &BufferDesc,
        nullptr,
        m_pCBBones.GetAddressOf())))
    {
        return E_FAIL;
    }

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResTestModelMesh::Unload(const std::any& arg)
{

	m_eState = STATE::UNLOAD;
	return S_OK;
}

HRESULT CResTestModelMesh::Bind_BoneMatrices(const std::vector<SPtr<CResTestModelBone>>& Bones)
{
    for (size_t i = 0; i < m_iNumBones; i++)
    {
        XMStoreFloat4x4(&m_BoneMatrices[i],
            XMLoadFloat4x4(&m_OffsetMatrices[i]) * Bones[m_BoneIndices[i]]->Get_CombinedTransformationMatrix());
    }
    if (!m_BoneMatrices.empty())
    {
        auto pContext = CGameInstance::Get().GetGraphicDeviceContext();

        D3D11_MAPPED_SUBRESOURCE MappedResource{};

        if (FAILED(pContext->Map(m_pCBBones.Get(),0, D3D11_MAP_WRITE_DISCARD,0,&MappedResource)))
        {
            return E_FAIL;
        }

        _float4x4* pBoneMatrices = reinterpret_cast<_float4x4*>(MappedResource.pData);

        for (uint32_t i = 0; i < 512; ++i)
        {
            XMStoreFloat4x4( &pBoneMatrices[i],XMMatrixIdentity());
        }

        const uint32_t iBoneCount = static_cast<uint32_t>(std::min<size_t>(m_BoneMatrices.size(), 512));

        memcpy(pBoneMatrices,m_BoneMatrices.data(),sizeof(_float4x4) * iBoneCount );

        pContext->Unmap(m_pCBBones.Get(), 0);

        ID3D11Buffer* pCBBones = m_pCBBones.Get();

        pContext->VSSetConstantBuffers(2,1,&pCBBones);
    }

 

    return S_OK;

}

HRESULT CResTestModelMesh::Ready_NonAnimMesh(const aiMesh* pAIMesh, _fmatrix PreTransformMatrix)
{
    m_iVertexStride = sizeof(VTXMESH);
    D3D11_BUFFER_DESC           VertexBufferDesc{};
    VertexBufferDesc.ByteWidth = m_iNumVertices * m_iVertexStride;
    VertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    VertexBufferDesc.StructureByteStride = m_iVertexStride;
    VertexBufferDesc.CPUAccessFlags = 0;
    VertexBufferDesc.MiscFlags = 0;

    VTXMESH* pVertices = new VTXMESH[m_iNumVertices];
    ZeroMemory(pVertices, sizeof(VTXMESH) * m_iNumVertices);

    for (size_t i = 0; i < m_iNumVertices; i++)
    {
        memcpy(&pVertices[i].vPosition, &pAIMesh->mVertices[i], sizeof(_float3));
        XMStoreFloat3(&pVertices[i].vPosition, XMVector3TransformCoord(XMLoadFloat3(&pVertices[i].vPosition), PreTransformMatrix));

        memcpy(&pVertices[i].vNormal, &pAIMesh->mNormals[i], sizeof(_float3));
        XMStoreFloat3(&pVertices[i].vNormal, XMVector3TransformNormal(XMLoadFloat3(&pVertices[i].vNormal), PreTransformMatrix));

        memcpy(&pVertices[i].vTangent, &pAIMesh->mTangents[i], sizeof(_float3));
        XMStoreFloat3(&pVertices[i].vTangent, XMVector3TransformNormal(XMLoadFloat3(&pVertices[i].vTangent), PreTransformMatrix));

        memcpy(&pVertices[i].vBinormal, &pAIMesh->mBitangents[i], sizeof(_float3));
        XMStoreFloat3(&pVertices[i].vBinormal, XMVector3TransformNormal(XMLoadFloat3(&pVertices[i].vBinormal), PreTransformMatrix));

        memcpy(&pVertices[i].vTexcoord, &pAIMesh->mTextureCoords[0][i], sizeof(_float2));
    }


    D3D11_SUBRESOURCE_DATA          VertexInitialData{};
    VertexInitialData.pSysMem = pVertices;

    if (FAILED(CreateVertexBuffer(VertexBufferDesc, &VertexInitialData)))
    {
        m_eState = STATE::LOADFAIL;
        return E_FAIL;
    }


    Safe_Delete_Array(pVertices);

    return S_OK;
}

HRESULT CResTestModelMesh::Ready_AnimMesh(CResTestModel* pModel, const aiMesh* pAIMesh)
{
    m_iVertexStride = sizeof(VTXANIMMESH);
    D3D11_BUFFER_DESC           VertexBufferDesc{};
    VertexBufferDesc.ByteWidth = m_iNumVertices * m_iVertexStride;
    VertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    VertexBufferDesc.StructureByteStride = m_iVertexStride;
    VertexBufferDesc.CPUAccessFlags = 0;
    VertexBufferDesc.MiscFlags = 0;



    VTXANIMMESH* pVertices = new VTXANIMMESH[m_iNumVertices];
    ZeroMemory(pVertices, sizeof(VTXANIMMESH) * m_iNumVertices);

    for (size_t i = 0; i < m_iNumVertices; i++)
    {
        memcpy(&pVertices[i].vPosition, &pAIMesh->mVertices[i], sizeof(_float3));
        memcpy(&pVertices[i].vNormal, &pAIMesh->mNormals[i], sizeof(_float3));
        memcpy(&pVertices[i].vTangent, &pAIMesh->mTangents[i], sizeof(_float3));
        memcpy(&pVertices[i].vBinormal, &pAIMesh->mBitangents[i], sizeof(_float3));
        memcpy(&pVertices[i].vTexcoord, &pAIMesh->mTextureCoords[0][i], sizeof(_float2));
    }

    m_iNumBones = pAIMesh->mNumBones;

    m_BoneMatrices.resize(m_iNumBones);
    m_OffsetMatrices.reserve(m_iNumBones);

    for (size_t i = 0; i < m_iNumBones; i++)
    {
        aiBone* pAIBone = pAIMesh->mBones[i];

        _float4x4   OffsetMatrix;
        memcpy(&OffsetMatrix, &pAIBone->mOffsetMatrix, sizeof(_float4x4));

        XMStoreFloat4x4(&OffsetMatrix, XMMatrixTranspose(XMLoadFloat4x4(&OffsetMatrix)));

        m_OffsetMatrices.push_back(OffsetMatrix);

        int32_t    iBoneIndex = pModel->Get_BoneIndex(pAIBone->mName.C_Str());
        if (-1 == iBoneIndex)
            return E_FAIL;

        m_BoneIndices.push_back(iBoneIndex);

        /* pAIBone->mNumWeights : 이 뼈가 영향을 주는 정점의 갯수 */
        for (size_t j = 0; j < pAIBone->mNumWeights; j++)
        {
            if (0 == pVertices[pAIBone->mWeights[j].mVertexId].vBlendWeights.x)
            {
                pVertices[pAIBone->mWeights[j].mVertexId].vBlendIndices.x = i;
                pVertices[pAIBone->mWeights[j].mVertexId].vBlendWeights.x = pAIBone->mWeights[j].mWeight;
            }

            else if (0 == pVertices[pAIBone->mWeights[j].mVertexId].vBlendWeights.y)
            {
                pVertices[pAIBone->mWeights[j].mVertexId].vBlendIndices.y = i;
                pVertices[pAIBone->mWeights[j].mVertexId].vBlendWeights.y = pAIBone->mWeights[j].mWeight;
            }

            else if (0 == pVertices[pAIBone->mWeights[j].mVertexId].vBlendWeights.z)
            {
                pVertices[pAIBone->mWeights[j].mVertexId].vBlendIndices.z = i;
                pVertices[pAIBone->mWeights[j].mVertexId].vBlendWeights.z = pAIBone->mWeights[j].mWeight;
            }

            else if (0 == pVertices[pAIBone->mWeights[j].mVertexId].vBlendWeights.w)
            {
                pVertices[pAIBone->mWeights[j].mVertexId].vBlendIndices.w = i;
                pVertices[pAIBone->mWeights[j].mVertexId].vBlendWeights.w = pAIBone->mWeights[j].mWeight;
            }
        }
    }

    if (0 == m_iNumBones)
    {
        m_iNumBones = 1;

        int32_t        iBoneIndex = { -1 };

        iBoneIndex = pModel->Get_BoneIndex(m_szName);

        if (-1 == iBoneIndex)
            return E_FAIL;

        _float4x4       OffsetMatrix;
        XMStoreFloat4x4(&OffsetMatrix, XMMatrixIdentity());

        m_BoneIndices.push_back(iBoneIndex);
        m_OffsetMatrices.push_back(OffsetMatrix);
        m_BoneMatrices.resize(iBoneIndex);
    }

    D3D11_SUBRESOURCE_DATA          VertexInitialData{};
    VertexInitialData.pSysMem = pVertices;


    if (FAILED(CreateVertexBuffer(VertexBufferDesc, &VertexInitialData)))
    {
        m_eState = STATE::LOADFAIL;
        return E_FAIL;
    }

    Safe_Delete_Array(pVertices);

    return S_OK;
}

SPtr<CResTestModelMesh> CResTestModelMesh::Create()
{
	return ToSPtr(new CResTestModelMesh{ "",CGameInstance::Get().GetGraphicDevice(),CGameInstance::Get().GetGraphicDeviceContext()});
}
