#include "pch.h"
#include "LevelLogoLoader.h"

#include "GameInstance.h"
#include "BackGround.h"

// UI
#include "UIController.h"
#include "EffectUI.h"
#include "TextureUI.h"
#include "VideoObject.h"
#include "UITextureResourceLoader.h"


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
			UILoad();

			// 워커 스레드
			if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LOGO", "Prototype_GameObject_BackGround", CBackGround::Create())))
			{
				return false;
			}
			// 워커 스레드 종료
			return  true;
		});
}

std::future<bool> CLevelLogoLoader::UnLoad()
{
	LOG_MEMORY("start");
	E::CGameInstance::Get().ClearAllRunningEffect();
	E::CGameInstance::Get().DelPrototype("LEVEL_LOGO");
	E::CGameInstance::Get().DelResource("LEVEL_LOGO");

	CGameInstance::Get().Clear_DynamicLightList();
	E::CGameInstance::Get().DelResource("LIGHT");
	LOG_MEMORY("end");
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_LOGO", []()
		{
			return true;
		});
}

_bool CLevelLogoLoader::UILoad()
{
	/**********************UI********************/
	{
		{
			const char* targetDirectories[] = {
				"./Resources/SampleClient/Textures/UI/UITexture/LOGO"
			};

			for (const auto& targetDir : targetDirectories)
				UITextureResourceLoader::LoadDirectory(
					"LEVEL_LOGO", targetDir);
		}

		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LOGO", "Prototype_GameObject_TextureUI", CTextureUI::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LOGO", "Prototype_GameObject_EffectUI", CEffectUI::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LOGO", "Prototype_GameObject_UIController", CUIController::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LOGO", "Prototype_GameObject_VideoObject", CVideoObject::Create())))
		{
			return false;
		}
	}

	return true;
}
