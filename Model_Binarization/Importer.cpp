
#include "Importer.h"
#include "Bone.h"
#include "Mesh.h"
#include <assimp/config.h>
#include <cctype>
#include <cmath>

namespace
{
	struct WHOLE_MAP_CHUNK_KEY
	{
		int32_t x{};
		int32_t y{};
		int32_t z{};

		bool operator==(const WHOLE_MAP_CHUNK_KEY&) const = default;
	};

	struct WHOLE_MAP_CHUNK_KEY_HASH
	{
		size_t operator()(const WHOLE_MAP_CHUNK_KEY& key) const noexcept
		{
			const size_t hx = std::hash<int32_t>{}(key.x);
			const size_t hy = std::hash<int32_t>{}(key.y);
			const size_t hz = std::hash<int32_t>{}(key.z);
			const size_t hxy = hx ^ (hy + 0x9e3779b9u + (hx << 6) + (hx >> 2));
			return hxy ^ (hz + 0x9e3779b9u + (hxy << 6) + (hxy >> 2));
		}
	};

	std::string NormalizeMaterialTextureToken(std::string value)
	{
		std::replace(value.begin(), value.end(), '\\', '/');
		std::transform(value.begin(), value.end(), value.begin(),
			[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
		return value;
	}

	std::string MakeWholeMapMaterialSignature(
		const std::vector<std::shared_ptr<CMaterial>>& materials,
		uint32_t materialIndex)
	{
		if (materialIndex >= materials.size() || materials[materialIndex] == nullptr)
			return "unique:" + std::to_string(materialIndex);

		std::vector<std::string> textureTokens;
		for (const auto& textureGroup : materials[materialIndex]->m_textures)
		{
			for (const auto& texture : textureGroup)
			{
				if (texture.File.empty())
					continue;

				textureTokens.push_back(
					std::to_string(texture.m_textureType) + ":" +
					std::to_string(texture.m_textureNum) + ":" +
					NormalizeMaterialTextureToken(texture.File) +
					NormalizeMaterialTextureToken(texture.Ext));
			}
		}

		// Missing texture metadata does not prove that two FBX materials are equal.
		// Keep those materials separate to avoid an unsafe whole-map merge.
		if (textureTokens.empty())
			return "unique:" + std::to_string(materialIndex);

		std::sort(textureTokens.begin(), textureTokens.end());
		std::string signature = "textures:";
		for (const auto& token : textureTokens)
		{
			signature += std::to_string(token.size());
			signature += ':';
			signature += token;
		}
		return signature;
	}
}

CImporter::CImporter()
{
}

CImporter::~CImporter()
{
}

HRESULT CImporter::ImportFBXFolder(
	const std::string& strLevelName,
	const std::string& strSourceFolder
)
{
	std::filesystem::path sourcePath(strSourceFolder);

	// 예: strSourceFolder = "../../JUSIN_160_FINAL_TEAM_RESOURCE/SampleClient/Models/OriginData/Skeletal"
	// category = "Skeletal"
	// Static이면 category = "Static"
	std::string category = sourcePath.filename().string();

	if (!std::filesystem::exists(strSourceFolder))
	{
		return E_FAIL;
	}

	// ------------------------------------------------------------
	// OriginData 앞 경로까지만 남긴다.
	// "../../.../Models/OriginData/Skeletal"
	// -> "../../.../Models/"
	// ------------------------------------------------------------
	std::string rootPath = strSourceFolder;

	size_t pos = rootPath.find("OriginData");

	if (pos != std::string::npos)
	{
		rootPath = rootPath.substr(0, pos);
	}
	else
	{
		// OriginData가 경로에 없으면 sourcePath의 부모의 부모를 기준으로 처리
		// 필요 없으면 이 else는 return E_FAIL 해도 됨
		rootPath = sourcePath.parent_path().parent_path().string() + "/";
	}

	// ------------------------------------------------------------
	// strLevelName 무시
	// 최종 basePath:
	// "../../JUSIN_160_FINAL_TEAM_RESOURCE/SampleClient/Models/Skeletal/"
	// "../../JUSIN_160_FINAL_TEAM_RESOURCE/SampleClient/Models/Static/"
	// ------------------------------------------------------------
	std::filesystem::path basePath = std::filesystem::path(rootPath) / category;
	std::filesystem::path originTextureDir =
		MakeTextureOutputDir(std::filesystem::path(rootPath) / "OriginData" / category);
	for (const auto& entry : std::filesystem::recursive_directory_iterator(strSourceFolder))
	{
		if (!entry.is_regular_file())
			continue;

		const auto& path = entry.path();

		std::string ext = path.extension().string();

		if (_stricmp(ext.c_str(), ".fbx") != 0)
			continue;

		std::string inputPath = path.string();
		std::string modelName = path.stem().string();

		std::filesystem::path modelDir;

		if (_stricmp(category.c_str(), "Static") == 0)
		{
			// Static bin은 Models/Static 바로 아래
			modelDir = basePath;
		}
		else
		{
			// Skeletal은 Models/Skeletal/모델이름 아래
			modelDir = basePath / modelName;
		}

		// ------------------------------------------------------------
		// Texture 폴더는 bin 존재 여부와 상관없이 먼저 생성
		// Static texture는 Textures/Static/모델이름/
		// Skeletal texture는 Textures/Skeletal/모델이름/
		// ------------------------------------------------------------
		std::filesystem::path textureDir;

		if (_stricmp(category.c_str(), "Static") == 0)
		{
			textureDir = MakeTextureOutputDir(basePath) / modelName;
		}
		else
		{
			textureDir = MakeTextureOutputDir(modelDir);
		}

		std::filesystem::create_directories(textureDir);

	

		if (HasExtractedModelData(modelDir, modelName))
		{
			continue;
		}

		std::filesystem::create_directories(modelDir);

		if (FAILED(AssimpFBX(inputPath)))
		{
			Clear();
			continue;
		}

		// ------------------------------------------------------------
	// Static bin 저장용 텍스처 복사
	//
	// Textures/OriginData/Static/*.png
	// -> Textures/Static/모델이름/*.png
	// ------------------------------------------------------------
		if (_stricmp(category.c_str(), "Static") == 0)
		{
			CopyUsedTextureFilesToFolder(originTextureDir, textureDir);
		}

		std::filesystem::path outputPath = modelDir / (modelName + ".bin");

		if (FAILED(ExportFBX(outputPath.string())))
		{
			Clear();
			continue;
		}

		Clear();
	}

	return S_OK;
}

HRESULT CImporter::AssimpFBX(const std::string& fbxFileName)
{
    m_index = 0;
    m_bHasAnimation = false;
    m_bHasBone = false;

    uint32_t iFlag = 0;
    iFlag |= aiProcess_ConvertToLeftHanded;
    iFlag |= aiProcess_PopulateArmatureData;
    iFlag |= aiProcess_GlobalScale;
    iFlag |= aiProcess_ImproveCacheLocality;
    iFlag |= aiProcessPreset_TargetRealtime_Fast;
//    iFlag |= aiProcess_OptimizeMeshes;

    Assimp::Importer importer;


    const aiScene* pScene = importer.ReadFile(fbxFileName, iFlag);

    if (!pScene)
        return E_FAIL;

    m_bHasAnimation = pScene->HasAnimations();

    for (uint32_t i = 0; i < pScene->mNumMeshes; ++i)
    {
        if (pScene->mMeshes[i]->HasBones())
        {
            m_bHasBone = true;
            break;
        }
    }

    if (m_bHasBone)
        Ready_Bones(pScene->mRootNode, -1);

    if (pScene->HasMeshes())
        Ready_Mesh(pScene, m_bHasBone);

    if (pScene->HasMaterials())
        Ready_Material(pScene);

    if (m_bHasAnimation)
        Ready_Animation(pScene);

    return S_OK;
}

HRESULT CImporter::ImportWholeMapFBX(
	const std::string& fbxFileName,
	const std::string& outputDirectory,
	float chunkSize)
{
	if (fbxFileName.empty() || outputDirectory.empty() ||
		!std::isfinite(chunkSize) || chunkSize <= 0.f)
	{
		return E_INVALIDARG;
	}

	const std::filesystem::path inputPath(fbxFileName);
	if (!std::filesystem::is_regular_file(inputPath))
		return E_FAIL;

	Clear();
	m_index = 0;
	m_FBXSourceDir = inputPath.parent_path();

	uint32_t flags = 0;
	flags |= aiProcess_ConvertToLeftHanded;
	flags |= aiProcess_GlobalScale;
	flags |= aiProcess_ImproveCacheLocality;
	flags |= aiProcessPreset_TargetRealtime_Fast;
	// This path is static-map-only. Baking the complete node hierarchy makes all
	// vertices and bounds share the FBX root-local coordinate system.
	flags |= aiProcess_PreTransformVertices;

	Assimp::Importer importer;
	// Bake node transforms into root-local vertex positions without Assimp's
	// default global merge of meshes that share a material. Spatial chunking and
	// material batching must happen explicitly after this import step.
	importer.SetPropertyBool(AI_CONFIG_PP_PTV_KEEP_HIERARCHY, true);
	const aiScene* scene = importer.ReadFile(inputPath.string(), flags);
	if (scene == nullptr || !scene->HasMeshes())
	{
		Clear();
		return E_FAIL;
	}

	for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
	{
		if (scene->mMeshes[i] != nullptr && scene->mMeshes[i]->HasBones())
		{
			Clear();
			return E_FAIL;
		}
	}

	Ready_Mesh(scene, false);

	// PreTransformVertices is only used for root-local geometry. Read materials
	// from an unmodified scene so FBX texture slots and original material indices
	// are preserved in the chunk binaries.
	Assimp::Importer materialImporter;
	const uint32_t materialFlags = flags & ~static_cast<uint32_t>(aiProcess_PreTransformVertices);
	const aiScene* materialScene = materialImporter.ReadFile(inputPath.string(), materialFlags);
	if (materialScene == nullptr || !materialScene->HasMaterials() ||
		materialScene->mNumMaterials != scene->mNumMaterials ||
		FAILED(Ready_Material(materialScene)))
	{
		Clear();
		return E_FAIL;
	}

	uint32_t sourceTextureReferenceCount = 0;
	for (uint32_t materialIndex = 0; materialIndex < materialScene->mNumMaterials; ++materialIndex)
	{
		const aiMaterial* material = materialScene->mMaterials[materialIndex];
		if (material == nullptr)
			continue;
		for (uint32_t textureType = 0; textureType < AI_TEXTURE_TYPE_MAX; ++textureType)
			sourceTextureReferenceCount += material->GetTextureCount(static_cast<aiTextureType>(textureType));
	}
	std::cout << "Whole-map source materials: " << materialScene->mNumMaterials
		<< ", texture references: " << sourceTextureReferenceCount << '\n';
	if (sourceTextureReferenceCount == 0)
	{
		std::cout << "Materials contain no Assimp file-texture slots. Sample names:\n";
		for (uint32_t materialIndex = 0; materialIndex < (std::min)(materialScene->mNumMaterials, 10u); ++materialIndex)
		{
			aiString materialName;
			if (materialScene->mMaterials[materialIndex]->Get(AI_MATKEY_NAME, materialName) == AI_SUCCESS)
				std::cout << "  [" << materialIndex << "] " << materialName.C_Str() << '\n';
		}
	}

	struct MERGED_BATCH
	{
		std::shared_ptr<CMesh> mesh{};
		std::unordered_map<const CMesh*, std::unordered_map<uint32_t, uint32_t>> vertexRemaps{};
	};

	using MATERIAL_BATCHES = std::unordered_map<std::string, MERGED_BATCH>;
	std::unordered_map<WHOLE_MAP_CHUNK_KEY, MATERIAL_BATCHES, WHOLE_MAP_CHUNK_KEY_HASH> chunkBatches;
	uint64_t sourceIndexCount = 0;

	for (const auto& sourceMesh : Meshes)
	{
		if (sourceMesh == nullptr || sourceMesh->m_vertices == nullptr || sourceMesh->m_indices == nullptr ||
			sourceMesh->m_vertices->empty() || sourceMesh->m_indices->empty())
		{
			continue;
		}

		if (sourceMesh->m_indices->size() % 3 != 0)
		{
			Clear();
			return E_FAIL;
		}

		sourceIndexCount += sourceMesh->m_indices->size();
		for (size_t triangleStart = 0; triangleStart < sourceMesh->m_indices->size(); triangleStart += 3)
		{
			const uint32_t sourceIndices[3] = {
				(*sourceMesh->m_indices)[triangleStart],
				(*sourceMesh->m_indices)[triangleStart + 1],
				(*sourceMesh->m_indices)[triangleStart + 2]
			};
			if (sourceIndices[0] >= sourceMesh->m_vertices->size() ||
				sourceIndices[1] >= sourceMesh->m_vertices->size() ||
				sourceIndices[2] >= sourceMesh->m_vertices->size())
			{
				Clear();
				return E_FAIL;
			}

			const auto& p0 = (*sourceMesh->m_vertices)[sourceIndices[0]].vPosition;
			const auto& p1 = (*sourceMesh->m_vertices)[sourceIndices[1]].vPosition;
			const auto& p2 = (*sourceMesh->m_vertices)[sourceIndices[2]].vPosition;
			const XMFLOAT3 triangleCenter{
				(p0.x + p1.x + p2.x) / 3.f,
				(p0.y + p1.y + p2.y) / 3.f,
				(p0.z + p1.z + p2.z) / 3.f
			};
			const WHOLE_MAP_CHUNK_KEY key{
				static_cast<int32_t>(std::floor(triangleCenter.x / chunkSize)),
				static_cast<int32_t>(std::floor(triangleCenter.y / chunkSize)),
				static_cast<int32_t>(std::floor(triangleCenter.z / chunkSize))
			};

			const std::string materialSignature =
				MakeWholeMapMaterialSignature(Materials, sourceMesh->m_materialIndex);
			auto& batch = chunkBatches[key][materialSignature];
			if (batch.mesh == nullptr)
			{
				batch.mesh = std::make_shared<CMesh>();
				batch.mesh->m_name = "Merged_Material_" + std::to_string(sourceMesh->m_materialIndex);
				batch.mesh->m_materialIndex = sourceMesh->m_materialIndex;
				batch.mesh->m_min = { FLT_MAX, FLT_MAX, FLT_MAX };
				batch.mesh->m_max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
				batch.mesh->m_vertices = std::make_shared<std::vector<VTXMESH>>();
				batch.mesh->m_indices = std::make_shared<std::vector<uint32_t>>();
			}

			auto& sourceVertexRemap = batch.vertexRemaps[sourceMesh.get()];
			for (const uint32_t sourceIndex : sourceIndices)
			{
				auto remapIt = sourceVertexRemap.find(sourceIndex);
				uint32_t mergedIndex = 0;
				if (remapIt == sourceVertexRemap.end())
				{
					mergedIndex = static_cast<uint32_t>(batch.mesh->m_vertices->size());
					const VTXMESH& vertex = (*sourceMesh->m_vertices)[sourceIndex];
					batch.mesh->m_vertices->push_back(vertex);
					sourceVertexRemap.emplace(sourceIndex, mergedIndex);
					batch.mesh->m_min.x = (std::min)(batch.mesh->m_min.x, vertex.vPosition.x);
					batch.mesh->m_min.y = (std::min)(batch.mesh->m_min.y, vertex.vPosition.y);
					batch.mesh->m_min.z = (std::min)(batch.mesh->m_min.z, vertex.vPosition.z);
					batch.mesh->m_max.x = (std::max)(batch.mesh->m_max.x, vertex.vPosition.x);
					batch.mesh->m_max.y = (std::max)(batch.mesh->m_max.y, vertex.vPosition.y);
					batch.mesh->m_max.z = (std::max)(batch.mesh->m_max.z, vertex.vPosition.z);
				}
				else
				{
					mergedIndex = remapIt->second;
				}
				batch.mesh->m_indices->push_back(mergedIndex);
			}
		}
	}

	using CHUNK_MESHES = std::vector<std::shared_ptr<CMesh>>;
	std::unordered_map<WHOLE_MAP_CHUNK_KEY, CHUNK_MESHES, WHOLE_MAP_CHUNK_KEY_HASH> chunks;
	uint64_t mergedIndexCount = 0;
	for (auto& [key, materialBatches] : chunkBatches)
	{
		std::vector<MERGED_BATCH*> sortedBatches;
		sortedBatches.reserve(materialBatches.size());
		for (auto& [materialSignature, batch] : materialBatches)
			sortedBatches.push_back(&batch);
		std::sort(sortedBatches.begin(), sortedBatches.end(),
			[](const MERGED_BATCH* lhs, const MERGED_BATCH* rhs)
			{
				return lhs->mesh->m_materialIndex < rhs->mesh->m_materialIndex;
			});

		auto& chunkMeshes = chunks[key];
		chunkMeshes.reserve(sortedBatches.size());
		for (MERGED_BATCH* batch : sortedBatches)
		{
			auto& mergedMesh = batch->mesh;
			mergedIndexCount += mergedMesh->m_indices->size();
			chunkMeshes.push_back(std::move(mergedMesh));
		}
	}

	if (mergedIndexCount != sourceIndexCount)
	{
		Clear();
		return E_FAIL;
	}

	if (chunks.empty())
	{
		Clear();
		return E_FAIL;
	}

	std::vector<WHOLE_MAP_CHUNK_KEY> sortedKeys;
	sortedKeys.reserve(chunks.size());
	for (const auto& [key, unused] : chunks)
		sortedKeys.push_back(key);

	std::sort(sortedKeys.begin(), sortedKeys.end(), [](const auto& lhs, const auto& rhs)
	{
		if (lhs.z != rhs.z)
			return lhs.z < rhs.z;
		if (lhs.y != rhs.y)
			return lhs.y < rhs.y;
		return lhs.x < rhs.x;
	});

	const std::filesystem::path outputDir(outputDirectory);
	std::error_code directoryError;
	std::filesystem::create_directories(outputDir, directoryError);
	if (directoryError)
	{
		Clear();
		return E_FAIL;
	}

	const std::string modelName = inputPath.stem().string();
	nlohmann::json manifest;
	manifest["format"] = "whole_map_render_chunks_v3";
	manifest["source"] = inputPath.filename().string();
	manifest["modelName"] = modelName;
	manifest["chunkSize"] = chunkSize;
	manifest["coordinateSpace"] = "fbx_root_local_xyz";
	manifest["assignment"] = "triangle_centroid";
	manifest["vertexSpace"] = "chunk_origin_relative";
	manifest["sourceMeshCount"] = Meshes.size();
	manifest["sourceIndexCount"] = sourceIndexCount;
	manifest["mergedIndexCount"] = mergedIndexCount;
	manifest["chunks"] = nlohmann::json::array();

	for (const WHOLE_MAP_CHUNK_KEY& key : sortedKeys)
	{
		const auto& chunkMeshes = chunks.at(key);

		// Rebase each render chunk around the center of its actual geometry. The
		// saved origin must be applied as the MapMeshObject's local position at
		// runtime (before the whole-map world transform).
		XMFLOAT3 sourceMin{ FLT_MAX, FLT_MAX, FLT_MAX };
		XMFLOAT3 sourceMax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
		for (const auto& mesh : chunkMeshes)
		{
			sourceMin.x = (std::min)(sourceMin.x, mesh->m_min.x);
			sourceMin.y = (std::min)(sourceMin.y, mesh->m_min.y);
			sourceMin.z = (std::min)(sourceMin.z, mesh->m_min.z);
			sourceMax.x = (std::max)(sourceMax.x, mesh->m_max.x);
			sourceMax.y = (std::max)(sourceMax.y, mesh->m_max.y);
			sourceMax.z = (std::max)(sourceMax.z, mesh->m_max.z);
		}

		const XMFLOAT3 localOrigin{
			(sourceMin.x + sourceMax.x) * 0.5f,
			(sourceMin.y + sourceMax.y) * 0.5f,
			(sourceMin.z + sourceMax.z) * 0.5f
		};

		for (const auto& mesh : chunkMeshes)
		{
			for (VTXMESH& vertex : *mesh->m_vertices)
			{
				vertex.vPosition.x -= localOrigin.x;
				vertex.vPosition.y -= localOrigin.y;
				vertex.vPosition.z -= localOrigin.z;
			}

			mesh->m_min.x -= localOrigin.x;
			mesh->m_min.y -= localOrigin.y;
			mesh->m_min.z -= localOrigin.z;
			mesh->m_max.x -= localOrigin.x;
			mesh->m_max.y -= localOrigin.y;
			mesh->m_max.z -= localOrigin.z;
		}

		const std::string fileName =
			"SM_" + modelName + "_Chunk_" + std::to_string(key.x) + "_" +
			std::to_string(key.y) + "_" + std::to_string(key.z) + ".bin";
		const std::filesystem::path chunkPath = outputDir / fileName;

		if (FAILED(ExportStaticMeshSubset(chunkPath, chunkMeshes)))
		{
			Clear();
			return E_FAIL;
		}

		XMFLOAT3 minBounds{ FLT_MAX, FLT_MAX, FLT_MAX };
		XMFLOAT3 maxBounds{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
		for (const auto& mesh : chunkMeshes)
		{
			minBounds.x = (std::min)(minBounds.x, mesh->m_min.x);
			minBounds.y = (std::min)(minBounds.y, mesh->m_min.y);
			minBounds.z = (std::min)(minBounds.z, mesh->m_min.z);
			maxBounds.x = (std::max)(maxBounds.x, mesh->m_max.x);
			maxBounds.y = (std::max)(maxBounds.y, mesh->m_max.y);
			maxBounds.z = (std::max)(maxBounds.z, mesh->m_max.z);
		}

		manifest["chunks"].push_back({
			{ "x", key.x },
			{ "y", key.y },
			{ "z", key.z },
			{ "file", fileName },
			{ "meshCount", chunkMeshes.size() },
			{ "localOrigin", { localOrigin.x, localOrigin.y, localOrigin.z } },
			{ "localMin", { minBounds.x, minBounds.y, minBounds.z } },
			{ "localMax", { maxBounds.x, maxBounds.y, maxBounds.z } }
		});
	}

	manifest["chunkCount"] = sortedKeys.size();
	const std::filesystem::path manifestPath = outputDir / (modelName + "_RenderChunks.json");
	std::ofstream manifestFile(manifestPath);
	if (!manifestFile.is_open())
	{
		Clear();
		return E_FAIL;
	}
	manifestFile << manifest.dump(2);
	if (!manifestFile.good())
	{
		Clear();
		return E_FAIL;
	}

	std::cout << "Whole-map conversion complete: " << sortedKeys.size()
		<< " chunks written to " << outputDir.string() << '\n';
	Clear();
	return S_OK;
}

HRESULT CImporter::ExportStaticMeshSubset(
	const std::filesystem::path& outpath,
	const std::vector<std::shared_ptr<CMesh>>& meshes) const
{
	if (meshes.empty())
		return E_INVALIDARG;

	std::ofstream file(outpath, std::ios::binary);
	if (!file.is_open())
		return E_FAIL;

	std::vector<char> localMeshBuffer;
	std::vector<char> localMaterialBuffer;
	const auto append = [](std::vector<char>& buffer, const void* data, size_t size)
	{
		const size_t oldSize = buffer.size();
		buffer.resize(oldSize + size);
		memcpy(buffer.data() + oldSize, data, size);
	};

	// A render chunk only needs the materials referenced by its merged meshes.
	// Remap sparse FBX material indices to a compact chunk-local range.
	std::vector<uint32_t> usedMaterialIndices;
	usedMaterialIndices.reserve(meshes.size());
	for (const auto& mesh : meshes)
	{
		if (mesh == nullptr || mesh->m_materialIndex >= Materials.size() ||
			Materials[mesh->m_materialIndex] == nullptr)
		{
			return E_FAIL;
		}
		usedMaterialIndices.push_back(mesh->m_materialIndex);
	}
	std::sort(usedMaterialIndices.begin(), usedMaterialIndices.end());
	usedMaterialIndices.erase(
		std::unique(usedMaterialIndices.begin(), usedMaterialIndices.end()),
		usedMaterialIndices.end());

	std::unordered_map<uint32_t, uint32_t> materialRemap;
	materialRemap.reserve(usedMaterialIndices.size());
	for (uint32_t localIndex = 0; localIndex < usedMaterialIndices.size(); ++localIndex)
		materialRemap.emplace(usedMaterialIndices[localIndex], localIndex);

	MODEL_FILE_HEADER header{};
	header.bHasBone = false;
	header.bHasAnimation = false;
	header.MeshCount = static_cast<uint32_t>(meshes.size());
	header.MaterialCount = static_cast<uint32_t>(usedMaterialIndices.size());
	header.AnimationCount = 0;
	header.BoneCount = 0;
	file.write(reinterpret_cast<const char*>(&header), sizeof(header));

	for (const auto& mesh : meshes)
	{
		if (mesh == nullptr || mesh->m_vertices == nullptr || mesh->m_indices == nullptr)
			return E_FAIL;

		const uint32_t nameLength = static_cast<uint32_t>(mesh->m_name.size());
		const uint32_t vertexCount = static_cast<uint32_t>(mesh->m_vertices->size());
		const uint32_t indexCount = static_cast<uint32_t>(mesh->m_indices->size());
		const uint32_t meshSize =
			sizeof(uint32_t) + nameLength +
			sizeof(uint32_t) +
			sizeof(XMFLOAT3) * 2 +
			sizeof(uint32_t) * 2 +
			static_cast<uint32_t>(sizeof(VTXMESH) * vertexCount) +
			static_cast<uint32_t>(sizeof(uint32_t) * indexCount);

		append(localMeshBuffer, &meshSize, sizeof(meshSize));
		append(localMeshBuffer, &nameLength, sizeof(nameLength));
		append(localMeshBuffer, mesh->m_name.data(), nameLength);
		const uint32_t localMaterialIndex = materialRemap.at(mesh->m_materialIndex);
		append(localMeshBuffer, &localMaterialIndex, sizeof(localMaterialIndex));
		append(localMeshBuffer, &mesh->m_min, sizeof(mesh->m_min));
		append(localMeshBuffer, &mesh->m_max, sizeof(mesh->m_max));
		append(localMeshBuffer, &vertexCount, sizeof(vertexCount));
		append(localMeshBuffer, &indexCount, sizeof(indexCount));
		append(localMeshBuffer, mesh->m_vertices->data(), sizeof(VTXMESH) * vertexCount);
		append(localMeshBuffer, mesh->m_indices->data(), sizeof(uint32_t) * indexCount);
	}

	ChunkHeader meshChunk{ ChunkType::CHUNK_MESH, static_cast<uint32_t>(localMeshBuffer.size()) };
	file.write(reinterpret_cast<const char*>(&meshChunk), sizeof(meshChunk));
	file.write(localMeshBuffer.data(), static_cast<std::streamsize>(localMeshBuffer.size()));

	for (const uint32_t sourceMaterialIndex : usedMaterialIndices)
	{
		const auto& material = Materials[sourceMaterialIndex];
		if (material == nullptr)
			return E_FAIL;

		uint32_t materialSize = sizeof(uint32_t) * 2;
		for (const auto& textureGroup : material->m_textures)
		{
			materialSize += sizeof(uint32_t);
			for (const auto& texture : textureGroup)
			{
				materialSize += sizeof(uint32_t) * 3;
				materialSize += static_cast<uint32_t>(texture.File.size() + texture.Ext.size());
			}
		}

		append(localMaterialBuffer, &materialSize, sizeof(materialSize));
		const uint32_t localMaterialIndex = materialRemap.at(sourceMaterialIndex);
		append(localMaterialBuffer, &localMaterialIndex, sizeof(localMaterialIndex));
		const uint32_t textureTypeCount = static_cast<uint32_t>(material->m_textures.size());
		append(localMaterialBuffer, &textureTypeCount, sizeof(textureTypeCount));

		for (const auto& textureGroup : material->m_textures)
		{
			const uint32_t textureCount = static_cast<uint32_t>(textureGroup.size());
			append(localMaterialBuffer, &textureCount, sizeof(textureCount));
			for (const auto& texture : textureGroup)
			{
				append(localMaterialBuffer, &texture.m_textureType, sizeof(texture.m_textureType));
				const uint32_t fileLength = static_cast<uint32_t>(texture.File.size());
				append(localMaterialBuffer, &fileLength, sizeof(fileLength));
				append(localMaterialBuffer, texture.File.data(), fileLength);
				const uint32_t extensionLength = static_cast<uint32_t>(texture.Ext.size());
				append(localMaterialBuffer, &extensionLength, sizeof(extensionLength));
				append(localMaterialBuffer, texture.Ext.data(), extensionLength);
			}
		}
	}

	ChunkHeader materialChunk{ ChunkType::CHUNK_MATERIAL, static_cast<uint32_t>(localMaterialBuffer.size()) };
	file.write(reinterpret_cast<const char*>(&materialChunk), sizeof(materialChunk));
	file.write(localMaterialBuffer.data(), static_cast<std::streamsize>(localMaterialBuffer.size()));

	return file.good() ? S_OK : E_FAIL;
}
HRESULT CImporter::ExportFBX(const std::string& outpath)
{
	std::filesystem::path path(outpath);

	std::string modelName = path.stem().string();

	std::string prefix;

	if (!m_bHasBone && !m_bHasAnimation)
		prefix = "SM_";
	else if (!m_bHasBone && m_bHasAnimation)
		prefix = "SM_";
	else if (m_bHasBone && m_bHasAnimation)
		prefix = "SK_";
	else
		prefix = "SK_";

	std::filesystem::path modelOutputDir = path.parent_path();

	std::filesystem::path finalPath =
		modelOutputDir / (prefix + modelName + ".bin");

	fileParentName = modelOutputDir.string();

	// ------------------------------------------------------------
	// Texture output path
	// Models -> Textures
	// Static 모델은 Textures/Static/모델이름/ 으로 분리
	// Skeletal은 기존처럼 Textures/Skeletal/모델이름/
	// ------------------------------------------------------------
	std::filesystem::path textureOutputDir =
		MakeTextureOutputDir(modelOutputDir);

	if (!m_bHasBone)
	{
		std::string dirName = modelOutputDir.filename().string();

		if (_stricmp(dirName.c_str(), "Static") == 0)
		{
			textureOutputDir /= modelName;
		}
	}

	textureParentName = textureOutputDir.string();

	std::filesystem::create_directories(textureOutputDir);

	if (!m_bHasBone && !m_bHasAnimation)
	{
		ExportStatic(finalPath.string());
	}
	else if (!m_bHasBone && m_bHasAnimation)
	{
		// ExportStaticAnim(finalPath.string());
	}
	else if (m_bHasBone)
	{
		ExportSkeletal(finalPath.string());
	}
	else
	{
		// ExportSkeletalAnim(finalPath.string());
	}

	if (m_bHasAnimation)
	{
		ExportAnimation(finalPath.string());
	}

	return S_OK;
}
HRESULT CImporter::ExportStatic(const std::string& outpath)
{
    std::ofstream file(outpath, std::ios::binary);

    if (!file.is_open()) {
        return E_FAIL;
    }

    auto pushMesh = [&](const void* data, size_t size)
        {
            size_t old = meshBuffer.size();
            meshBuffer.resize(old + size);
            memcpy(meshBuffer.data() + old, data, size);
        };

    //---------------------------------------------------FILEHEADER-------------------------------------------------------------------//
    MODEL_FILE_HEADER MFH;
    MFH.bHasBone = false;
    MFH.bHasAnimation = false;
    MFH.MeshCount = (uint32_t)Meshes.size();
    MFH.AnimationCount = 0;
    MFH.MaterialCount = (uint32_t)Materials.size();
    MFH.BoneCount = (uint32_t)Bones.size();
    file.write((char*)&MFH, sizeof(MFH));

    //---------------------------------------------------------MESH-------------------------------------------------------------------//
	for (auto& mesh : Meshes)
	{
		uint32_t snameLen = static_cast<uint32_t>(mesh->m_name.size());
		uint32_t svCount = static_cast<uint32_t>(mesh->m_vertices->size());
		uint32_t siCount = static_cast<uint32_t>(mesh->m_indices->size());

		uint32_t meshSize =
			sizeof(uint32_t) + snameLen +              // nameLen + name
			sizeof(uint32_t) +                         // materialIndex
			sizeof(XMFLOAT3) +                         // min
			sizeof(XMFLOAT3) +                         // max
			sizeof(uint32_t) +                         // vCount
			sizeof(uint32_t) +                         // iCount
			sizeof(VTXMESH) * svCount +                // vertices
			sizeof(uint32_t) * siCount;                // indices

		pushMesh(&meshSize, sizeof(uint32_t));

		uint32_t nameLen = static_cast<uint32_t>(mesh->m_name.size());
		pushMesh(&nameLen, sizeof(uint32_t));

		pushMesh(mesh->m_name.data(), nameLen);

		uint32_t vMaterialIndex = mesh->m_materialIndex;
		pushMesh(&vMaterialIndex, sizeof(uint32_t));

		// ------------------------------------------------------------
		// min / max 저장
		// ------------------------------------------------------------
		pushMesh(&mesh->m_min, sizeof(XMFLOAT3));
		pushMesh(&mesh->m_max, sizeof(XMFLOAT3));

		uint32_t vCount = static_cast<uint32_t>(mesh->m_vertices->size());
		pushMesh(&vCount, sizeof(uint32_t));

		uint32_t iCount = static_cast<uint32_t>(mesh->m_indices->size());
		pushMesh(&iCount, sizeof(uint32_t));

		pushMesh(mesh->m_vertices->data(), sizeof(VTXMESH) * vCount);
		pushMesh(mesh->m_indices->data(), sizeof(uint32_t) * iCount);
	}

    ChunkHeader chMesh;
    chMesh.type = ChunkType::CHUNK_MESH;
    chMesh.size = (uint32_t)meshBuffer.size();

    file.write((char*)&chMesh, sizeof(chMesh));
    file.write(meshBuffer.data(), meshBuffer.size());

    //--------------------------------------------------------Material-------------------------------------------------------------------//
    auto pushMaterial = [&](const void* data, size_t size)
        {
            size_t old = materialBuffer.size();
            materialBuffer.resize(old + size);
            memcpy(materialBuffer.data() + old, data, size);
        };

    for (auto& mat : Materials)
    {
        uint32_t materialSize = 0;

        // materialNum
        materialSize += sizeof(uint32_t);

        // textureTypeCount
        materialSize += sizeof(uint32_t);

        for (auto& texs : mat->m_textures)
        {
            // textureCount
            materialSize += sizeof(uint32_t);

            for (auto& tex : texs)
            {
                materialSize += sizeof(uint32_t); // m_textureType

                materialSize += sizeof(uint32_t); // File length
                materialSize += (uint32_t)tex.File.size();

                materialSize += sizeof(uint32_t); // Ext length
                materialSize += (uint32_t)tex.Ext.size();
            }
        }

        // Material 크기 먼저 기록
        pushMaterial(&materialSize, sizeof(uint32_t));


        uint32_t materialNum = mat->m_materialNum;
        pushMaterial(&materialNum, sizeof(uint32_t));

        uint32_t textureTypeCount = (uint32_t)mat->m_textures.size();
        pushMaterial(&textureTypeCount, sizeof(uint32_t));

        for (auto& texs : mat->m_textures)
        {
            uint32_t textureCount = (uint32_t)texs.size();
            pushMaterial(&textureCount, sizeof(uint32_t));

            for (auto& tex : texs)
            {
                pushMaterial(&tex.m_textureType, sizeof(uint32_t));

                uint32_t len;

                len = (uint32_t)tex.File.size();
                pushMaterial(&len, sizeof(uint32_t));
                pushMaterial(tex.File.c_str(), len);

                len = (uint32_t)tex.Ext.size();
                pushMaterial(&len, sizeof(uint32_t));
                pushMaterial(tex.Ext.c_str(), len);
            }
        }
    }

    ChunkHeader chMaterial;
    chMaterial.type = ChunkType::CHUNK_MATERIAL;
    chMaterial.size = (uint32_t)materialBuffer.size();

    file.write((char*)&chMaterial, sizeof(chMaterial));
    file.write(materialBuffer.data(), materialBuffer.size());

    file.close();
    return S_OK;
}
HRESULT CImporter::ExportSkeletal(const std::string& outpath) {
    std::ofstream file(outpath, std::ios::binary);

    if (!file.is_open()) {
        return E_FAIL;
    }

    auto pushBone = [&](const void* data, size_t size)
        {
            size_t old = boneBuffer.size();
            boneBuffer.resize(old + size);
            memcpy(boneBuffer.data() + old, data, size);
        };

    auto pushMesh = [&](const void* data, size_t size)
        {
            size_t old = meshBuffer.size();
            meshBuffer.resize(old + size);
            memcpy(meshBuffer.data() + old, data, size);
        };

    auto pushMaterial = [&](const void* data, size_t size)
        {
            size_t old = materialBuffer.size();
            materialBuffer.resize(old + size);
            memcpy(materialBuffer.data() + old, data, size);
        };


    //---------------------------------------------------FILEHEADER-------------------------------------------------------------------//
    MODEL_FILE_HEADER MFH;
    MFH.bHasBone = true;
    MFH.bHasAnimation = false;
    MFH.MeshCount = (uint32_t)Meshes.size();
    MFH.AnimationCount = 0;
    MFH.MaterialCount = (uint32_t)Materials.size();
    MFH.BoneCount = (uint32_t)Bones.size();
    file.write((char*)&MFH, sizeof(MFH));

    //-------------------------------------------------BONE-----------------------------------------------------------------------------//

    for (auto& bone : Bones)
    {
        uint32_t len = (uint32_t)bone->Bone.m_name.size();

        uint32_t boneSize =
            sizeof(uint32_t) +    // len
            len +                 // name
            sizeof(XMFLOAT4X4) +  // transform
            sizeof(uint32_t);     // parent index

        pushBone(&boneSize, sizeof(uint32_t));

        pushBone(&len, sizeof(uint32_t));
        pushBone(bone->Bone.m_name.c_str(), len);
        pushBone(&bone->Bone.m_TransformationMatrix, sizeof(XMFLOAT4X4));
        pushBone(&bone->Bone.m_patrentBoneIndex, sizeof(uint32_t));
    }

    ChunkHeader chBone;
    chBone.type = ChunkType::CHUNK_BONE;
    chBone.size = (uint32_t)boneBuffer.size();
    file.write((char*)&chBone, sizeof(chBone));
    file.write(boneBuffer.data(), boneBuffer.size());

    //---------------------------------------------------------MESH-------------------------------------------------------------------//
    for (auto& mesh : Meshes)
    {
        uint32_t vCount = (uint32_t)mesh->m_animvertices->size();
        uint32_t iCount = (uint32_t)mesh->m_indices->size();

        uint32_t BoneIndicesCount = (uint32_t)mesh->m_BoneIndices->size();
        uint32_t BoneMatricesCount = (uint32_t)mesh->m_BoneMatrices->size();
        uint32_t OffsetMatricesCount = (uint32_t)mesh->m_OffsetMatrices->size();

        uint32_t meshSize =
            sizeof(uint32_t) +                                         // MaterialIndex
            sizeof(uint32_t) +                                         // Vertex Count
            sizeof(uint32_t) +                                         // Index Count
            sizeof(VTXANIMMESH) * vCount +                             // Vertex Data
            sizeof(uint32_t) * iCount +                                // Index Data
            sizeof(uint32_t) +                                         // NumBones
            sizeof(uint32_t) +                                         // BoneIndices Count
            sizeof(uint32_t) +                                         // BoneMatrices Count
            sizeof(uint32_t) +                                         // OffsetMatrices Count
            sizeof(uint32_t) * BoneIndicesCount +                      // BoneIndices Data
            sizeof(XMFLOAT4X4) * BoneMatricesCount +                   // BoneMatrices Data
            sizeof(XMFLOAT4X4) * OffsetMatricesCount;                  // OffsetMatrices Data

        pushMesh(&meshSize, sizeof(uint32_t));

        // MaterialIndex
         pushMesh(&mesh->m_materialIndex, sizeof(uint32_t));

        // Vertex Count
        pushMesh(&vCount, sizeof(uint32_t));

        // Index Count
        pushMesh(&iCount, sizeof(uint32_t));

        // Vertex 데이터
        pushMesh(mesh->m_animvertices->data(),
            sizeof(VTXANIMMESH) * vCount);

        // Index 데이터
        pushMesh(mesh->m_indices->data(),
            sizeof(uint32_t) * iCount);

        // Mesh가 이용하는 뼈의 개수
        pushMesh(&mesh->m_iNumBones, sizeof(uint32_t));

        // BoneIndices Count
        pushMesh(&BoneIndicesCount, sizeof(uint32_t));

        // BoneMatrices Count
        pushMesh(&BoneMatricesCount, sizeof(uint32_t));

        // OffsetMatrices Count
        pushMesh(&OffsetMatricesCount, sizeof(uint32_t));

        // BoneIndices 데이터
        pushMesh(mesh->m_BoneIndices->data(),
            sizeof(uint32_t) * BoneIndicesCount);

        // BoneMatrices 데이터
        pushMesh(mesh->m_BoneMatrices->data(),
            sizeof(XMFLOAT4X4) * BoneMatricesCount);

        // OffsetMatrices 데이터
        pushMesh(mesh->m_OffsetMatrices->data(),
            sizeof(XMFLOAT4X4) * OffsetMatricesCount);
    }

    ChunkHeader chMesh;
    chMesh.type = ChunkType::CHUNK_MESH;
    chMesh.size = (uint32_t)meshBuffer.size();

    file.write((char*)&chMesh, sizeof(chMesh));
    file.write(meshBuffer.data(), meshBuffer.size());

    //--------------------------------------------------------Material-------------------------------------------------------------------//

    for (auto& mat : Materials)
    {
        uint32_t materialSize = 0;

        // materialNum
        materialSize += sizeof(uint32_t);

        // textureTypeCount
        materialSize += sizeof(uint32_t);

        for (auto& texs : mat->m_textures)
        {
            // textureCount
            materialSize += sizeof(uint32_t);

            for (auto& tex : texs)
            {
                materialSize += sizeof(uint32_t); // m_textureType

                materialSize += sizeof(uint32_t); // File length
                materialSize += (uint32_t)tex.File.size();

                materialSize += sizeof(uint32_t); // Ext length
                materialSize += (uint32_t)tex.Ext.size();
            }
        }

        // Material 크기 먼저 기록
        pushMaterial(&materialSize, sizeof(uint32_t));


        uint32_t materialNum = mat->m_materialNum;
        pushMaterial(&materialNum, sizeof(uint32_t));

        uint32_t textureTypeCount = (uint32_t)mat->m_textures.size();
        pushMaterial(&textureTypeCount, sizeof(uint32_t));

        for (auto& texs : mat->m_textures)
        {
            uint32_t textureCount = (uint32_t)texs.size();
            pushMaterial(&textureCount, sizeof(uint32_t));

            for (auto& tex : texs)
            {
                pushMaterial(&tex.m_textureType, sizeof(uint32_t));

                uint32_t len;

                len = (uint32_t)tex.File.size();
                pushMaterial(&len, sizeof(uint32_t));
                pushMaterial(tex.File.c_str(), len);

                len = (uint32_t)tex.Ext.size();
                pushMaterial(&len, sizeof(uint32_t));
                pushMaterial(tex.Ext.c_str(), len);
            }
        }
    }

    ChunkHeader chMaterial;
    chMaterial.type = ChunkType::CHUNK_MATERIAL;
    chMaterial.size = (uint32_t)materialBuffer.size();

    file.write((char*)&chMaterial, sizeof(chMaterial));
    file.write(materialBuffer.data(), materialBuffer.size());

    file.close();
    return S_OK;
}
HRESULT CImporter::ExportAnimation(const std::string& outpath)
{
    auto pushAnim = [&](const void* data, size_t size)
        {
            size_t old = animBuffer.size();
            animBuffer.resize(old + size);
            memcpy(animBuffer.data() + old, data, size);
        };
    //---------------------------------------------------Animaiton-------------------------------------------------------------------//

    for (auto& Animation : Animations)
    {
        animBuffer.clear();


        std::string animName = Animation->m_AnimationData.sAnimName;

        const std::string invalidChars = R"(< >:"/\|?*)";

        std::replace_if(
            animName.begin(),
            animName.end(),
            [](char c)
            {
                switch (c)
                {
                case '<':
                case '>':
                case ':':
                case '"':
                case '/':
                case '\\':
                case '|':
                case '?':
                case '*':
                    return true;
                default:
                    return false;
                }
            },
            '_'
        );


        std::filesystem::path animPath = fileParentName+ "/" + ( "AN_" + animName + ".bin");

        std::ofstream file(animPath, std::ios::binary);

        if (!file.is_open())
            continue;

        MODEL_FILE_HEADER MFH{};
        MFH.bHasBone = false;
        MFH.bHasAnimation = true;
        MFH.MeshCount = 0;
        MFH.AnimationCount = 1;
        MFH.MaterialCount = 0;
        MFH.BoneCount = 0;

        file.write((char*)&MFH, sizeof(MFH));


        float Duration = Animation->m_AnimationData.AnimationDuration;

        float TickPerSecond = Animation->m_AnimationData.AnimtaionTickPerSecond;

        pushAnim(&Duration, sizeof(float));
        pushAnim(&TickPerSecond, sizeof(float));

        uint32_t ChannelCount = Animation->m_AnimationData.ChannelCount;

        pushAnim(&ChannelCount, sizeof(uint32_t));
        


        for (uint32_t i = 0; i < ChannelCount; ++i)
        {



            int32_t BoneIndex = (*Animation->m_AnimationData.Channels)[i].BoneIndex;

            uint32_t KeyFrameCount = (*Animation->m_AnimationData.Channels)[i].KeyFrameCount;


            uint32_t ChannelSize =
                sizeof(int32_t) +                    // BoneIndex
                sizeof(uint32_t) +                   // KeyFrameCount
                KeyFrameCount *
                (
                    sizeof(XMFLOAT3) +               // Scale
                    sizeof(XMFLOAT4) +               // Rotation
                    sizeof(XMFLOAT3) +               // Translation
                    sizeof(float)                    // TrackPosition
                    );

            pushAnim(&ChannelSize, sizeof(uint32_t));



            pushAnim(&BoneIndex, sizeof(int32_t));
            pushAnim(&KeyFrameCount, sizeof(uint32_t));

            for (uint32_t j = 0; j < KeyFrameCount; ++j)
            {
                auto& KeyFrame = (*(*Animation->m_AnimationData.Channels)[i].KeyFrames)[j];

                pushAnim(&KeyFrame.vScale, sizeof(XMFLOAT3));
                pushAnim(&KeyFrame.vRotation, sizeof(XMFLOAT4));
                pushAnim(&KeyFrame.vTranslation, sizeof(XMFLOAT3));
                pushAnim(&KeyFrame.fTrackPosition, sizeof(float));
            }
        }

        ChunkHeader chAnim;
        chAnim.type = ChunkType::CHUNK_ANIM;
        chAnim.size = (uint32_t)animBuffer.size();

        file.write((char*)&chAnim, sizeof(chAnim));
        file.write(animBuffer.data(), animBuffer.size());

        file.close();
    }

    return S_OK;
}
    
HRESULT CImporter::Ready_Bones(const aiNode* pAINode, int32_t iParentBoneIndex) {

    auto    pBone = std::make_shared<CBone>();

    pBone->Bone.m_name= pAINode->mName.C_Str();

    memcpy(&(pBone->Bone.m_TransformationMatrix), &pAINode->mTransformation, sizeof(XMFLOAT4X4));

    pBone->Bone.m_patrentBoneIndex = iParentBoneIndex;

    Bones.push_back(pBone);

    int32_t iParentIndex = (int32_t)Bones.size() - 1;

    for (uint32_t i = 0; i < pAINode->mNumChildren; ++i)
    {
        Ready_Bones(pAINode->mChildren[i], iParentIndex);
    }

    return S_OK;
}

HRESULT CImporter::Ready_Material(const aiScene* scene) {
    uint32_t NumMaterials = scene->mNumMaterials;
    Materials.reserve(NumMaterials);

    for (uint32_t i = 0; i < NumMaterials; i++)
    {
        Load_Material(scene->mMaterials[i], i);
    }

    return S_OK;
}

void CImporter::Load_Material(aiMaterial* material, uint32_t materialNum)
{
    std::shared_ptr<CMaterial> fbxmaterial = std::make_shared<CMaterial>();
    fbxmaterial->m_materialNum = materialNum;
    for (size_t i = 0; i < AI_TEXTURE_TYPE_MAX; i++)
    {
        uint32_t		iNumTextures = material->GetTextureCount(static_cast<aiTextureType>(i));

        if (iNumTextures == 0) {
            continue;
        }

        std::vector<TEXTUREINFO> textureDummy;
        textureDummy.resize(iNumTextures);

        for (uint32_t j = 0; j < iNumTextures; j++)
        {

            char	szFileName[MAX_PATH] = { };
            char	szExt[MAX_PATH] = { };

            aiString		strTexturePath = {};

            material->GetTexture(static_cast<aiTextureType>(i), j, &strTexturePath);

            _splitpath_s(strTexturePath.C_Str(), nullptr, 0, nullptr, 0, szFileName, MAX_PATH, szExt, MAX_PATH);

            textureDummy[j].m_textureType = (uint32_t)i;
            textureDummy[j].m_textureNum = j;
            textureDummy[j].File = szFileName;
            textureDummy[j].Ext = szExt;
        }


        fbxmaterial->m_textures.push_back(textureDummy);
    }


    Materials.emplace_back(fbxmaterial);
}

HRESULT CImporter::Ready_Animation(const aiScene* scene)
{
    Animations.reserve(scene->mNumAnimations);
    for (size_t i = 0; i < scene->mNumAnimations; i++)
    {

        Animations.emplace_back(make_shared<CAnimation>());

        Load_Animaion(i, scene->mAnimations[i]);
    }

    return S_OK;
}

HRESULT CImporter::Load_Animaion(uint32_t iAnimaionCount, const aiAnimation* pAIAnimation)
{
    Animations[iAnimaionCount]->m_AnimationData.sAnimName = pAIAnimation->mName.C_Str();

    Animations[iAnimaionCount]->m_AnimationData.AnimationDuration = pAIAnimation->mDuration;
    Animations[iAnimaionCount]->m_AnimationData.AnimtaionTickPerSecond = pAIAnimation->mTicksPerSecond;
    Animations[iAnimaionCount]->m_AnimationData.ChannelCount = pAIAnimation->mNumChannels;

    Animations[iAnimaionCount]->m_AnimationData.Channels = make_shared<vector<CHANNELDATA>>();
    Animations[iAnimaionCount]->m_AnimationData.Channels->reserve(pAIAnimation->mNumChannels);


    for (size_t i = 0; i < pAIAnimation->mNumChannels; i++)
    {
        CHANNELDATA ChanelData;

        Load_Channel(ChanelData, pAIAnimation->mChannels[i]);
        Animations[iAnimaionCount]->m_AnimationData.Channels->emplace_back(ChanelData);
    }

    return S_OK;
}

HRESULT CImporter::Load_Channel(CHANNELDATA& ChannelData, const aiNodeAnim* pAIChannel)
{
    ChannelData.BoneIndex = Get_BoneIndex(pAIChannel->mNodeName.C_Str());
    if (-1 == ChannelData.BoneIndex) {
        return S_OK;
    }


    ChannelData.KeyFrameCount = max(pAIChannel->mNumScalingKeys, pAIChannel->mNumRotationKeys);
    ChannelData.KeyFrameCount = max(ChannelData.KeyFrameCount, pAIChannel->mNumPositionKeys);

    XMFLOAT3     vScale = {};
    XMFLOAT4     vRotation = {};
    XMFLOAT3     vTranslation = {};

    ChannelData.KeyFrames = make_shared<vector< KEYFRAME>>();

    ChannelData.KeyFrames->reserve(ChannelData.KeyFrameCount);

    for (size_t i = 0; i < ChannelData.KeyFrameCount; i++)
    {
        KEYFRAME            KeyFrame = {};

        if (i < pAIChannel->mNumScalingKeys) // 만약에 더 큰 값이 들어올떄 그 전의 값으로 마지막껄 채워준다.
        {
            memcpy(&vScale, &pAIChannel->mScalingKeys[i].mValue, sizeof vScale);
            KeyFrame.fTrackPosition = pAIChannel->mScalingKeys[i].mTime;
        }

        if (i < pAIChannel->mNumRotationKeys)
        {
            // memcpy(&vRotation, &pAIChannel->mRotationKeys[i].mValue, sizeof vRotation);
            vRotation.x = pAIChannel->mRotationKeys[i].mValue.x;
            vRotation.y = pAIChannel->mRotationKeys[i].mValue.y;
            vRotation.z = pAIChannel->mRotationKeys[i].mValue.z;
            vRotation.w = pAIChannel->mRotationKeys[i].mValue.w;
            KeyFrame.fTrackPosition = pAIChannel->mRotationKeys[i].mTime;
        }

        if (i < pAIChannel->mNumPositionKeys)
        {
            memcpy(&vTranslation, &pAIChannel->mPositionKeys[i].mValue, sizeof vTranslation);
            KeyFrame.fTrackPosition = pAIChannel->mPositionKeys[i].mTime;
        }

        KeyFrame.vScale = vScale;
        KeyFrame.vRotation = vRotation;
        KeyFrame.vTranslation = vTranslation;

        ChannelData.KeyFrames->emplace_back(KeyFrame);
    }


    return S_OK;

}

HRESULT CImporter::Ready_Mesh(const aiScene* scene, bool _bHasBone)
{
    aiNode* pNode = scene->mRootNode;
    Meshes.reserve(scene->mNumMeshes);
    if (_bHasBone)
        ProcessAnimNode(pNode, scene);
    else
        ProcessNonAnimNode(pNode, scene);

    return S_OK;
}

void CImporter::ProcessNonAnimMesh(aiMesh* mesh, const aiScene* scene)
{
    

    std::shared_ptr<std::vector<VTXMESH>> vertices = std::make_shared<std::vector<VTXMESH>>();
    std::shared_ptr<std::vector<uint32_t>> indices = std::make_shared<std::vector<uint32_t>>();


    vertices->reserve(mesh->mNumVertices);

    uint32_t indexCount = 0;
    for (UINT i = 0; i < mesh->mNumFaces; ++i)
    {
        indexCount += mesh->mFaces[i].mNumIndices;
    }
    indices->reserve(indexCount);

    for (UINT i = 0; i < mesh->mNumVertices; ++i)
    {
        VTXMESH v{};

        // Position
        v.vPosition = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        };

        // Normal
        if (mesh->HasNormals())
        {
            v.vNormal = {
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            };
        }

        // UV
        if (mesh->HasTextureCoords(0))
        {
            v.vTexcoord = {
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            };
        }

        // Tangent & Binormal
        if (mesh->HasTangentsAndBitangents())
        {
            v.vTangent = {
                mesh->mTangents[i].x,
                mesh->mTangents[i].y,
                mesh->mTangents[i].z
            };

            v.vBinormal = {
                mesh->mBitangents[i].x,
                mesh->mBitangents[i].y,
                mesh->mBitangents[i].z
            };
        }

        vertices->emplace_back(v);
    }

    // Index
    for (UINT i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];

        for (UINT j = 0; j < face.mNumIndices; ++j)
        {
            indices->emplace_back(face.mIndices[j]);
        }
    }


    std::string _name;


    uint32_t _materialIndex;
    XMFLOAT3 _min = { FLT_MAX, FLT_MAX, FLT_MAX };
    XMFLOAT3 _max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };


    if (mesh->mName.length > 0)
        _name = mesh->mName.C_Str();
    else
        _name = "Mesh_" + std::to_string(++m_index);

    _materialIndex = mesh->mMaterialIndex;

    // boundingBox
    for (UINT i = 0; i < mesh->mNumVertices; ++i)
    {
        const aiVector3D& pos = mesh->mVertices[i];

        _min.x = min(_min.x, pos.x);
        _min.y = min(_min.y, pos.y);
        _min.z = min(_min.z, pos.z);

        _max.x = max(_max.x, pos.x);
        _max.y = max(_max.y, pos.y);
        _max.z = max(_max.z, pos.z);
    }

    std::shared_ptr<CMesh> fbxmesh = std::make_shared<CMesh>();
    
    fbxmesh->m_name = _name;
    fbxmesh->m_materialIndex = _materialIndex;
    fbxmesh->m_max = _max;
    fbxmesh->m_min = _min;
    fbxmesh->m_vertices = vertices;
    fbxmesh->m_indices = indices;



    Meshes.emplace_back(fbxmesh);

    mesh->mMaterialIndex;


}

void CImporter::ProcessNonAnimNode(aiNode* node, const aiScene* scene)
{
    for (UINT i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessNonAnimMesh(mesh, scene);
    }

    for (UINT i = 0; i < node->mNumChildren; ++i)
        ProcessNonAnimNode(node->mChildren[i], scene);
}

void CImporter::ProcessAnimMesh(aiMesh* mesh, const aiScene* scene, std::string name)
{

    std::string _name;

    _name = name;


    uint32_t _materialIndex;
    XMFLOAT3 _min = { FLT_MAX, FLT_MAX, FLT_MAX };
    XMFLOAT3 _max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    uint32_t m_iNumBones = mesh->mNumBones;
    std::shared_ptr<std::vector<uint32_t>> m_Boneindices = std::make_shared<std::vector<uint32_t>>();
    std::shared_ptr<std::vector<XMFLOAT4X4>> m_BoneMatrices = std::make_shared<std::vector<XMFLOAT4X4>>();
    std::shared_ptr<std::vector<XMFLOAT4X4>> m_OffsetMatrices = std::make_shared<std::vector<XMFLOAT4X4>>();
    std::shared_ptr<std::vector<VTXANIMMESH>> vertices = std::make_shared<std::vector<VTXANIMMESH>>();
    std::shared_ptr<std::vector<uint32_t>> indices = std::make_shared<std::vector<uint32_t>>();
    if (m_iNumBones > 0)
    {
        m_Boneindices->resize(m_iNumBones);
        m_BoneMatrices->resize(m_iNumBones);
        m_OffsetMatrices->resize(m_iNumBones);
    }
    std::shared_ptr<CMesh> fbxmesh = std::make_shared<CMesh>();


    vertices->reserve(mesh->mNumVertices);

    uint32_t indexCount = 0;
    for (UINT i = 0; i < mesh->mNumFaces; ++i)
    {
        indexCount += mesh->mFaces[i].mNumIndices;
    }
    indices->reserve(indexCount);


    for (UINT i = 0; i < mesh->mNumVertices; ++i)
    {
        VTXANIMMESH v{};

        // Position
        v.vPosition = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        };

        // Normal
        if (mesh->HasNormals())
        {
            v.vNormal = {
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            };
        }

        // UV
        if (mesh->HasTextureCoords(0))
        {
            v.vTexcoord = {
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            };
        }

        // Tangent & Binormal
        if (mesh->HasTangentsAndBitangents())
        {
            v.vTangent = {
                mesh->mTangents[i].x,
                mesh->mTangents[i].y,
                mesh->mTangents[i].z
            };

            v.vBinormal = {
                mesh->mBitangents[i].x,
                mesh->mBitangents[i].y,
                mesh->mBitangents[i].z
            };
        }



        vertices->emplace_back(v);
    }



    if (0 == m_iNumBones)
    {
        m_iNumBones = 1;

        int32_t iBoneIndex = Get_BoneIndex(_name.data());

        if (-1 == iBoneIndex)
            return;

        XMFLOAT4X4 OffsetMatrix;
        XMStoreFloat4x4(&OffsetMatrix, XMMatrixIdentity());

        m_Boneindices->resize(1);
        m_BoneMatrices->resize(1);
        m_OffsetMatrices->resize(1);

        (*m_Boneindices)[0] = iBoneIndex;
        XMStoreFloat4x4(&(*m_BoneMatrices)[0], XMMatrixIdentity());
        (*m_OffsetMatrices)[0] = OffsetMatrix;
    }

    else {
        for (size_t i = 0; i < m_iNumBones; i++)
        {
            aiBone* pAIBone = mesh->mBones[i];

            XMFLOAT4X4   OffsetMatrix;
            memcpy(&OffsetMatrix, &pAIBone->mOffsetMatrix, sizeof(XMFLOAT4X4));

            XMStoreFloat4x4(&OffsetMatrix, XMMatrixTranspose(XMLoadFloat4x4(&OffsetMatrix)));
            (*m_OffsetMatrices)[i] = (OffsetMatrix);


            int32_t    iBoneIndex = Get_BoneIndex(pAIBone->mName.C_Str());
            if (-1 == iBoneIndex)
                return;

            (*m_Boneindices)[i] = (iBoneIndex);

            for (size_t j = 0; j < pAIBone->mNumWeights; j++)
            {
                if (0 == (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendWeights.x)
                {
                    (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendIndices.x = (uint32_t)i;
                    (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendWeights.x = pAIBone->mWeights[j].mWeight;
                }

                else if (0 == (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendWeights.y)
                {
                    (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendIndices.y = (uint32_t)i;
                    (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendWeights.y = pAIBone->mWeights[j].mWeight;
                }

                else if (0 == (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendWeights.z)
                {
                    (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendIndices.z = (uint32_t)i;
                    (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendWeights.z = pAIBone->mWeights[j].mWeight;
                }

                else if (0 == (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendWeights.w)
                {
                    (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendIndices.w = (uint32_t)i;
                    (*vertices)[pAIBone->mWeights[j].mVertexId].vBlendWeights.w = pAIBone->mWeights[j].mWeight;
                }
            }
        }
    }
    // Index
    for (UINT i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];

        for (UINT j = 0; j < face.mNumIndices; ++j)
        {
            indices->emplace_back(face.mIndices[j]);
        }
    }


    _materialIndex = mesh->mMaterialIndex;

    // boundingBox
    for (UINT i = 0; i < mesh->mNumVertices; ++i)
    {
        const aiVector3D& pos = mesh->mVertices[i];

        _min.x = min(_min.x, pos.x);
        _min.y = min(_min.y, pos.y);
        _min.z = min(_min.z, pos.z);

        _max.x = max(_max.x, pos.x);
        _max.y = max(_max.y, pos.y);
        _max.z = max(_max.z, pos.z);
    }


    fbxmesh->m_name = _name;
    fbxmesh->m_materialIndex = _materialIndex;
    fbxmesh->m_max = _max;
    fbxmesh->m_min = _min;
    fbxmesh->m_animvertices = vertices;
    fbxmesh->m_indices = indices;
    fbxmesh->m_iNumBones = m_iNumBones;
    fbxmesh->m_BoneIndices = m_Boneindices;
    fbxmesh->m_BoneMatrices = m_BoneMatrices;
    fbxmesh->m_OffsetMatrices = m_OffsetMatrices;


    Meshes.emplace_back(fbxmesh);


}

void CImporter::ProcessAnimNode(aiNode* node, const aiScene* scene)
{
    for (UINT i = 0; i < node->mNumMeshes; ++i)
    {
        std::string name = node->mName.C_Str();
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessAnimMesh(mesh, scene, name);
    }

    for (UINT i = 0; i < node->mNumChildren; ++i)
        ProcessAnimNode(node->mChildren[i], scene);
}

std::string CImporter::ToLowerFileName(std::string name) const
{
	std::transform(
		name.begin(),
		name.end(),
		name.begin(),
		[](unsigned char c)
		{
			return static_cast<char>(std::tolower(c));
		}
	);

	return name;
}

bool CImporter::HasExtractedModelData(
	const std::filesystem::path& modelDir,
	const std::string& modelName
) const
{
	if (!std::filesystem::exists(modelDir))
		return false;

	std::filesystem::path smPath = modelDir / ("SM_" + modelName + ".bin");
	std::filesystem::path skPath = modelDir / ("SK_" + modelName + ".bin");

	if (std::filesystem::exists(smPath))
		return true;

	if (std::filesystem::exists(skPath))
		return true;

	return false;
}

std::unordered_set<std::string> CImporter::LoadMapFBXNamesFromJsonFolder(
	const std::string& strJsonFolder
)
{
	std::unordered_set<std::string> fbxNames;

	if (!std::filesystem::exists(strJsonFolder))
		return fbxNames;

	for (const auto& entry : std::filesystem::directory_iterator(strJsonFolder))
	{
		if (!entry.is_regular_file())
			continue;

		const std::filesystem::path& jsonPath = entry.path();

		if (_stricmp(jsonPath.extension().string().c_str(), ".json") != 0)
			continue;

		std::ifstream file(jsonPath);
		if (!file.is_open())
			continue;

		try
		{
			nlohmann::json j;
			file >> j;

			if (!j.contains("fbx"))
				continue;

			// 문자열이든 배열이든 리스트로 통일해서 처리
			std::vector<std::string> fbxList;

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
			else
			{
				continue; // 문자열도 배열도 아니면 스킵
			}

			for (auto& fbxName : fbxList)
			{
				if (fbxName.empty())
					continue;

				// 혹시 경로까지 들어와도 파일 이름만 비교
				fbxName = std::filesystem::path(fbxName).filename().string();

				// 대소문자 무시 비교용
				fbxNames.insert(ToLowerFileName(fbxName));
			}
		}
		catch (...)
		{
			continue;
		}
	}

	return fbxNames;

}

HRESULT CImporter::ImportFBXFolder_ForMapJson(
	const std::string& strLevelName,
	const std::string& strSourceFolder,
	const std::string& strJsonFolder
)
{
	UNREFERENCED_PARAMETER(strLevelName);

	if (!std::filesystem::exists(strSourceFolder))
		return E_FAIL;

	if (!std::filesystem::exists(strJsonFolder))
		return E_FAIL;

	std::unordered_set<std::string> targetFBXNames =
		LoadMapFBXNamesFromJsonFolder(strJsonFolder);

	if (targetFBXNames.empty())
		return E_FAIL;

	std::filesystem::path sourcePath(strSourceFolder);

	std::string category = sourcePath.filename().string();

	std::string rootPath = strSourceFolder;

	size_t pos = rootPath.find("OriginData");

	if (pos != std::string::npos)
	{
		rootPath = rootPath.substr(0, pos);
	}
	else
	{
		rootPath = sourcePath.parent_path().parent_path().string() + "/";
	}

	// 핵심:
	// LevelAnimEditor 같은 중간 폴더를 넣지 않고,
	// Models/Static 또는 Models/Skeletal 바로 밑으로 보냄
	std::filesystem::path basePath = std::filesystem::path(rootPath) / category;

	for (const auto& entry : std::filesystem::recursive_directory_iterator(strSourceFolder))
	{
		if (!entry.is_regular_file())
			continue;

		const std::filesystem::path& path = entry.path();

		if (_stricmp(path.extension().string().c_str(), ".fbx") != 0)
			continue;

		std::string fbxFileName = path.filename().string();
		std::string lowerFBXName = ToLowerFileName(fbxFileName);

		if (targetFBXNames.find(lowerFBXName) == targetFBXNames.end())
			continue;

		std::string inputPath = path.string();
		std::string modelName = path.stem().string();

		std::filesystem::path modelDir;

		if (_stricmp(category.c_str(), "Static") == 0)
		{
			modelDir = basePath;
		}
		else
		{
			modelDir = basePath / modelName;
		}

		// ------------------------------------------------------------
		// bin 추출 스킵 전에 texture 폴더 생성
		// ------------------------------------------------------------
		std::filesystem::path textureDir;
		std::filesystem::path originTextureDir =
			MakeTextureOutputDir(std::filesystem::path(rootPath) / "OriginData" / category);

		if (_stricmp(category.c_str(), "Static") == 0)
		{
			textureDir = MakeTextureOutputDir(basePath) / modelName;
			std::filesystem::create_directories(textureDir);
			std::filesystem::create_directories(modelDir);
		}
		else
		{
			textureDir = MakeTextureOutputDir(modelDir);
			std::filesystem::create_directories(textureDir);
			std::filesystem::create_directories(modelDir);
		}



	
		if (HasExtractedModelData(modelDir, modelName))
			continue;




		if (FAILED(AssimpFBX(inputPath)))
		{
			Clear();
			continue;
		}
		
		// ------------------------------------------------------------
		// Static bin 저장용 텍스처 복사
		//
		// Textures/OriginData/Static/*.png
		// -> Textures/Static/모델이름/*.png
		// ------------------------------------------------------------
		std::filesystem::path outputPath;

		if (_stricmp(category.c_str(), "Static") == 0)
		{
			CopyUsedTextureFilesToFolder(originTextureDir, textureDir);
			outputPath = modelDir / (modelName + ".bin");
		}
		else {
			CopyUsedTextureFilesToFolder(originTextureDir, textureDir);
			
			outputPath = modelDir / (modelName + ".bin");

		}


		if (FAILED(ExportFBX(outputPath.string())))
		{
			Clear();
			continue;
		}

		Clear();
	}

	return S_OK;
}
int32_t CImporter::Get_BoneIndex(const char* pBoneName)
{
    int32_t iBoneIndex = { 0 };
    auto    iter = find_if(Bones.begin(), Bones.end(), [&](std::shared_ptr<CBone> pBone)->bool
        {
            if (true == pBone->Compare_Name(pBoneName))
                return true;

            ++iBoneIndex;

            return false;
        });

    if (iter == Bones.end())
        return -1;

    return iBoneIndex;
}

void CImporter::CopyPngFilesToFolder(
	const std::filesystem::path& srcDir,
	const std::filesystem::path& dstDir
) const
{
	if (!std::filesystem::exists(srcDir))
		return;

	if (!std::filesystem::is_directory(srcDir))
		return;

	std::filesystem::create_directories(dstDir);

	for (const auto& entry : std::filesystem::directory_iterator(srcDir))
	{
		if (!entry.is_regular_file())
			continue;

		const std::filesystem::path& srcPath = entry.path();

		std::string ext = srcPath.extension().string();

		if (_stricmp(ext.c_str(), ".png") != 0)
			continue;

		std::filesystem::path dstPath = dstDir / srcPath.filename();

		std::error_code ec;

		std::filesystem::copy_file(
			srcPath,
			dstPath,
			std::filesystem::copy_options::overwrite_existing,
			ec
		);
	}
}

void CImporter::CopyUsedTextureFilesToFolder(
	const std::filesystem::path& srcDir,
	const std::filesystem::path& dstDir
) const
{
	if (!std::filesystem::exists(srcDir))
		return;

	if (!std::filesystem::is_directory(srcDir))
		return;

	std::filesystem::create_directories(dstDir);

	for (const auto& mat : Materials)
	{
		if (mat == nullptr)
			continue;

		for (const auto& texGroup : mat->m_textures)
		{
			for (const auto& tex : texGroup)
			{
				if (tex.File.empty())
					continue;

				// 기본은 material에 저장된 확장자 사용
				std::filesystem::path srcPath =
					srcDir / (tex.File + tex.Ext);

				// 근데 png만 원본 폴더에 모아뒀다면 png 우선 탐색
				std::filesystem::path srcPngPath =
					srcDir / (tex.File + ".png");

				if (std::filesystem::exists(srcPngPath))
				{
					srcPath = srcPngPath;
				}

				if (!std::filesystem::exists(srcPath))
					continue;

				std::filesystem::path dstPath =
					dstDir / srcPath.filename();

				std::error_code ec;

				std::filesystem::copy_file(
					srcPath,
					dstPath,
					std::filesystem::copy_options::overwrite_existing,
					ec
				);
			}
		}
	}
}

void CImporter::Clear() {
    Bones.clear();
    Meshes.clear();
    Materials.clear();
    meshBuffer.clear();
    boneBuffer.clear();
    materialBuffer.clear();
    Animations.clear();

    m_bHasAnimation = false;
    m_bHasBone = false;

}
std::filesystem::path CImporter::MakeTextureOutputDir(
	const std::filesystem::path& modelOutputDir
) const
{
	std::filesystem::path result;

	bool replaced = false;

	for (const auto& part : modelOutputDir)
	{
		std::string token = part.string();

		if (!replaced && token == "Models")
		{
			result /= "Textures";
			replaced = true;
		}
		else
		{
			result /= part;
		}
	}

	return result;
}
