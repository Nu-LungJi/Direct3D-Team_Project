#include "pch.h"
#include "GameInstance.h"
#include "ComStaticModelInstance.h"
#include "ResStaticModelMesh.h"
#include "ResModelMaterial.h"
#include "ResModel.h"
NS_USING(Engine)



CComStaticModelInstance::CComStaticModelInstance()
{


}

CComStaticModelInstance::~CComStaticModelInstance()
{
}


HRESULT CComStaticModelInstance::Initialize(void* pArg)
{
    if (FAILED(CComponent::Initialize(pArg)))
    {
        return E_FAIL;
    }

    if (pArg != nullptr) {
        CComStaticModelInstance::DESC* pDesc = reinterpret_cast<CComStaticModelInstance::DESC*>(pArg);
        m_pModel = CGameInstance::Get().GetResourceFirst<CResStaticModel>(pDesc->sGroupTag, pDesc->sResTag);
        if (m_pModel == nullptr)
        {
            return E_FAIL;
        }
    }

    return S_OK;
}


HRESULT CComStaticModelInstance::Bind_Materials(ID3D11DeviceContext* pContext, uint32_t iMeshIndex, AI_TEXTURE_TYPE eMaterialType, uint32_t iTextureIndex)
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
SPtr<CResTexture2D> CComStaticModelInstance::Get_MeshTexture(uint32_t iMeshIndex, AI_TEXTURE_TYPE eMaterialType, uint32_t iTextureIndex) {
    auto Materials = m_pModel->GetMaterials();
    auto Mesh = m_pModel->GetMeshes();
    auto Textures = Materials[Mesh[iMeshIndex]->Get_MaterialIndex()]->GetTextures();

    return Textures[eMaterialType][iTextureIndex];
}

UPtr<CComStaticModelInstance> CComStaticModelInstance::Create()
{
    auto pInstance = ToUPtr(new CComStaticModelInstance{});
    if (FAILED(pInstance->InitializePrototype()))
    {
        MSG_BOX("Failed to Created : CComStaticModelInstance");
        return nullptr;
    }
    return pInstance;
}

UPtr<CPrototype> CComStaticModelInstance::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CComStaticModelInstance{ *this });
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned: CComStaticModelInstance");
        return nullptr;
    }
    return pInstance;
}