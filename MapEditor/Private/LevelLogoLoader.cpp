#include "pch.h"
#include "LevelLogoLoader.h"

#include "GameInstance.h"
#include "Resources.h"


NS_USING(Client)

std::future<bool> CLevelLogoLoader::Load()
{
	if (auto res = E::CGameInstance::Get().AddResource("MAPEDITOR_LOGO", "TEX_SHM", E::CResTexture2D::Create("./Resources/SampleClient/Textures/SHM.png")))
	{
		res->Load();
	}

	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_MAPEDITOR", []()
		{
			return true;
		});
	
}
HRESULT CLevelLogoLoader::UnLoad()
{
	LOG_MEMORY("start");

	E::CGameInstance::Get().DelResource("MAPEDITOR_LOGO");

	CGameInstance::Get().Clear_DynamicLightList();
	E::CGameInstance::Get().DelResource("LIGHT");

	LOG_MEMORY("end");
	return S_OK;
}
