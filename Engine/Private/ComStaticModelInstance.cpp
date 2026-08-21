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

	ImGui::InputText("Json Name", szJsonName, IM_ARRAYSIZE(szJsonName));

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
		m_sGroupTag = pDesc->sGroupTag;
		m_sResTag = pDesc->sResTag;
        //"PERMANENT", "Prototype_Component_StaticModelInstance"
        if (m_pModel == nullptr)
        {
            return E_FAIL;
        }
    }

    return S_OK;
}


VOID CComStaticModelInstance::Bind_Textures(ID3D11DeviceContext* pContext, uint32_t _MeshIndex) {
	SPtr<CResTexture2D> DiffuseTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_DIFFUSE");
	if (auto Resource = Get_MeshTexture(_MeshIndex, AI_TEXTURE_TYPE::aiTextureType_DIFFUSE, 0)) {
		DiffuseTexture = Resource;
	}
	pContext->PSSetShaderResources(0, 1, DiffuseTexture->GetSRV().GetAddressOf());

	SPtr<CResTexture2D> NormalTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_NORMAL");
	if (auto Resource = Get_MeshTexture(_MeshIndex, AI_TEXTURE_TYPE::aiTextureType_NORMALS, 0)) {
		NormalTexture = Resource;
	}
	pContext->PSSetShaderResources(1, 1, NormalTexture->GetSRV().GetAddressOf());

	SPtr<CResTexture2D> SMROTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_SMRO");
	if (auto Resource = Get_MeshTexture(_MeshIndex, AI_TEXTURE_TYPE::aiTextureType_METALNESS, 0)) {
		SMROTexture = Resource;
	}
	pContext->PSSetShaderResources(2, 1, SMROTexture->GetSRV().GetAddressOf());

	SPtr<CResTexture2D> EmissiveTexture = E::CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_EMISSIVE");
	if (auto Resource =	Get_MeshTexture(_MeshIndex, AI_TEXTURE_TYPE::aiTextureType_EMISSIVE, 0)) {
		EmissiveTexture = Resource;
	}
	pContext->PSSetShaderResources(3, 1, EmissiveTexture->GetSRV().GetAddressOf());
}

VOID CComStaticModelInstance::Bind_Materials(ID3D11DeviceContext* pContext, _float3 _EmissiveColor, _float _EmissiveIntensity, _float3 _DissolveColor, _float _DissolveIntensity, _float _ObjectAlpha,
	_float _NormalIntensity, _float _MetallicIntensity, _float _RoughnessIntensity, _float _AmbientIntensity)
{
	auto MaterialConstantBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_MATERIAL");
	D3D11_MAPPED_SUBRESOURCE MRES;
	if (SUCCEEDED(pContext->Map(MaterialConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
	{
		CB_MATERIAL   CMMAT{};

		CMMAT.EmissiveColor		 = _EmissiveColor;
		CMMAT.EmissiveIntensity  = _EmissiveIntensity;
								 
		CMMAT.DissolveColor		 = _DissolveColor;
		CMMAT.DissolveIntensity  = _DissolveIntensity;
								 
		CMMAT.ObjectAlpha		 = _ObjectAlpha;
								 
		CMMAT.NormalIntensity	 = _NormalIntensity;
		CMMAT.MetallicIntensity  = _MetallicIntensity;
		CMMAT.RoughnessIntensity = _RoughnessIntensity;
		CMMAT.AmbientIntensity	 = _AmbientIntensity;

		memcpy(MRES.pData, &CMMAT, sizeof(CB_MATERIAL));
		pContext->Unmap(MaterialConstantBuffer->GetCBuffer().Get(), 0);
	}
	pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::MATERIAL), 1, MaterialConstantBuffer->GetCBuffer().GetAddressOf());
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

	// 모델 파일이 .bin이면 저장하지 않음
	if (_stricmp(modelPath.extension().string().c_str(), ".bin") == 0)
		return E_FAIL;

	// 파일 이름만 저장
	std::string fbxName = modelPath.filename().string();

	if (fbxName.empty())
		return E_FAIL;

	nlohmann::json j;

	// 기존 파일이 있으면 읽어와서 내용 유지
	if (std::filesystem::exists(savePath))
	{
		std::ifstream inFile(savePath);
		if (inFile.is_open())
		{
			try
			{
				inFile >> j;
			}
			catch (...)
			{
				j = nlohmann::json{}; // 파싱 실패하면 그냥 새로 시작
			}
			inFile.close();
		}
	}

	// 기존 "fbx" 값을 리스트로 통일
	std::vector<std::string> fbxList;

	if (j.contains("fbx"))
	{
		if (j["fbx"].is_string())
		{
			fbxList.push_back(j["fbx"].get<std::string>());
		}
		else if (j["fbx"].is_array())
		{
			for (const auto& elem : j["fbx"])
			{
				if (elem.is_string())
					fbxList.push_back(elem.get<std::string>());
			}
		}
	}

	// 중복 방지 후 새 이름 추가
	if (std::find(fbxList.begin(), fbxList.end(), fbxName) == fbxList.end())
		fbxList.push_back(fbxName);

	// 배열로 저장
	j["fbx"] = fbxList;

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
