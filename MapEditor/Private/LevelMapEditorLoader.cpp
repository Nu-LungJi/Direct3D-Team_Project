#include "pch.h"
#include "LevelMapEditorLoader.h"

#include "GameInstance.h"
#include "Resources.h"
#include "Terrain.h"

NS_USING(Client)

namespace
{
	std::string MakeStaticModelResourceTag(const std::filesystem::path& rootPath, const std::filesystem::path& binPath)
	{
		std::filesystem::path relativePath = binPath.lexically_relative(rootPath);
		if (relativePath.empty())
		{
			relativePath = binPath.filename();
		}

		relativePath.replace_extension();

		std::string resourceTag = relativePath.string();
		for (char& ch : resourceTag)
		{
			const unsigned char value = static_cast<unsigned char>(ch);
			if (!std::isalnum(value))
			{
				ch = '_';
			}
		}

		return resourceTag;
	}

	bool LoadLevelAnimEditorStaticModels()
	{
		const std::filesystem::path staticModelDir = /*E::PATH_MINSOO_FBX;*/ E::PATH_MAPEDITOR_STATIC_MODEL_DIR;
		if (!std::filesystem::exists(staticModelDir))
		{
			return false;
		}

		for (const auto& entry : std::filesystem::recursive_directory_iterator(staticModelDir))
		{
			if (!entry.is_regular_file() || _stricmp(entry.path().extension().string().c_str(), ".bin") != 0)
			{
				continue;
			}

			const std::string resourceTag = MakeStaticModelResourceTag(staticModelDir, entry.path());
			auto res = E::CGameInstance::Get().AddResourceT<E::CResStaticModel>(
				E::TAG_RES_GRP_MAPEDITOR_STATIC_MODEL,
				resourceTag,
				E::CResStaticModel::Create(entry.path().string()));

			if (!res)
			{
				return false;
			}

			E::CResStaticModel::DESC desc{};
			desc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f);

			if (FAILED(res->Load(desc)))
			{
				return false;
			}
		}

		return true;
	}
}


std::future<bool> CLevelMapEditorLoader::Load()
{
	if (auto res = CGameInstance::Get().AddResource("MAPEDITOR_TERRAIN_TILE", "Tile0", CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds")))
	{
		if (FAILED(res->Load()))
		{
			MSG_BOX("Terrain Tile Png Load Failed");
		}
	}

	const std::filesystem::path staticModelDir = /*E::PATH_MINSOO_FBX;*/ E::PATH_MAPEDITOR_STATIC_MODEL_DIR;
	if (!std::filesystem::exists(staticModelDir))
	{
		MSG_BOX("NO_STATIC_MODEL_DIR");
	}

	std::future<bool> result;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(staticModelDir))
	{
		if (!entry.is_regular_file() || _stricmp(entry.path().extension().string().c_str(), ".bin") != 0)
		{
			continue;
		}

		result = E::CGameInstance::Get().WorkerEnqueueWithFuture("Loading_MapFast", [=]()
			{
				const std::string resourceTag = MakeStaticModelResourceTag(staticModelDir, entry.path());
				auto res = E::CGameInstance::Get().AddResourceT<E::CResStaticModel>(
					E::TAG_RES_GRP_MAPEDITOR_STATIC_MODEL,
					resourceTag,
					E::CResStaticModel::Create(entry.path().string()));

				if (!res)
				{
					return false;
				}

				E::CResStaticModel::DESC desc{};
				desc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixRotationAxis({ 1.f,0.f,0.f }, XMConvertToRadians(90.f));

				if (FAILED(res->Load(desc)))
				{
					return false;
				}
				return true;
			}
		);
	}

	const std::filesystem::path terrainTileDir = R"(.\Resources\SampleClient\Textures\Terrain)";
	if (!std::filesystem::exists(terrainTileDir))
	{
		MSG_BOX("NO_TERRAIN_TILE_DIR");
	}
	for (const auto& entry : std::filesystem::recursive_directory_iterator(terrainTileDir))
	{
		if (!entry.is_regular_file())
		{
			continue;
		}
		const std::string extension = entry.path().extension().string();
		if ((_stricmp(extension.c_str(), ".dds") != 0 &&
			_stricmp(extension.c_str(), ".png") != 0 &&
			_stricmp(extension.c_str(), ".jpg") != 0 &&
			_stricmp(extension.c_str(), ".jpeg") != 0 &&
			_stricmp(extension.c_str(), ".tga") != 0) ||
			_stricmp(entry.path().stem().string().c_str(), "Height") == 0)
			continue;

		result = E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_MAPEDITOR_TERRAIN_TILE", [=]()
			{
				const std::string resourceTag = MakeStaticModelResourceTag(terrainTileDir, entry.path());
				auto res = E::CGameInstance::Get().AddResourceT<E::CResTexture2D>(
					"MAPEDITOR_TERRAIN_TILE",
					resourceTag,
					E::CResTexture2D::Create(entry.path().string()));

				if (!res)
					return false;

				if (FAILED(res->Load()))
				{
					MSG_BOX("Terrain Tile Png Load Failed");
					return false;
				}

				return true;
			}
		);
	}

	result = E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_MAPEDITOR", []()
	{
		// 터레인
		if (FAILED(E::CGameInstance::Get().AddPrototype("MAPEDITOR", "Prototype_GameObject_Terrain", E::CTerrain::Create())))
		{
			return false;
		}

		return true;
	});

	return result;
}
HRESULT CLevelMapEditorLoader::UnLoad()
{
	LOG_MEMORY("start");

	E::CGameInstance::Get().ClearAllChunk();
	E::CGameInstance::Get().GetNavMeshManager()->Clear();
	E::CGameInstance::Get().DelPrototype("MAPEDITOR");
	E::CGameInstance::Get().DelResource("MAPEDITOR");
	E::CGameInstance::Get().DelResource("MAPEDITOR_TERRAIN_TILE");
	E::CGameInstance::Get().DelResource(TAG_RES_GRP_MAPEDITOR_STATIC_MODEL);

	
	CGameInstance::Get().Clear_DynamicLightList();
	E::CGameInstance::Get().DelResource("LIGHT");

	LOG_MEMORY("end");
	return S_OK;
}
