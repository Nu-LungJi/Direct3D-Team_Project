#include "pch.h"
#include "LevelUIEditorLoader.h"

#include "GameInstance.h"
#include "BackGround.h"

NS_USING(Client)
std::future<bool> CLevelUIEditorLoader::Load()
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

HRESULT CLevelUIEditorLoader::UnLoad()
{
    return E_NOTIMPL;
}
