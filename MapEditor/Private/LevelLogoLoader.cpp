#include "pch.h"
#include "LevelLogoLoader.h"

#include "GameInstance.h"
#include "Resources.h"


NS_USING(Client)

std::future<bool> CLevelLogoLoader::Load()
{
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_MAPEDITOR", []()
		{
			return true;
		});
	
}
HRESULT CLevelLogoLoader::UnLoad()
{
	LOG_MEMORY("start");

	E::CGameInstance::Get().DelResource("TEX_SHM");
	//E::CGameInstance::Get().DelResource("SAMPLE_CLIENT_BUFFER_PHYSX");
	//E::CGameInstance::Get().DelPrototype("SAMPLE_CLIENT_PHYSX");

	CGameInstance::Get().Clear_DynamicLightList();
	E::CGameInstance::Get().DelResource("LIGHT");

	LOG_MEMORY("end");
	return S_OK;
}
