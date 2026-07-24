#include "pch.h"
#include "LevelBossCharlesRookwoodLoader.h"

#include "GameInstance.h"
#include "BackGround.h"

NS_USING(Client)

std::future<bool> CLevelBossCharlesRookwoodLoader::Load()
{
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_BossCharlesRookwood", []()
		{

			return  true;
		});
}

std::future<bool> CLevelBossCharlesRookwoodLoader::UnLoad()
{
	LOG_MEMORY("start");

	E::CGameInstance::Get().ClearAllChunk();
	E::CGameInstance::Get().GetNavMeshManager()->Clear();

	LOG_MEMORY("end");
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_BossCharlesRookwood", []()
		{
			E::CGameInstance::Get().DelPrototype("MAPEDITOR");
			E::CGameInstance::Get().DelResource("MAPEDITOR");   E::CGameInstance::Get().DelResource(TAG_RES_GRP_MAPEDITOR_STATIC_MODEL);

			return true;
		});
}
