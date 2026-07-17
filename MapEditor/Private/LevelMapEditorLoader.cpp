#include "pch.h"
#include "LevelMapEditorLoader.h"

#include "GameInstance.h"
#include "Resources.h"
#include "ResMapEditorTerrainVIBuffer.h"
#include "MapEditorTerrain.h"

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
	// 터레인 띄우려고 SampleClient에서 복붙해옴
	{
		if (auto res = CGameInstance::Get().AddResource("MAPEDITOR", "TEX2D_Terrain_Tile0", CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds")))
		{
			if (FAILED(res->Load()))
			{
				MSG_BOX("Terrain Tile Png Load Failed");
			}
		}

		if (auto res = CGameInstance::Get().AddResource("MAPEDITOR", "VIBUFFER_Terrain", CResMapEditorTerrainVIBuffer::Create("./Resources/SampleClient/Textures/Terrain/Height.bmp")))
		{
			if (FAILED(res->Load(CResMapEditorTerrainVIBuffer::DESC{})))
			{
				MSG_BOX("Terrain VIBuffer Load Failed");
			}
		}
	}

	//if (!LoadLevelAnimEditorStaticModels())
	//{
	//	MSG_BOX("StaticModel Load Failed");
	//}

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
				desc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f);

				if (FAILED(res->Load(desc)))
				{
					return false;
				}
			}
		);
	}

	result = E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_MAPEDITOR", []()
	{
		// 터레인
		if (FAILED(E::CGameInstance::Get().AddPrototype("MAPEDITOR", "Prototype_GameObject_MapEditorTerrain", CMapEditorTerrain::Create())))
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
	E::CGameInstance::Get().DelResource(TAG_RES_GRP_MAPEDITOR_STATIC_MODEL);

	
	CGameInstance::Get().Clear_DynamicLightList();
	E::CGameInstance::Get().DelResource("LIGHT");

	LOG_MEMORY("end");
	return S_OK;
}
