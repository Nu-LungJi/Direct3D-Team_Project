#include "pch.h"
#include "LevelMapEditorLoader.h"
#include "MapEditorStaticModelLoader.h"

#include "GameInstance.h"
#include "Resources.h"
#include "Terrain.h"

NS_USING(Client)

std::future<bool> CLevelMapEditorLoader::Load()
{
	if (auto res = CGameInstance::Get().AddResource("MAPEDITOR_TERRAIN_TILE", "Tile0", CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds")))
	{
		if (FAILED(res->Load()))
		{
			MSG_BOX("Terrain Tile Png Load Failed");
		}
	}

	if (!std::filesystem::exists(E::PATH_MAPEDITOR_STATIC_MODEL_DIR))
	{
		MSG_BOX("NO_STATIC_MODEL_DIR");
	}

	std::future<bool> result;

	const std::filesystem::path terrainTileDir = R"(.\Resources\SampleClient\Textures\Terrain\Tile)";
	if (!std::filesystem::exists(terrainTileDir))
	{
		MSG_BOX("NO_TERRAIN_TILE_DIR");
	}
	bool terrainTilesLoaded = true;
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

		const std::string resourceTag = MakeMapEditorStaticModelTag(terrainTileDir, entry.path());
		auto res = E::CGameInstance::Get().AddResourceT<E::CResTexture2D>(
			"MAPEDITOR_TERRAIN_TILE", resourceTag,
			E::CResTexture2D::Create(entry.path().string()));
		if (!res || FAILED(res->Load()))
		{
			terrainTilesLoaded = false;
			MSG_BOX("Terrain Tile Load Failed");
		}
	}

	result = E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_MAPEDITOR", [terrainTilesLoaded]()
	{
		if (!terrainTilesLoaded)
			return false;
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
