#include "pch.h"
#include "GameInstance.h"
#include "ComStaticModelInstance.h"
#include "ResStaticModelMesh.h"
#include "ResModelMaterial.h"
#include "ResModel.h"
NS_USING(Engine)

void CComStaticModelInstance::UpdateGUI()
{
	static char szJsonName[MAX_PATH] = "StaticModel.json";

	if (ImGui::Button("Save Json"))
	{
		std::string saveName = szJsonName;

		if (saveName.empty())
			return;

		std::filesystem::path savePath =
			std::filesystem::path("./Resources/SampleClient/Models/StaticModelJson") / saveName;

		if (savePath.extension().empty())
			savePath.replace_extension(".json");

		Save_Binary_Json(savePath.string());
	}
}
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
        //"PERMANENT", "Prototype_Component_StaticModelInstance"
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


HRESULT CComStaticModelInstance::ChangeModel(const StringID& sGroupTag, const StringID& sResTag)
{
    auto pModel = CGameInstance::Get().GetResourceFirst<CResStaticModel>(sGroupTag, sResTag);
    if (pModel == nullptr)
    {
        return E_FAIL;
    }

    m_pModel = pModel;
    return S_OK;
}
HRESULT CComStaticModelInstance::Save_Binary_Json(std::string outpath)
{
	if (outpath.empty())
		return E_FAIL;

	std::filesystem::path savePath(outpath);

	// 확장자가 없으면 .json 붙이기
	if (savePath.extension().empty())
		savePath.replace_extension(".json");

	// 폴더 없으면 생성
	if (!savePath.parent_path().empty())
		std::filesystem::create_directories(savePath.parent_path());

	// 현재 모델 없으면 실패
	if (m_pModel == nullptr)
		return E_FAIL;

	std::filesystem::path modelPath = m_pModel->GetPath();

	// 파일 이름만 저장
	std::string fbxName = modelPath.filename().string();


	nlohmann::json j;

	j["fbx"] = fbxName;

	std::ofstream file(savePath, std::ios::out);

	if (!file.is_open())
		return E_FAIL;

	file << j.dump(4);
	file.close();

	return S_OK;
}
 

// 모델의 단일 텍스쳐 반환
SPtr<CResTexture2D> CComStaticModelInstance::Get_MeshTexture(uint32_t iMeshIndex, AI_TEXTURE_TYPE eMaterialType, uint32_t iTextureIndex) {
    auto Materials = m_pModel->GetMaterials();
    auto Mesh = m_pModel->GetMeshes();
    auto Textures = Materials[Mesh[iMeshIndex]->Get_MaterialIndex()]->GetTextures();
    if (Textures[eMaterialType].size() == 0)
    {
        return nullptr;
    }

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
