#include "pch.h"
#include "LevelLogoLoader.h"

#include "GameInstance.h"
#include "BackGround.h"

// UI
#include "UIController.h"
#include "EffectUI.h"
#include "TextureUI.h"
#include "VideoObject.h"


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
			namespace fs = std::filesystem;

			const char* targetDirectories[] = {
				"./Resources/SampleClient/Textures/UI/UITexture/LOGO"
			};

			// 배열을 순회하며 기존 로직을 한 번만 작성하여 처리합니다.
			for (const auto& targetDir : targetDirectories)
			{
				if (fs::exists(targetDir) && fs::is_directory(targetDir))
				{
					for (const auto& entry : fs::directory_iterator(targetDir))
					{
						if (entry.is_regular_file() && entry.path().extension() == ".png")
						{
							std::string fileName = entry.path().stem().string();
							std::string resTag = "TEX_" + fileName;
							std::string fullPath = entry.path().generic_string();

							if (auto res = E::CGameInstance::Get().AddResource("LEVEL_LOGO", resTag, E::CResTexture2D::Create(fullPath)))
							{
								res->Load();
							}
						}
					}
				}
			}
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
