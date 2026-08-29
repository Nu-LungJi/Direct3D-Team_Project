#include "pch.h"
#include "LevelHogwartWorldLoader.h"

#include "GameInstance.h"
#include "Level_Defines.h"
#include "Client_Resources.h"
#include "Player.h"
#include "WiggenweldPotion.h"
#include "PlayerThirdPersonCamera.h"
#include "Player_Weapon.h"
#include "Player_Broom.h"
#include "Player_Magic_Bullet.h"
#include "Player_Bombarda_Bullet.h"
#include "Player_Confringo_Bullet.h"
#include "Player_Stupefy_Bullet.h"
#include "NvClothCape.h"
#include "ResNvClothMesh.h"
#include "WaterWheel.h"

#include "UIController.h"
#include "EffectUI.h"
#include "TextureUI.h"
#include "Button.h"
#include "GeneralButton.h"
#include "TextBox.h"
#include "SpellMeter.h"
#include "HPBar.h"
#include "MiniMap.h"
#include "GameOverMask.h"
#include "VideoObject.h"
#include "Cursor.h"
#include "SpellMiniGame.h"
#include "UITextureResourceLoader.h"
#include "Spider.h"
#include "Mon_Spawner.h"
#include "Mon_State.h"
#include "WorldNpc.h"
#include "InteractiveNpc.h"
#include "ShopNpc.h"
#include "Griff.h"
#include "GriffChild.h"
#include "Troll.h"
#include "TrollWeapon.h"
#include "WorldAnimal.h"
#include "PhysicsDoor.h"
#include "AccioBall.h"
#include "AccioActivity_Base.h"
#include "AccioActivity_Platform.h"
#include "AccioActivity_BumperA.h"
#include "AccioActivity_BumperB.h"
#include "AccioActivity_RampLarge.h"
#include "AccioActivity_LampSmall.h"
#include "AccioActivity_NpcController.h"
#include "AccioActivity_NpcCharacter.h"
#include "AnimatedWorldObject.h"
// Client Terrain과 구분하기 위해 Engine Terrain 헤더를 명시한다.
#include "../../EngineSDK/Inc/Terrain.h"
#include "Water.h"
#include "WayPointManager.h"
#include "Coin.h"
NS_USING(Client)

std::future<bool> CLevelHogwartWorldLoader::Load()
{
	return E::CGameInstance::Get().WorkerEnqueueWithFuture(
		"LOADING_HogwartWorld",
		[]()
		{
			if (FAILED(E::CGameInstance::Get().LoadMapResources(MAP_PATH)))
				return false;

			if (!UILoad_InWorker())
				return false;

			if (FAILED(E::CGameInstance::Get().LoadCinematic("AcientThunderAttack")))
				return false;
			if (FAILED(E::CGameInstance::Get().LoadCinematic("InteractiveNpcDialogue")))
				return false;
			if (FAILED(E::CGameInstance::Get().LoadCinematic("ShopNpcEntrance")))
				return false;
			if (FAILED(E::CGameInstance::Get().LoadCinematic("ShopNpcDialogueCloseUp")))
				return false;
			if (FAILED(E::CGameInstance::Get().LoadCinematic("ShopNpcWandBox")))
				return false;
			if (FAILED(E::CGameInstance::Get().LoadCinematic("ShopNpcSpellLesson")))
				return false;

			if (auto texture = E::CGameInstance::Get().AddResource(
				LEVEL::HOGWART_WORLD,
				"TEX2D_Terrain_Tile0",
				E::CResTexture2D::Create(
					"./Resources/SampleClient/Textures/Terrain/Tile0.dds")))
			{
				if (FAILED(texture->Load()))
					return false;
			}
			else
			{
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::HOGWART_WORLD,
				PROTO_GAMEOBJECT::Prototype_GameObject_Terrain,
				E::CTerrain::Create())))
			{
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::HOGWART_WORLD,
				PROTO_GAMEOBJECT::Prototype_GameObject_Water,
				E::CWater::Create())))
			{
				return false;
			}

			if(FAILED(MonsterLoad_InWorker()))
				return false;
			if (FAILED(NpcLoad_InWorker()))
				return false;
			if (FAILED(AnimatedObjectLoad_InWorker()))
				return false;
			if (FAILED(WorldAgentLoad_InWorker()))
				return false;

			if (FAILED(LoadCollsion_InWorker()))
				return false;
			if (FAILED(E::CGameInstance::Get().LoadCinematic("TrollDoljin")))
			{
				return false;
			}
			if (FAILED(E::CGameInstance::Get().LoadCinematic("SpiderSpawn")))
			{
				return false;
			}

			if (FAILED(LoadHogsmeade_ExtraAsset()))
				return false;
			if (FAILED(LoadPhysicsDoorResources()))
				return false;
			if (FAILED(LoadAccioActivityResources()))
				return false;

			if (FAILED(LoadWay_InWorker()))
				return false;
			return SUCCEEDED(LoadPlayerResources());
		});
}

std::future<bool> CLevelHogwartWorldLoader::UnLoad()
{
	E::CGameInstance::Get().ClearAllRunningEffect();
	E::CGameInstance::Get().ClearAllChunk();
	E::CGameInstance::Get().GetNavMeshManager()->Clear();
	E::CGameInstance::Get().Clear_DynamicLightList();

	return E::CGameInstance::Get().WorkerEnqueueWithFuture(
		"UNLOADING_HogwartWorld",
		[]()
		{
			E::CGameInstance::Get().DelPrototype("MAPEDITOR");
			E::CGameInstance::Get().DelResource("MAPEDITOR");
			E::CGameInstance::Get().DelResource(TAG_RES_GRP_MAPEDITOR_STATIC_MODEL);
			E::CGameInstance::Get().DelPrototype("LEVEL_HOGWART_WORLD");
			E::CGameInstance::Get().DelResource("LEVEL_HOGWART_WORLD");
			E::CGameInstance::Get().DelPrototype(LEVEL::HOGWART_WORLD);
			E::CGameInstance::Get().DelResource(LEVEL::HOGWART_WORLD);
			return true;
		});
}

HRESULT CLevelHogwartWorldLoader::LoadPhysicsDoorResources()
{
	auto resource = CGameInstance::Get().AddResourceT<CResStaticModel>(
		CURR_LEVEL,
		"Static_PhysicsDoor_Resource",
		CResStaticModel::Create(
			"./Resources/SampleClient/Models/Static/LCJ_ObjecMap/"
			"SM_BP_Door_Template64_1295.bin"));
	if (!resource)
		return E_FAIL;

	CResStaticModel::DESC desc{};
	desc.PreTransformMatrix =
		XMMatrixScaling(0.03f, 0.03f, 0.03f) *
		XMMatrixTranslation(-1.875f, -3.73479f, 0.031791f);
	if (FAILED(resource->Load(desc)))
		return E_FAIL;

	return CGameInstance::Get().AddPrototype(
		CURR_LEVEL,
		PROTO_GAMEOBJECT::Prototype_GameObject_PhysicsDoor,
		CPhysicsDoor::Create());
}

HRESULT CLevelHogwartWorldLoader::LoadAccioActivityResources()
{
	const auto loadStaticModel = [](
		const StringID& resourceTag,
		const _char* modelPath,
		FXMMATRIX preTransform)
	{
		auto resource = CGameInstance::Get().AddResourceT<CResStaticModel>(
			CURR_LEVEL,
			resourceTag,
			CResStaticModel::Create(modelPath));
		if (!resource)
			return false;

		CResStaticModel::DESC desc{};
		desc.PreTransformMatrix = preTransform;
		return SUCCEEDED(resource->Load(desc));
	};

	if (!loadStaticModel(
		"Static_AccioBall_Blue_Resource",
		"./Resources/SampleClient/Models/Static/"
		"SM_SM_HM_Quid_BallBox_Quaffle_RoundA_Blue.bin",
		XMMatrixIdentity()))
	{
		return E_FAIL;
	}
	if (!loadStaticModel(
		"Static_AccioBall_Red_Resource",
		"./Resources/SampleClient/Models/Static/"
		"SM_SM_HM_Quid_BallBox_Quaffle_RoundA_Red.bin",
		XMMatrixIdentity()))
	{
		return E_FAIL;
	}

	struct ACCIO_STATIC_RESOURCE
	{
		const _char* pTag;
		const _char* pPath;
		_matrix PreTransform;
	};

	const ACCIO_STATIC_RESOURCE staticResources[] =
	{
		{
			"Static_AccioActivity_Resource",
			"./Resources/SampleClient/Models/Static/SM_SM_HW_AccioActivity.bin",
			XMMatrixScaling(500.f, 500.f, 500.f) *
			XMMatrixRotationX(XMConvertToRadians(90.f))
		},
		{
			"Static_AccioActivity_Platform_Resource",
			"./Resources/SampleClient/Models/Static/SM_SM_HW_AccioActivity_Platform.bin",
			XMMatrixScaling(600.f, 600.f, 600.f) *
			XMMatrixRotationX(XMConvertToRadians(90.f))
		},
		{
			"Static_AccioActivity_Bumper_Resource",
			"./Resources/SampleClient/Models/Static/SM_SM_HW_AccioActivity_Bumper.bin",
			XMMatrixIdentity()
		},
		{
			"Static_AccioActivity_BumperA_Resource",
			"./Resources/SampleClient/Models/Static/SM_SM_HW_AccioActivity_Bumper_A.bin",
			XMMatrixIdentity()
		},
		{
			"Static_AccioActivity_RampLarge_Resource",
			"./Resources/SampleClient/Models/Static/SM_SM_HW_AccioActivity_RampLarge.bin",
			XMMatrixIdentity()
		},
		{
			"Static_AccioActivity_RampSmall_Resource",
			"./Resources/SampleClient/Models/Static/SM_SM_HW_AccioActivity_RampSmall.bin",
			XMMatrixIdentity()
		}
	};

	for (const auto& entry : staticResources)
	{
		if (!loadStaticModel(entry.pTag, entry.pPath, entry.PreTransform))
			return E_FAIL;
	}

	auto studentModel = CGameInstance::Get().AddResourceT<CResModel>(
		CURR_LEVEL,
		"ACCIO_ACTIVITY_STUDENT_MODEL_RESOURCE",
		CResModel::Create(
			"./Resources/SampleClient/Models/Skeleton/"
			"ElegantStudent_PrettyGirl2_RigCorrectedFinal/"
			"SK_ElegantStudent_PrettyGirl2_RigCorrectedFinal.bin"));
	if (!studentModel)
		return E_FAIL;

	CResModel::DESC studentDesc{};
	studentDesc.PreTransformMatrix =
		XMMatrixScaling(3.f, 3.f, 3.f) *
		XMMatrixRotationY(XMConvertToRadians(180.f)) *
		XMMatrixTranslation(0.f, -1.8f, 0.f);
	if (FAILED(studentModel->Load(studentDesc)))
		return E_FAIL;

	if (FAILED(CGameInstance::Get().AddPrototype(
		CURR_LEVEL,
		PROTO_GAMEOBJECT::Prototype_GameObject_AccioBall,
		CAccioBall::Create())))
	{
		return E_FAIL;
	}
	if (FAILED(CGameInstance::Get().AddPrototype(
		CURR_LEVEL,
		PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_Base,
		CAccioActivity_Base::Create())))
	{
		return E_FAIL;
	}
	if (FAILED(CGameInstance::Get().AddPrototype(
		CURR_LEVEL,
		PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_Platform,
		CAccioActivity_Platform::Create())))
	{
		return E_FAIL;
	}
	if (FAILED(CGameInstance::Get().AddPrototype(
		CURR_LEVEL,
		PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_BumperA,
		CAccioActivity_BumperA::Create())))
	{
		return E_FAIL;
	}
	if (FAILED(CGameInstance::Get().AddPrototype(
		CURR_LEVEL,
		PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_BumperB,
		CAccioActivity_BumperB::Create())))
	{
		return E_FAIL;
	}
	if (FAILED(CGameInstance::Get().AddPrototype(
		CURR_LEVEL,
		PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_RampLarge,
		CAccioActivity_RampLarge::Create())))
	{
		return E_FAIL;
	}
	if (FAILED(CGameInstance::Get().AddPrototype(
		CURR_LEVEL,
		PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_LampSmall,
		CAccioActivity_LampSmall::Create())))
	{
		return E_FAIL;
	}
	if (FAILED(CGameInstance::Get().AddPrototype(
		CURR_LEVEL,
		PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_NpcController,
		CAccioActivity_NpcController::Create())))
	{
		return E_FAIL;
	}

	return CGameInstance::Get().AddPrototype(
		CURR_LEVEL,
		PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_NpcCharacter,
		CAccioActivity_NpcCharacter::Create());
}

HRESULT CLevelHogwartWorldLoader::LoadPlayerResources()
{
	if (auto model = E::CGameInstance::Get().AddResourceT<E::CResModel>(
		LEVEL::HOGWART_WORLD,
		"PLAYER_MODEL_RESROUCE",
		E::CResModel::Create(
			"./Resources/SampleClient/Models/Skeleton/professor/SK_professor.bin")))
	{
		E::CResModel::DESC desc{};
		desc.PreTransformMatrix =
			XMMatrixScaling(3.f, 3.f, 3.f) *
			XMMatrixRotationY(XMConvertToRadians(180.f)) *
			XMMatrixTranslation(0.f, -1.4f, 0.f);
		if (FAILED(model->Load(desc)))
			return E_FAIL;
	}
	else
	{
		return E_FAIL;
	}

	if (auto potion = E::CGameInstance::Get().AddResourceT<E::CResStaticModel>(
		LEVEL::HOGWART_WORLD,
		"Static_WiggenweldPotion_Resource",
		E::CResStaticModel::Create(
			"./Resources/SampleClient/Models/Static/Potion_Wiggenweld/SM_Potion_Wiggenweld.bin")))
	{
		E::CResStaticModel::DESC desc{};
		desc.PreTransformMatrix =  XMMatrixScaling(2.f, 2.f, 2.f);
		if (FAILED(potion->Load(desc))) return E_FAIL;
	}
	else return E_FAIL;

	if (FAILED(LoadPlayerCape()))
		return E_FAIL;

	if (auto weapon = E::CGameInstance::Get().AddResourceT<E::CResStaticModel>(
		LEVEL::HOGWART_WORLD,
		"PLAYER_WEAPON_RESROUCE",
		E::CResStaticModel::Create(
			"./Resources/SampleClient/Models/Static/SM_Wand.bin")))
	{
		E::CResStaticModel::DESC desc{};
		desc.PreTransformMatrix = XMMatrixRotationX(XMConvertToRadians(-90.f)) * XMMatrixIdentity();
		if (FAILED(weapon->Load(desc)))
			return E_FAIL;
	}
	else
	{
		return E_FAIL;
	}
	if (auto broom = E::CGameInstance::Get().AddResourceT<E::CResModel>(
		LEVEL::HOGWART_WORLD,
		"PLAYER_BROOM_RESOURCE",
		E::CResModel::Create(
			"./Resources/SampleClient/Models/Skeleton/professor/Broom/SK_FlyingClassBroom_01.bin")))
	{
		E::CResModel::DESC desc{};
		desc.PreTransformMatrix = XMMatrixIdentity();
		if (FAILED(broom->Load(desc)))
			return E_FAIL;
	}
	else
	{
		return E_FAIL;
	}

	auto& gameInstance = E::CGameInstance::Get();
	if (FAILED(gameInstance.AddPrototype(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_PlayerWeapon,
		CPlayer_Weapon::Create())) ||
		FAILED(gameInstance.AddPrototype(
			LEVEL::HOGWART_WORLD,
			PROTO_GAMEOBJECT::Prototype_GameObject_PlayerBroom,
			CPlayer_Broom::Create())) ||
		FAILED(gameInstance.AddPrototype(
			LEVEL::HOGWART_WORLD,
			PROTO_GAMEOBJECT::Prototype_GameObject_PlayerMagicBullet,
			CPlayer_Magic_Bullet::Create())) ||
		FAILED(gameInstance.AddPrototype(
			LEVEL::HOGWART_WORLD,
			PROTO_GAMEOBJECT::Prototype_GameObject_PlayerConfringoBullet,
			CPlayer_Confringo_Bullet::Create())) ||
		FAILED(gameInstance.AddPrototype(
			LEVEL::HOGWART_WORLD,
			PROTO_GAMEOBJECT::Prototype_GameObject_PlayerBombardaBullet,
			CPlayer_Bombarda_Bullet::Create())) ||
		FAILED(gameInstance.AddPrototype(
			LEVEL::HOGWART_WORLD,
			PROTO_GAMEOBJECT::Prototype_GameObject_PlayerStupefyBullet,
			CPlayer_Stupefy_Bullet::Create())) ||
		FAILED(gameInstance.AddPrototype(
			LEVEL::HOGWART_WORLD,
			PROTO_GAMEOBJECT::Prototype_GameObject_Player,
			CPlayer::Create())) ||
		FAILED(gameInstance.AddPrototype(
			LEVEL::HOGWART_WORLD,
			PROTO_GAMEOBJECT::Prototype_GameObject_WiggenweldPotion,
			CWiggenweldPotion::Create())) ||
		FAILED(gameInstance.AddPrototype(
			LEVEL::HOGWART_WORLD,
			PROTO_GAMEOBJECT::Prototype_GameObject_PlayerThirdPersonCamera,
			CPlayerThirdPersonCamera::Create())))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CLevelHogwartWorldLoader::LoadPlayerCape()
{
	constexpr char CAPE_MODEL_PATH[] =
		"./Resources/SampleClient/Models/Skeleton/clothes/SK_clothes.bin";
	const _matrix preTransform =
		XMMatrixScaling(3.f, 3.f, 3.f) *
		XMMatrixRotationY(XMConvertToRadians(180.f)) *
		XMMatrixTranslation(0.f, -1.5f, 0.f);

	if (auto model = E::CGameInstance::Get().AddResourceT<E::CResModel>(
		LEVEL::HOGWART_WORLD,
		"PLAYER_CAPE_MODEL_RESOURCE",
		E::CResModel::Create(CAPE_MODEL_PATH)))
	{
		E::CResModel::DESC desc{};
		desc.PreTransformMatrix = preTransform;
		if (FAILED(model->Load(desc)))
			return E_FAIL;
	}
	else
	{
		return E_FAIL;
	}

	if (auto cloth = E::CGameInstance::Get().AddResourceT<E::CResNvClothMesh>(
		LEVEL::HOGWART_WORLD,
		"PLAYER_CAPE_CLOTH_RESOURCE",
		E::CResNvClothMesh::Create(CAPE_MODEL_PATH)))
	{
		E::CResNvClothMesh::DESC desc{};
		desc.PreTransformMatrix = preTransform;
		desc.sSimulationAnchorBone = "Spine3";
		desc.iSimulationMeshIndex = 0;
		desc.iRenderMeshIndex = 1;
		desc.fWeldTolerance = 1.e-5f;
		desc.fFixedTopRatio = 0.1f;
		if (FAILED(cloth->Load(desc)))
			return E_FAIL;
	}
	else
	{
		return E_FAIL;
	}

	return E::CGameInstance::Get().AddPrototype(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_NvClothCape,
		CNvClothCape::Create());
}

_bool CLevelHogwartWorldLoader::UILoad_InWorker()
{
	/**********************UI********************/
	{
		{
			const char* targetDirectories[] = {
				"./Resources/SampleClient/Textures/UI/UITexture/PlayScreen",
				"./Resources/SampleClient/Textures/UI/UITexture/SpellType",
				"./Resources/SampleClient/Textures/UI/UITexture/SpellSlot",
				"./Resources/SampleClient/Textures/UI/UITexture/DeadScene",
				"./Resources/SampleClient/Textures/UI/UITexture/Cursor",
				"./Resources/SampleClient/Textures/UI/UITexture/WandShop",
				"./Resources/SampleClient/Textures/UI/UITexture/SpellMiniGame",
				"./Resources/SampleClient/Textures/UI/UITexture/MiniGame",
				"./Resources/SampleClient/Textures/UI/FlipBook"
			};

			for (const auto& targetDir : targetDirectories)
				UITextureResourceLoader::LoadDirectory(
					"LEVEL_HOGWART_WORLD", targetDir);
		}

		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_HOGWART_WORLD", "Prototype_GameObject_TextureUI", CTextureUI::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_HOGWART_WORLD", "Prototype_GameObject_EffectUI", CEffectUI::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_HOGWART_WORLD", "Prototype_GameObject_TextBox", CTextBox::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_HOGWART_WORLD", "Prototype_GameObject_Button", CButton::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_HOGWART_WORLD", "Prototype_GameObject_GeneralButton", CGeneralButton::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_HOGWART_WORLD", "Prototype_GameObject_SpellMeter", CSpellMeter::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_HOGWART_WORLD", "Prototype_GameObject_HPBar", CHPBar::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_HOGWART_WORLD", "Prototype_GameObject_MiniMap", CMiniMap::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_HOGWART_WORLD", "Prototype_GameObject_UIController", CUIController::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(
			"LEVEL_HOGWART_WORLD",
			"Prototype_GameObject_SpellMiniGame",
			CSpellMiniGame::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_HOGWART_WORLD", "Prototype_GameObject_GameOverMask", CGameOverMask::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_HOGWART_WORLD", "Prototype_GameObject_VideoObject", CVideoObject::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_HOGWART_WORLD", "Prototype_GameObject_Cursor", CCursor::Create())))
		{
			return false;
		}
		
	}
	return true;
}
HRESULT CLevelHogwartWorldLoader::MonsterLoad_InWorker()
{
	{
		if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::HOGWART_WORLD, "Model_Resource_Spider",
			CResModel::Create("./Resources/SampleClient/Models/Skeleton/Spider/SK_Spider.bin"))) {

			E::CResModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(6.f, 6.f, 6.f) * XMMatrixRotationY(XMConvertToRadians(180.f));

			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("HOGWART_WORLD Failed Model_Resource_Spider");
				return E_FAIL;
			}
		}

		//트롤
		{
			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::HOGWART_WORLD, "Model_Resource_Troll",
				CResModel::Create("./Resources/SampleClient/Models/Skeleton/Troll/SK_Troll.bin"))) {

				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(4.f, 4.f, 4.f) * XMMatrixRotationY(XMConvertToRadians(180.f));

				if (FAILED(res->Load(pDesc)))
				{
					MSG_BOX("HOGWART_WORLD Failed Model_Resource_Troll");
					return E_FAIL;
				}
			}
			if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::HOGWART_WORLD, "Model_Resource_TrollWeapon",
				CResStaticModel::Create("./Resources/SampleClient/Models/Static/TrollWeapon/SM_TrollWeapon.bin"))) {

				E::CResStaticModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(3.f, 3.f, 3.f);

				if (FAILED(res->Load(pDesc)))
				{
					MSG_BOX("HOGWART_WORLD Failed Model_Resource_TrollWeapon");
					return E_FAIL;
				}
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_TrollWeapon, CTrollWeapon::Create())))
			{
				MSG_BOX("HOGWART_WORLD Failed Prototype_GameObject_TrollWeapon");
				return E_FAIL;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Troll, CTroll::Create())))
			{
				MSG_BOX("HOGWART_WORLD Failed Prototype_GameObject_Troll");
				return E_FAIL;
			}
		}
		if (auto res = CGameInstance::Get().AddResource("SPAWNER", "EVENTSPIDER", CResJson::Create("./Resources/json/Spawn/EVENTSPIDER.json")))
		{
			if (FAILED(res->Load()))
			{
				MSG_BOX("LOAD FAILED EDGWAYPT EVENTSPAWN JSON");
				return E_FAIL;
			}
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Spider, CSpider::Create())))
		{
			MSG_BOX("HOGWART_WORLD Failed Prototype_GameObject_Spider");
			return E_FAIL;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_MonSpawner, CMon_Spawner::Create())))
		{
			MSG_BOX("HOGWART_WORLD Failed Prototype_GameObject_Spawner");
			return E_FAIL;
		}

		if (FAILED(CGameInstance::Get().AddPrototype(LEVEL::HOGWART_WORLD, "Prototype_Component_Mon_FSM", CMon_State::Create()))) return E_FAIL;
		
		if (auto res = CGameInstance::Get().AddResource("BTJSON", "TROLL", CResJson::Create("./Resources/json/BeHavior/Troll.json")))
		{
			if (FAILED(res->Load()))
			{
				MSG_BOX("LOAD FAILED TROLL JSON");
				return E_FAIL;
			}
		}
	}
}

HRESULT CLevelHogwartWorldLoader::NpcLoad_InWorker()
{
	struct NPC_MODEL_ENTRY { const char* pTag; const char* pCharacter; };
	static constexpr NPC_MODEL_ENTRY NpcModels[] =
	{
		// NpcSpawnIdle/NpcSpawnWalk에서 실제 사용하는 NPC만 선로드한다.
		// 리소스 폴더의 Viector 오타는 런타임 태그와 분리해 여기서만 보정한다.
		{ "Model_Resource_NPC_GerboldOllivander", "GerboldOllivander" },
		{ "Model_Resource_NPC_VictorRookwood", "VictorRookwood_lsy" },
		{ "Model_Resource_NPC_LeopoldBabcocke", "LeopoldBabcocke" },
		{ "Model_Resource_NPC_SolomonSallow", "SolomonSallow" },
		{ "Model_Resource_NPC_TownCrier", "TownCrier" },
		{ "Model_Resource_NPC_CrispinDunn", "CrispinDunn" },
		{ "Model_Resource_NPC_AugustusHill", "AugustusHill" },
		{ "Model_Resource_NPC_AdelaideOakes", "AdelaideOakes" },
		{ "Model_Resource_NPC_AnneSallow", "AnneSallow" },
		{ "Model_Resource_NPC_MirabelGarlick", "MirabelGarlick" },
	};
	for (const auto& Entry : NpcModels)
	{
		const _string ModelPath = "./Resources/SampleClient/Models/Skeleton/NPC_" + _string(Entry.pCharacter) +
			"/SK_NPC_" + Entry.pCharacter + ".bin";
		if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(
			LEVEL::HOGWART_WORLD, Entry.pTag, CResModel::Create(ModelPath)))
		{
			E::CResModel::DESC Desc{};
			Desc.PreTransformMatrix = XMMatrixScaling(3.f, 3.f, 3.f) *
				XMMatrixRotationY(XMConvertToRadians(180.f));
			if (FAILED(res->Load(Desc)))
			{
				MessageBoxA(g_hWnd, ModelPath.c_str(), "hm", MB_OK | MB_ICONERROR);
				MSG_BOX("HOGWART Failed NPC model resource");
				return E_FAIL;
			}
		}
	}

	if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_WorldNpc, CWorldNpc::Create())))
	{
		MSG_BOX("HOGWART_WORLD Failed Prototype_GameObject_Npc");
		return E_FAIL;
	}
	if (FAILED(E::CGameInstance::Get().AddPrototype(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_MiniGameNpc,
		CInteractiveNpc::Create())))
	{
		MSG_BOX("HOGWART_WORLD Failed Prototype_GameObject_MiniGameNpc");
		return E_FAIL;
	}
	if (FAILED(E::CGameInstance::Get().AddPrototype(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_ShopNpc,
		CShopNpc::Create())))
	{
		MSG_BOX("HOGWART_WORLD Failed Prototype_GameObject_ShopNpc");
		return E_FAIL;
	}
	return S_OK;
}

HRESULT CLevelHogwartWorldLoader::AnimatedObjectLoad_InWorker()
{
	static constexpr const char* ModelNames[] =
	{
		"AnimatedGlobe_Animated",
		"BalloonLauncher_Animated",
		"CottonCandyDisplay_Animated",
		"DeathdayParty_Animated",
		"DragonBush_Animated",
		"EnchantedScarecrow_Animated",
		"EnchantedWateringCan_Animated",
		"HungryForRubbish_Animated",
		"LivingBooks_Animated",
		"MagicKiteBattle_Animated",
		"ManicStreetSigns_Animated",
		"MarionetteCandyBooth_Animated",
		"MirrorMirror_Animated",
		"NifflerTightropeToy_Animated",
		"OneManBand_Animated",
		"PaperAndQuill_Animated",
		"PlantParty_Animated",
		"PlayingWithFire_Animated",
		"RollUpRollUpCart_Animated",
		"SelfCheckingBooks_Animated",
		"SelfPruningTools_Animated",
		"SelfShufflingCards_Animated",
		"SelfWrappingPresent_Animated",
		"Snowman_Animated",
		"StirCrazyKitchen_Animated"
	};

	auto& gameInstance = E::CGameInstance::Get();
	for (const char* modelName : ModelNames)
	{
		const _string resourceTag =
			"Model_Resource_AnimatedObject_" + _string{ modelName };
		const _string modelPath =
			"./Resources/SampleClient/Models/AnimatedObject/" +
			_string{ modelName } + "/SK_" + modelName + ".bin";

		auto model = gameInstance.AddResourceT<E::CResModel>(
			LEVEL::HOGWART_WORLD,
			resourceTag,
			E::CResModel::Create(modelPath));
		if (!model)
		{
			MSG_BOX("HOGWART_WORLD Failed to register AnimatedObject model resource");
			return E_FAIL;
		}

		E::CResModel::DESC modelDesc{};
		modelDesc.PreTransformMatrix = XMMatrixIdentity();
		if (FAILED(model->Load(modelDesc)))
		{
			MSG_BOX("HOGWART_WORLD Failed to load AnimatedObject model resource");
			return E_FAIL;
		}
	}

	return S_OK;
}

HRESULT CLevelHogwartWorldLoader::WorldAgentLoad_InWorker()
{
	{
		const _string resourceTag =
			"Model_Resource_Ollivander_WandBox_Full_Selection";
		const _string modelPath =
			"./Resources/SampleClient/Models/Skeleton/"
			"Ollivander_WandBox_Full_Selection/"
			"SK_Ollivanders_WandBox_Full_Selection.bin";
		auto model = CGameInstance::Get().AddResourceT<E::CResModel>(
			LEVEL::HOGWART_WORLD, resourceTag, CResModel::Create(modelPath));
		if (!model)
			return E_FAIL;
		E::CResModel::DESC desc{};
		// The supplied BIN is already authored at gameplay scale. Applying the
		// generic Blender x100 correction makes the hand prop enormous.
		desc.PreTransformMatrix = XMMatrixIdentity();
		if (FAILED(model->Load(desc)) || model->GetAnimations().empty())
			return E_FAIL;
	}

	struct MODEL_ANIMAL
	{ _string ResName{};					_string PathName{};				_float3 vScale{3.f,3.f,3.f}; };
	MODEL_ANIMAL resAnimal[]{ 
	{"Model_Resource_Griff",	   "./Resources/SampleClient/Models/Skeleton/Griff/SK_Griff.bin",_float3(6.f,6.f,6.f)},
	{"Model_Resource_Cat",		   "./Resources/SampleClient/Models/Skeleton/Cat/SK_Cat.bin"},
	{"Model_Resource_Bird_Kestrel","./Resources/SampleClient/Models/Skeleton/Birds_Kestrel/SK_Birds_Kestrel.bin"},
	{"Model_Resource_Bird_Phoneix","./Resources/SampleClient/Models/Skeleton/Phoneix/SK_Phoneix.bin",_float3(5.f,5.f,5.f) },};

	for (auto& iter : resAnimal)
	{
		if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::HOGWART_WORLD, iter.ResName,CResModel::Create(iter.PathName))) {
			E::CResModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(iter.vScale.x, iter.vScale.y, iter.vScale.z) * XMMatrixRotationY(XMConvertToRadians(180.f));

			if (FAILED(res->Load(pDesc)))
			{
				_string FailedName = "HOGWART_WORLD Failed" + iter.ResName;
				MessageBoxA(g_hWnd, FailedName.c_str(), "System Error Message", MB_OK | MB_ICONERROR);
				return E_FAIL;
			}
		}
	}

	static constexpr const char* Blender43Skeletons[] =
	{
		"BlueButterfly_Animated_Blender_4_3", "CaptureBag_Animated_Blender_4_3",
		"ChompingCabbage_Animated_Blender_4_3", "DisillusionmentChest_Animated_Blender_4_3",
		"FlyingMagicPaper_Animated_Blender_4_3", "GACTreasureChest_Animated_Blender_4_3",
		"GiantPendulumClock_Animated_Blender_4_3", "GlowingLumosMoth_Animated_Blender_4_3",
		"Hippogriff_Animated_Blender_4_3", "HoppingPot_Animated_Blender_4_3",
		"IdentificationStation_Animated_Blender_4_3", /*"MagicChoppingIngredients_Animated_Blender_4_3",
		"MagicChoppingStation_Animated_Blender_4_3", "MagicMaterialRefinerTools_Animated_Blender_4_3",*/
		"OrangeButterfly_Animated_Blender_4_3", "OutdoorDiricawlBird_Animated_Blender_4_3",
		"OutdoorFwooperBird_Animated_Blender_4_3", "SanctuaryToyBox_Animated_Blender_4_3",
		"SelfWrappingPaper_Animated_Blender_4_3", "ShopCounterHandBell_Animated_Blender_4_3",
		"StirCrazyTeaSpoon_Animated_Blender_4_3", "StreetRabbit_Animated_Blender_4_3",
		"StreetRat_Animated_Blender_4_3", "StreetRaven_Animated_Blender_4_3",
		"StreetSquirrel_Animated_Blender_4_3", "TeaShopTeaCup_Animated_Blender_4_3",
		"ThestralStreetCarriage_Animated_Blender_4_3", "VillageGiantToad_Animated_Blender_4_3",
		"WizardingDeck_Animated_Blender_4_3",
		"BlueButterfly", "GlowingLumosMoth", "LeapingMushroom",
		"OrangeButterfly", "PlantParty_Plant_01", "PlantParty_Plant_02",
		"PlantParty_Plant_03", "PlantParty_Plant_04", "PlantParty_Plant_05",
		"VenomousTentaculaBush", "VenomousTentaculaFlower"
	};
	for (const char* modelName : Blender43Skeletons)
	{
		const _string folder = modelName;
		const _string resourceTag = "Model_Resource_" + folder;
		const _string modelPath = "./Resources/SampleClient/Models/Skeleton/" + folder + "/SK_" + folder + ".bin";
		// The resource manager stores multiple resources under one tag while
		// model instances resolve the first one. Re-registering here could load
		// a new model successfully but leave runtime objects bound to an older,
		// animation-empty model. Reuse the same resource the instances resolve.
		auto model = CGameInstance::Get().GetResourceFirst<E::CResModel>(
			LEVEL::HOGWART_WORLD, resourceTag);
		if (!model)
		{
			model = CGameInstance::Get().AddResourceT<E::CResModel>(
				LEVEL::HOGWART_WORLD, resourceTag,
				CResModel::Create(modelPath));
		}
		if (!model)
			return E_FAIL;

		E::CResModel::DESC desc{};
		// Blender 4.3 export 모델은 정점 단위가 월드 기준보다 100배 작다.
		// 에디터의 Scale 1이 실제 월드 크기가 되도록 로드 시 보정한다.
		desc.PreTransformMatrix = XMMatrixScaling(100.f, 100.f, 100.f);

		if (FAILED(model->Load(desc)))
			return E_FAIL;
		if (model->GetAnimations().empty())
		{
			const _string failedName =
				"HOGWART_WORLD animation list is empty: " + folder;
			MessageBoxA(g_hWnd, failedName.c_str(),
				"System Error Message", MB_OK | MB_ICONERROR);
			return E_FAIL;
		}
	}

	if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Griff, CGriff::Create())))
	{
		MSG_BOX("HOGWART_WORLD Failed Prototype_GameObject_Griff");
		return E_FAIL;
	}
	if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_GriffChild, CGriffChild::Create())))
	{
		MSG_BOX("HOGWART_WORLD Failed Prototype_GameObject_GriffChild");
		return E_FAIL;
	}
	if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_WorldAnimal, CWorldAnimal::Create())))
	{
		MSG_BOX("HOGWART_WORLD Failed Prototype_GameObject_WorldWorldAgent");
		return E_FAIL;
	}
	if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_AnimatedWorldObject, CAnimatedWorldObject::Create())))
	{
		MSG_BOX("HOGWART_WORLD Failed Prototype_GameObject_AnimatedWorldObject");
		return E_FAIL;
	}
	return S_OK;
}
HRESULT CLevelHogwartWorldLoader::LoadCollsion_InWorker()
{	
	if (FAILED(E::CGameInstance::Get().AddPrototype(PX_COLLISION_PROXY_PROTOTYPE_GROUP, PROTO_GAMEOBJECT::Prototype_GameObject_Coin, CCoin::Create())))
	{
		MSG_BOX("TERRAIN Failed Prototype_GameObject_Coin");
		return E_FAIL;
	}
	return S_OK;
}

HRESULT CLevelHogwartWorldLoader::LoadWay_InWorker()
{
	auto* pWay = CGameInstance::Get().GetWayManager();
	if (nullptr == pWay)
	{
		MSG_BOX("WayLoad Failed in HogwartWorld");
		return E_FAIL;
	}
	if (auto res = CGameInstance::Get().AddResource("WAYPOINT", "PHONEIX", CResJson::Create("./Resources/json/WayPoint/Phoneix.json")))
	{
		if (FAILED(res->Load()))
		{
			MSG_BOX("LOAD FAILED PHONEIX JSON");
			return E_FAIL;
		}
		else//처음 매개변수는 json이름과 동일하게
			pWay->RegistWayTag("Phoneix", "WAYPOINT", "PHONEIX");
	}
	return S_OK;
}

HRESULT CLevelHogwartWorldLoader::LoadHogsmeade_ExtraAsset(){

	{
		if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::HOGWART_WORLD, "Static_WaterWheel_Resource",
			CResStaticModel::Create("./Resources/SampleClient/Models/Static/Hogsmeade_ExtraAsset/SM_CGY_WaterWheel.bin"))) {

			E::CResStaticModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f);

			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("HOGWART_WORLD Failed Static_WaterWheel Resource");
				//return false;
			}
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_WaterWheel, CWaterWheel::Create())))
		{
			MSG_BOX("HOGWART_WORLD Failed Prototype_GameObject_WaterWheel");
			return E_FAIL;
		}
	}
	return S_OK;
}
