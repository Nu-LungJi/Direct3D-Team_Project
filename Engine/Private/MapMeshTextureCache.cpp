#include "pch.h"
#include "MapMeshTextureCache.h"

NS_USING(Engine)

const CMapMeshTextureCache::MODEL_TEXTURE_SETS* CMapMeshTextureCache::GetOrCreateTextureSets(const SPtr<CResStaticModel>& model)
{
	if (model == nullptr)
		return nullptr;

	if (const auto iter = m_ModelTextureSets.find(model); iter != m_ModelTextureSets.end())
		return &iter->second;

	// 모델에 텍스처가 없는 슬롯도 항상 안전하게 바인딩되도록 기본값으로 시작한다
	MESH_TEXTURE_SET defaultTextures{
		CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_DIFFUSE"),
		CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_NORMAL"),
		CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_SMRO"),
		CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_EMISSIVE")
	};
	if (std::ranges::any_of(defaultTextures, [](const auto& texture) { return texture == nullptr; }))
		return nullptr;

	// 모델의 메시 수와 같은 크기로 만들어 메시 인덱스로 바로 접근하게 한다
	MODEL_TEXTURE_SETS textureSets(model->Get_NumMeshes(), defaultTextures);
	for (uint32_t meshIndex = 0; meshIndex < textureSets.size(); ++meshIndex)
	{
		auto& meshTextures = textureSets[meshIndex];
		if (auto texture = GetMapMeshTexture(model, meshIndex, AI_TEXTURE_TYPE::aiTextureType_DIFFUSE))
			meshTextures[0] = std::move(texture);
		if (auto texture = GetMapMeshTexture(model, meshIndex, AI_TEXTURE_TYPE::aiTextureType_NORMALS))
			meshTextures[1] = std::move(texture);
		if (auto texture = GetMapMeshTexture(model, meshIndex, AI_TEXTURE_TYPE::aiTextureType_METALNESS))
			meshTextures[2] = std::move(texture);
		if (auto texture = GetMapMeshTexture(model, meshIndex, AI_TEXTURE_TYPE::aiTextureType_EMISSIVE))
			meshTextures[3] = std::move(texture);
	}

	const auto iter = m_ModelTextureSets.emplace(model, std::move(textureSets)).first;
	return &iter->second;
}

HRESULT CMapMeshTextureCache::BindTextures(ID3D11DeviceContext* context, const MODEL_TEXTURE_SETS& textureSets, uint32_t meshIndex) const
{
	if (context == nullptr || meshIndex >= textureSets.size())
		return E_INVALIDARG;

	ID3D11ShaderResourceView* shaderResourceViews[TEXTURE_COUNT]{};
	for (size_t i = 0; i < TEXTURE_COUNT; ++i)
	{
		if (textureSets[meshIndex][i] == nullptr)
			return E_FAIL;

		shaderResourceViews[i] = textureSets[meshIndex][i]->GetSRV().Get();
	}
	context->PSSetShaderResources(0, TEXTURE_COUNT, shaderResourceViews);

	return S_OK;
}

void CMapMeshTextureCache::ClearAll()
{
	m_ModelTextureSets.clear();
}

void CMapMeshTextureCache::EraseModel(const SPtr<CResStaticModel>& model)
{
	m_ModelTextureSets.erase(model);
}

SPtr<CResTexture2D> CMapMeshTextureCache::GetMapMeshTexture(const SPtr<CResStaticModel>& model, uint32_t meshIndex, AI_TEXTURE_TYPE materialType)
{
	if (model == nullptr)
		return nullptr;

	auto& meshes = model->GetMeshes();

	if (meshIndex >= meshes.size() || meshes[meshIndex] == nullptr)
		return nullptr;

	auto& materials = model->GetMaterials();
	const uint32_t materialIndex = meshes[meshIndex]->Get_MaterialIndex();

	if (materialIndex >= materials.size() || materials[materialIndex] == nullptr)
		return nullptr;

	auto textures = materials[materialIndex]->GetTextures();
	if (textures[materialType].empty())
		return nullptr;

	return textures[materialType].front();
}
