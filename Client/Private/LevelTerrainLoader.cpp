#include "pch.h"
#include "LevelTerrainLoader.h"
#include "GameInstance.h"
#include "Level_Defines.h"
#include "Terrain.h"
#include "Client_Resources.h"
NS_USING(Client)

std::future<bool> CLevelTerrainLoader::Load()
{
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_TERRAIN", []()
		{
			if (auto res = CGameInstance::Get().AddResource(LEVEL::TERRAIN, "TEX2D_Terrain_Tile0", CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds")))
			{
				if (FAILED(res->Load()))
				{
					MSG_BOX("");
					//return E_FAIL;
				}
			}
			if (auto res = CGameInstance::Get().AddResource(LEVEL::TERRAIN, "VIBUFFER_Terrain", CResTerrainVIBuffer::Create("./Resources/SampleClient/Textures/Terrain/Height.bmp")))
			{
				if (FAILED(res->Load(CResTerrainVIBuffer::DESC{})))
				{
					MSG_BOX("LEVEL::TERRAIN Failed VIBUFFER_Terrain ");
					//return false;
				}
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Terrain, CTerrain::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_Terrain");
				return false;
			}
			// 워커 스레드 종료
			return  true;
		});
}

std::future<bool> CLevelTerrainLoader::UnLoad()
{
	LOG_MEMORY("start");
	LOG_MEMORY("end");
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_TERRAIN", []()
		{
			CGameInstance::Get().DelPrototype(LEVEL::TERRAIN);
			CGameInstance::Get().DelResource(LEVEL::TERRAIN);

			return true;
		});
}
