#include "pch.h"
#include "LevelLogoLoader.h"

#include "GameInstance.h"
#include "BackGround.h"

NS_USING(Client)

std::future<bool> CLevelLogoLoader::Load()
{
	// 메인 스레드 시작
	if (auto res = E::CGameInstance::Get().AddResource("LEVEL_LOGO", "TEX_SHM", E::CResTexture2D::Create("./Resources/SampleClient/Textures/SHM.png")))
	{
		res->Load();
	}

	// 메인 스레드 종료
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_LOGO", []()
		{
			// 워커 스레드
			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LOGO", "Prototype_GameObject_BackGround", CBackGround::Create())))
			{
				return false;
			}
			// 워커 스레드 종료
			return  true;
		});
}

HRESULT CLevelLogoLoader::UnLoad()
{
	LOG_MEMORY("start");
	E::CGameInstance::Get().DelPrototype("LEVEL_LOGO");
	E::CGameInstance::Get().DelResource("LEVEL_LOGO");

	CGameInstance::Get().Clear_DynamicLightList();
	E::CGameInstance::Get().DelResource("LIGHT");
	LOG_MEMORY("end");
	return S_OK;
}
