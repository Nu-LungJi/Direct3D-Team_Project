#include "pch.h"
#include "LevelCharlesRookwoodLoader.h"
#include "GameInstance.h"

#include "Player.h"
#include "DebugPlayer.h"
#include "DebugPlayerThirdPersonCamera.h"
#include "PlayerThirdPersonCamera.h"
#include "Level_Defines.h"

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
#include "VideoObject.h"

NS_USING(Client)

std::future<bool> CLevelCharlesRookwoodLoader::Load()
{
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_CharlesRookwood", []()
		{
			// Map Load
			if (FAILED(E::CGameInstance::Get().LoadMapResources(MAP_PATH)))
			{
				return false;
			}
			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>("MODEL", "PLAYER_MODEL_RESROUCE",CResModel::Create("./Resources/SampleClient/Models/Skeleton/professor/SK_professor.bin"))) {

				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(3.f , 3.f, 3.f) * XMMatrixRotationY(XMConvertToRadians(180.f)) * XMMatrixTranslation(0.f, -1.5f, 0.f);
				if (FAILED(res->Load(pDesc))) {
					MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_Player");
					return false;
				}
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_Player, CPlayer::Create())))
			{
				MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_Player");
				return false;
			}

			// 디버그 플레이어 프로토타입 등록
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_DebugPlayer, CDebugPlayer::Create())))
			{
				MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_DebugPlayer");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_DebugPlayerThirdPersonCamera, CDebugPlayerThirdPersonCamera::Create())))
			{
				MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_DebugPlayerThirdPersonCamera");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerThirdPersonCamera, CPlayerThirdPersonCamera::Create())))
			{
				MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_PlayerThirdPersonCamera");
				return false;
			}

			UILoad();

			return true;
		});
}

std::future<bool> CLevelCharlesRookwoodLoader::UnLoad()
{
	LOG_MEMORY("start");

	// 메인스레드 MAP해제
	E::CGameInstance::Get().ClearAllChunk();
	E::CGameInstance::Get().GetNavMeshManager()->Clear();
	
	LOG_MEMORY("end");
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_CharlesRookwood", []()
		{
			// 워커스레드 MAP 해제
			E::CGameInstance::Get().DelPrototype("MAPEDITOR");
			E::CGameInstance::Get().DelResource("MAPEDITOR");   E::CGameInstance::Get().DelResource(TAG_RES_GRP_MAPEDITOR_STATIC_MODEL);

			CGameInstance::Get().DelPrototype(LEVEL::CHARLES_ROOKWOOD);

			return true;
		});
}

_bool CLevelCharlesRookwoodLoader::UILoad()
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

							if (auto res = E::CGameInstance::Get().AddResource("LEVEL_CHARLES_ROOKWOOD", resTag, E::CResTexture2D::Create(fullPath)))
							{
								res->Load();
							}
						}
					}
				}
			}
		}

		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CHARLES_ROOKWOOD", "Prototype_GameObject_TextureUI", CTextureUI::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CHARLES_ROOKWOOD", "Prototype_GameObject_EffectUI", CEffectUI::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CHARLES_ROOKWOOD", "Prototype_GameObject_TextBox", CTextBox::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CHARLES_ROOKWOOD", "Prototype_GameObject_Button", CButton::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CHARLES_ROOKWOOD", "Prototype_GameObject_SpellMeter", CSpellMeter::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CHARLES_ROOKWOOD", "Prototype_GameObject_HPBar", CHPBar::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CHARLES_ROOKWOOD", "Prototype_GameObject_MiniMap", CMiniMap::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CHARLES_ROOKWOOD", "Prototype_GameObject_UIController", CUIController::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CHARLES_ROOKWOOD", "Prototype_GameObject_GameOverMask", CGameOverMask::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_CHARLES_ROOKWOOD", "Prototype_GameObject_VideoObject", CVideoObject::Create())))
		{
			return false;
		}
	}
}
