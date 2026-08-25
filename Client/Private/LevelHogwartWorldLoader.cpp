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
#include "Griff.h"
#include "GriffChild.h"
#include "Troll.h"
#include "TrollWeapon.h"
#include "WorldAnimal.h"
// Client Terrain과 구분하기 위해 Engine Terrain 헤더를 명시한다.
#include "../../EngineSDK/Inc/Terrain.h"
#include "Water.h"

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
			if (FAILED(WorldAgentLoad_InWorker()))
				return false;

			if (FAILED(LoadCollsion_InWorker()))
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
				"./Resources/SampleClient/Textures/UI/UITexture/WandShop"
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

	}
}

HRESULT CLevelHogwartWorldLoader::NpcLoad_InWorker()
{
	struct NPC_MODEL_ENTRY { const char* pTag; const char* pCharacter; };
	static constexpr NPC_MODEL_ENTRY NpcModels[] =
	{
		{ "Model_Resource_NPC_VictorRookwood", "AesopSharp" },
		{ "Model_Resource_NPC_AlbieWeekes", "AlbieWeekes" },
		{ "Model_Resource_NPC_AnneSallow", "AnneSallow" },
		{ "Model_Resource_NPC_AugustusHill", "AugustusHill" },
		{ "Model_Resource_NPC_CrispinDunn", "CrispinDunn" },
		{ "Model_Resource_NPC_EffieBones", "EffieBones" },
		{ "Model_Resource_NPC_EleazarFig", "EleazarFig" },
		{ "Model_Resource_NPC_GladwinMoon", "GladwinMoon" },
		{ "Model_Resource_NPC_HelenThistlewood", "HelenThistlewood" },
		{ "Model_Resource_NPC_JasperTrout", "JasperTrout" },
		{ "Model_Resource_NPC_LeonaPeck", "LeonaPeck" },
		{ "Model_Resource_NPC_LeopoldBabcocke", "LeopoldBabcocke" },
		{ "Model_Resource_NPC_NoreenBlainey", "NoreenBlainey" },
		{ "Model_Resource_NPC_PadraicHaggarty", "PadraicHaggarty" },
		{ "Model_Resource_NPC_PercivalPippin", "PercivalPippin" },
		{ "Model_Resource_NPC_PhineasBlack", "PhineasBlack" },
		{ "Model_Resource_NPC_SironaRyan", "SironaRyan" },
		{ "Model_Resource_NPC_ThomasBrown", "ThomasBrown" },
		{ "Model_Resource_NPC_TimothyTeasdale", "TimothyTeasdale" },
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
	return S_OK;
}

HRESULT CLevelHogwartWorldLoader::WorldAgentLoad_InWorker()
{
	if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::HOGWART_WORLD, "Model_Resource_Griff",
		CResModel::Create("./Resources/SampleClient/Models/Skeleton/Griff/SK_Griff.bin"))) {

		E::CResModel::DESC pDesc{};
		pDesc.PreTransformMatrix = XMMatrixScaling(6.f,6.f,6.f)*XMMatrixRotationY(XMConvertToRadians(180.f));

		if (FAILED(res->Load(pDesc)))
		{
			MSG_BOX("HOGWART_WORLD Failed Model_Resource_Griff");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::HOGWART_WORLD, "Model_Resource_Cat",
		CResModel::Create("./Resources/SampleClient/Models/Skeleton/Cat/SK_Cat.bin"))) {

		E::CResModel::DESC pDesc{};
		pDesc.PreTransformMatrix = XMMatrixScaling(3.f, 3.f, 3.f) * XMMatrixRotationY(XMConvertToRadians(180.f));

		if (FAILED(res->Load(pDesc)))
		{
			MSG_BOX("HOGWART_WORLD Failed Model_Resource_Cat");
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
