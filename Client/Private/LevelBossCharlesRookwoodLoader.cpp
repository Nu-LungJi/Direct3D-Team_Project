#include "pch.h"
#include "LevelBossCharlesRookwoodLoader.h"

#include "GameInstance.h"
#include "BackGround.h"

// UI
#include "UIController.h"
#include "EffectUI.h"
#include "TextureUI.h"
#include "Button.h"
#include "TextBox.h"
#include "SpellMeter.h"
#include "HPBar.h"
#include "MiniMap.h"
#include "GameOverMask.h"

NS_USING(Client)

std::future<bool> CLevelBossCharlesRookwoodLoader::Load()
{
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_BossCharlesRookwood", []()
		{
			UILoad();

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

_bool CLevelBossCharlesRookwoodLoader::UILoad()
{
	/**********************UI********************/
	{
		{
			namespace fs = std::filesystem;

			const char* targetDirectories[] = {
				"./Resources/SampleClient/Textures/UI/UITexture/PlayScreen",
				"./Resources/SampleClient/Textures/UI/UITexture/SpellType",
				"./Resources/SampleClient/Textures/UI/UITexture/SpellSlot",
				"./Resources/SampleClient/Textures/UI/UITexture/DeadScene"
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

							if (auto res = E::CGameInstance::Get().AddResource("LEVEL_BOSS_CHARLES_ROOKWOOD", resTag, E::CResTexture2D::Create(fullPath)))
							{
								res->Load();
							}
						}
					}
				}
			}
		}

		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_TextureUI", CTextureUI::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_EffectUI", CEffectUI::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_TextBox", CTextBox::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_Button", CButton::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_SpellMeter", CSpellMeter::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_HPBar", CHPBar::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_MiniMap", CMiniMap::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_UIController", CUIController::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_GameOvermask", CGameOverMask::Create())))
		{
			return false;
		}
	}
}
