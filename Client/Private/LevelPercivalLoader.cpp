#include "pch.h"
#include "LevelPercivalLoader.h"

#include "GameInstance.h"
#include "BackGround.h"

NS_USING(Client)

std::future<bool> CLevelPercivalLoader::Load()
{
	// 메인 스레드 시작
	

	// 메인 스레드 종료
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_PERCIBAL", []()
		{
			// 워커 스레드
			
			// 워커 스레드 종료
			return SUCCEEDED(E::CGameInstance::Get().LoadMapResources(MAP_PATH));
		});
}

std::future<bool> CLevelPercivalLoader::UnLoad()
{
	LOG_MEMORY("start");
	
	LOG_MEMORY("end");
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_PERCIVAL", []()
		{
			E::CGameInstance::Get().DelGameObjectLayer(E::MAPMESHOBJECTLAYER);
			E::CGameInstance::Get().ClearAllChunk();
			E::CGameInstance::Get().DelResource(E::TAG_RES_GRP_MAPEDITOR_STATIC_MODEL);
			return true;
		});
}
