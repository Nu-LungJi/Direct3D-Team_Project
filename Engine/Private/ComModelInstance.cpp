#include "pch.h"
#include "GameInstance.h"
#include "ComModelInstance.h"
#include "ResModelBone.h"
#include "ResModelMesh.h"
#include "ResModelMaterial.h"
#include "ResModel.h"
NS_USING(Engine)



CComModelInstance::CComModelInstance()
{
  

}

CComModelInstance::~CComModelInstance()
{
}


HRESULT CComModelInstance::Initialize(void* pArg)
{
    if (FAILED(CComponent::Initialize(pArg)))
    {
        return E_FAIL;
    }

    if (pArg != nullptr) {
        // 모델 Instance는 하나의 메모리를 모두 공유한다.
        CComModelInstance::DESC* pDesc = reinterpret_cast<CComModelInstance::DESC*>(pArg);
        m_pModel = CGameInstance::Get().GetResourceFirst<CResModel>(pDesc->sGroupTag, pDesc->sResTag);
        if (m_pModel == nullptr)
        {
			return E_FAIL;
        }
    }
	
    return S_OK;
}

HRESULT CComModelInstance::Bind_BoneMatrices(ID3D11DeviceContext* pContext, uint32_t iMeshIndex)
{
    // 나중에 Bind 할떄 animation 정보를 던져준 GPU에서 Animatino 돌린다. 나중에 수정
    auto& pMesh = m_pModel->GetMeshes()[iMeshIndex];
	auto& Bones = m_pModel->GetBones();

    auto& BoneMatrices = pMesh->GetBoneMatrices();
    auto& BoneIndices = pMesh->GetBoneIndices();
    auto& OffsetMatrix = pMesh->GetOffsetMatrices();
    

    for (uint32_t i = 0; i < pMesh->Get_BoneIndex(); i++)
    {
        XMStoreFloat4x4(&BoneMatrices[i],
            XMLoadFloat4x4(&OffsetMatrix[i]) * Bones[BoneIndices[i]]->Get_CombinedTransformationMatrix());
    }

    if (!BoneMatrices.empty())
    {

        D3D11_MAPPED_SUBRESOURCE MappedResource{};

        if (FAILED(pContext->Map(pMesh->GetCBBones().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
        {
            return E_FAIL;
        }

        _float4x4* pBoneMatrices = reinterpret_cast<_float4x4*>(MappedResource.pData);

        for (uint32_t i = 0; i < 512; ++i)
        {
            XMStoreFloat4x4(&pBoneMatrices[i], XMMatrixIdentity());
        }

        const uint32_t iBoneCount = static_cast<uint32_t>(std::min<size_t>(BoneMatrices.size(), 512));

        memcpy(pBoneMatrices, BoneMatrices.data(), sizeof(_float4x4) * iBoneCount);

        pContext->Unmap(pMesh->GetCBBones().Get(), 0);

        ID3D11Buffer* pCBBones = pMesh->GetCBBones().Get();

        pContext->VSSetConstantBuffers(2, 1, &pCBBones);
    }




    return S_OK;

}


HRESULT CComModelInstance::Bind_Materials(ID3D11DeviceContext* pContext, uint32_t iMeshIndex, AI_TEXTURE_TYPE eMaterialType, uint32_t iTextureIndex)
{
	auto Materials = m_pModel->GetMaterials();
	auto Mesh = m_pModel->GetMeshes();

	auto Textures = Materials[Mesh[iMeshIndex]->Get_MaterialIndex()]->GetTextures(); 
 


    if (Textures[eMaterialType].size() == 0)
    {
        pContext->PSSetShaderResources(eMaterialType, 1, Textures[0].front()->GetSRV().GetAddressOf());
        return S_OK;
    }

    pContext->PSSetShaderResources(eMaterialType, 1, Textures[eMaterialType][iTextureIndex]->GetSRV().GetAddressOf());


    return S_OK;


}

// 모델의 단일 텍스쳐 반환
SPtr<CResTexture2D> CComModelInstance::Get_MeshTexture(uint32_t iMeshIndex, AI_TEXTURE_TYPE eMaterialType, uint32_t iTextureIndex) {
    auto Materials = m_pModel->GetMaterials();
    auto Mesh = m_pModel->GetMeshes();
    auto Textures = Materials[Mesh[iMeshIndex]->Get_MaterialIndex()]->GetTextures();

    return Textures[eMaterialType][iTextureIndex];
}

UPtr<CComModelInstance> CComModelInstance::Create()
{
    auto pInstance = ToUPtr(new CComModelInstance{});
    if (FAILED(pInstance->InitializePrototype()))
    {
        MSG_BOX("Failed to Created : CComModelInstance");
        return nullptr;
    }
    return pInstance;
}

UPtr<CPrototype> CComModelInstance::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CComModelInstance{ *this });
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned: CComModelInstance");
        return nullptr;
    }
    return pInstance;
}