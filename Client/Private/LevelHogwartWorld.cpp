#include "LevelHogwartWorld.h"
#include "SkyCloudyCube.h"
#include "pch.h"

#include "AnimatedObjectPlacementManager.h"
#include "AnimatedWorldObject.h"
#include "ComAnimator.h"
#include "FlyCamera.h"
#include "GameInstance.h"
#include "Griff.h"
#include "InteractiveNpc.h"
#include "ShopNpc.h"
#include "LevelHogwartWorldLoader.h"
#include "LightPlacementObject.h"
#include "Mon_Spawner.h"
#include "NpcPlacementData.h"
#include "NpcPlacementManager.h"
#include "NvClothCape.h"
#include "Player.h"
#include "ComCharacterMoveIntent.h"
#include "PlayerThirdPersonCamera.h"
#include "ResModel.h"
#include "Troll.h"
#include "UIController.h"
#include "UIManager.h"
#include "UiCamera.h"
#include "WorldAgent.h"
#include "InteractiveNpc.h"
#include "Griff.h"
#include "NpcPlacementData.h"
#include "NpcPlacementManager.h"
#include "Troll.h"
#include "PropBarrel.h"
#include "LightPlacementObject.h"
#include "PhysicsDoor.h"
#include "AccioBall.h"
#include "AccioActivity_Base.h"
#include "AccioActivity_Platform.h"
#include "AccioActivity_NpcController.h"
#include "AccioActivity_NpcCharacter.h"
#include "WorldAnimal.h"
#include "WorldNpc.h"
// Client에도 같은 이름의 Terrain.h가 있으므로 Engine SDK 헤더를 명시한다.
#include "../../EngineSDK/Inc/Terrain.h"
#include "Water.h"
#include "WaterWheel.h"
#include "WayPointManager.h"
NS_USING(Client)

CLevelHogwartWorld::CLevelHogwartWorld() : CLevel{ETOUI(LEVEL::HOGWART_WORLD)}
{
}

HRESULT CLevelHogwartWorld::Initialize()
{
	auto &gameInstance = E::CGameInstance::Get();
	gameInstance.GameObjectAllResetExceptLayers({"00_ENGINE_CINEMATIC_CAMERA"});

	if (FAILED(gameInstance.Initialize_EffectLight(15)))
		return E_FAIL;

	const auto hPlayer = SpawnPlayer();
	if (!hPlayer)
		return E_FAIL;
	m_hDebugPlayer = *hPlayer;
	if (auto* pNpcManager = gameInstance.GetNpcPlacementManager())
	{
		pNpcManager->ClearNpcOptions();
		pNpcManager->SetPickingQueryMask(ETOUI(COLLISION_LAYER::WORLD_STATIC));
		E::NPC_PLACEMENT_DESC NpcOption{};
		NpcOption.sPrototypeGroupTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
		NpcOption.sPrototypeTag = MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_WorldNpc);
		NpcOption.sLayerTag = "02_Npc";
		NpcOption.sModelGroupTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
		NpcOption.sModelResourceTag = "Model_Resource_NPC_VictorRookwood";
		NpcOption.sBehaviorMajorTag = "BTJSON";
		NpcOption.sBehaviorMinorTag = "NPC1";
		pNpcManager->RegisterNpcOption("World NPC", NpcOption);
		pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag,
											   "Victor Rookwood",
											   NpcOption.sModelGroupTag,
											   "Model_Resource_NPC_VictorRookwood",
											   "./Resources/SampleClient/Models/Skeleton/NPC_ViectorRookwood_lsy/");

		pNpcManager->RegisterBehaviorOption("World NPC", "BTJSON", "NPC1");
		{
			NpcOption.sPrototypeTag = MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_WorldAnimal);
			NpcOption.sLayerTag = "02_Animal";
			pNpcManager->RegisterNpcOption("Animal", NpcOption);
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "Spider",NpcOption.sModelGroupTag, "Model_Resource_Spider");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "Cat", NpcOption.sModelGroupTag, "Model_Resource_Cat","./Resources/SampleClient/Models/Skeleton/Cat/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "Bird_Kestrel", NpcOption.sModelGroupTag, "Model_Resource_Bird_Kestrel", "./Resources/SampleClient/Models/Skeleton/Birds_Kestrel/");
			
		}
		//{
		//	NpcOption.sPrototypeTag = MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_WorldAnimal);
		//	NpcOption.sLayerTag = "02_AnimObject";
		//	pNpcManager->RegisterNpcOption("AnimObject", NpcOption);
		//	pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag,
		//		"BalloonLauncher",
		//		NpcOption.sModelGroupTag,
		//		"Model_Resource_AnimatedObject_BalloonLauncher_Animated",
		//		"./Resources/SampleClient/Models/AnimatedObject/BalloonLauncher_Animated/");
		//
		//}
		pNpcManager->SetSpawnCallback(
			[hTarget = *hPlayer](const E::NPC_PLACEMENT_DESC &Placement)
			{
				if (Placement.sPrototypeTag != MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_WorldNpc) &&
					Placement.sPrototypeTag !=
						MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_WorldAnimal))
					return E::NPC_PLACEMENT_RESULT{Placement.iPlacementId, false, {}, "Unsupported Hogwarts NPC."};

				CWorldNpc::WORLD_AGENT_DESC Desc{};
				Desc.sObjectTag = "NpcPlacement_" + std::to_string(Placement.iPlacementId);
				Desc.TargetHandle = hTarget;
				Desc.LevelTag = Placement.sModelGroupTag;
				Desc.ReSourceTag = Placement.sModelResourceTag;
				Desc.BeHaviorTag = Placement.sBehaviorMinorTag;
				Desc.resBeHaviorMajor = Placement.sBehaviorMajorTag;
				Desc.resBeHaviorMinor = Placement.sBehaviorMinorTag;
				Desc.vPos = Placement.vPosition;
				Desc.vStartPos = Placement.vPatrolStartPosition;
				Desc.vEndPos = Placement.vPatrolEndPosition;
				Desc.vRot = Placement.vRotation;
				Desc.vScale = Placement.vScale;
				Desc.bDonMove = Placement.eRuntimeType == E::NPC_RUNTIME_TYPE::CPU_ACTOR_AMBIENT;
				Desc.fSpeed = Placement.fSpeed;
				Desc.bPhyx = Placement.bPhyx;
				Desc.AnimName = Placement.strAnimName;
				const auto hNpc = E::CGameInstance::Get().AddGameObjectToLayer(
					Placement.sPrototypeGroupTag, Placement.sPrototypeTag, Placement.sLayerTag, &Desc);
				if (!hNpc)
					return E::NPC_PLACEMENT_RESULT{Placement.iPlacementId, false, {}, "Spawn failed."};

				return E::NPC_PLACEMENT_RESULT{Placement.iPlacementId, true, *hNpc, "Spawn succeeded."};
			});
	}
	if (auto *pAnimatedManager = gameInstance.GetAnimatedObjectPlacementManager())
	{
		pAnimatedManager->ClearOptions();
		pAnimatedManager->SetPickingQueryMask(ETOUI(COLLISION_LAYER::WORLD_STATIC));
		static constexpr const char *AnimatedObjectModels[] = {
			"AnimatedGlobe_Animated",		 "BalloonLauncher_Animated",	 "CottonCandyDisplay_Animated",
			"DeathdayParty_Animated",		 "DragonBush_Animated",			 "EnchantedScarecrow_Animated",
			"EnchantedWateringCan_Animated", "HungryForRubbish_Animated",	 "LivingBooks_Animated",
			"MagicKiteBattle_Animated",		 "ManicStreetSigns_Animated",	 "MarionetteCandyBooth_Animated",
			"MirrorMirror_Animated",		 "NifflerTightropeToy_Animated", "OneManBand_Animated",
			"PaperAndQuill_Animated",		 "PlantParty_Animated",			 "PlayingWithFire_Animated",
			"RollUpRollUpCart_Animated",	 "SelfCheckingBooks_Animated",	 "SelfPruningTools_Animated",
			"SelfShufflingCards_Animated",	 "SelfWrappingPresent_Animated", "Snowman_Animated",
			"StirCrazyKitchen_Animated"};
		for (const char *modelName : AnimatedObjectModels)
		{
			E::ANIMATED_OBJECT_PLACEMENT_DESC option{};
			option.sPrototypeGroupTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
			option.sPrototypeTag = MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_AnimatedWorldObject);
			option.sLayerTag = "01_AnimatedObject";
			option.sModelGroupTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
			option.sModelResourceTag = "Model_Resource_AnimatedObject_" + _string{modelName};
			std::vector<_string> animationNames{};
			const std::filesystem::path animationFolder =
				std::filesystem::path{"./Resources/SampleClient/Models/AnimatedObject"} / modelName;
			if (std::filesystem::exists(animationFolder))
				for (const auto &entry : std::filesystem::directory_iterator(animationFolder))
					if (entry.is_regular_file() && entry.path().extension() == ".bin" &&
						entry.path().stem().string().starts_with("AN_"))
						animationNames.emplace_back(entry.path().filename().string());
			std::ranges::sort(animationNames);
			if (animationNames.empty())
				continue;
			option.sAnimationName = animationNames.front();
			pAnimatedManager->RegisterOption(modelName, option, animationNames);
		}
		pAnimatedManager->SetSpawnCallback(
			[](const E::ANIMATED_OBJECT_PLACEMENT_DESC &placement)
			{
				const _string tagPrefix = "Model_Resource_AnimatedObject_";
				if (!placement.sModelResourceTag.starts_with(tagPrefix))
					return E::ANIMATED_OBJECT_PLACEMENT_RESULT{
						placement.iPlacementId, false, {}, "Invalid animated-object resource tag."};
				if (!E::CGameInstance::Get().GetResourceFirst<E::CResModel>(placement.sModelGroupTag,
																			placement.sModelResourceTag))
				{
					return E::ANIMATED_OBJECT_PLACEMENT_RESULT{
						placement.iPlacementId,
						false,
						{},
						"AnimatedObject resource was not preloaded by the level loader."};
				}
				CAnimatedWorldObject::DESC desc{};
				desc.sObjectTag = "AnimatedObjectPlacement_" + std::to_string(placement.iPlacementId);
				desc.sModelGroupTag = placement.sModelGroupTag;
				desc.sModelResourceTag = placement.sModelResourceTag;
				desc.vPosition = placement.vPosition;
				desc.vRotation = placement.vRotation;
				// 원본 FBX가 미터 기준에서 한 번 더 0.01 배율로 export되어 정점 크기가
				// 0.001~0.03 수준이다. 에디터의 Scale 1을 월드 기준 크기로 보정한다.
				desc.vScale = {placement.vScale.x * 100.f, placement.vScale.y * 100.f, placement.vScale.z * 100.f};
				desc.sAnimationName = placement.sAnimationName;
				desc.bAutoPlay = placement.bAutoPlay;
				desc.bLoop = placement.bLoop;
				desc.fAnimationSpeed = placement.fAnimationSpeed;
				desc.fStartRatio = placement.fStartRatio;
				const auto handle = E::CGameInstance::Get().AddGameObjectToLayer(
					placement.sPrototypeGroupTag, placement.sPrototypeTag, placement.sLayerTag, &desc);
				if (!handle)
					return E::ANIMATED_OBJECT_PLACEMENT_RESULT{placement.iPlacementId, false, {}, "Spawn failed."};
				auto *object = E::CGameInstance::Get().GetGameObjectByHandleT<CAnimatedWorldObject>(*handle);
				if (!object)
					return E::ANIMATED_OBJECT_PLACEMENT_RESULT{
						placement.iPlacementId, false, {}, "Spawned object type mismatch."};
				return E::ANIMATED_OBJECT_PLACEMENT_RESULT{placement.iPlacementId, true, *handle, "Spawn succeeded."};
			});
		pAnimatedManager->SetTestCallback(
			[](const E::CHandle &handle,
			   const E::ANIMATED_OBJECT_PLACEMENT_DESC &placement,
			   E::CAnimatedObjectPlacementManager::TEST_COMMAND command)
			{
				auto *object = E::CGameInstance::Get().GetGameObjectByHandleT<CAnimatedWorldObject>(handle);
				if (!object)
					return false;
				switch (command)
				{
				case E::CAnimatedObjectPlacementManager::TEST_COMMAND::PLAY:
					return object->PlayAnimation(
						placement.sAnimationName, placement.bLoop, placement.fAnimationSpeed, placement.fStartRatio);
				case E::CAnimatedObjectPlacementManager::TEST_COMMAND::PAUSE:
					object->SetAnimationPaused(true);
					return true;
				case E::CAnimatedObjectPlacementManager::TEST_COMMAND::RESUME:
					object->SetAnimationPaused(false);
					return true;
				case E::CAnimatedObjectPlacementManager::TEST_COMMAND::STOP:
					object->StopAnimation();
					return true;
				case E::CAnimatedObjectPlacementManager::TEST_COMMAND::UPDATE_TRANSFORM:
					object->ApplyTransform(
						placement.vPosition,
						placement.vRotation,
						{
							placement.vScale.x * 100.f,
							placement.vScale.y * 100.f,
							placement.vScale.z * 100.f
						});
					return true;
				}
				return false;
			});
	}

	if (FAILED(SpawnPlayerCape(*hPlayer)))
		return E_FAIL;

	if (FAILED(gameInstance.LoadMap(CLevelHogwartWorldLoader::MAP_PATH, true)))
		return E_FAIL;

	if (FAILED(SpawnStaticCollision()))
		return E_FAIL;

	if (FAILED(SpawnTerrain(*hPlayer)))
		return E_FAIL;

	// [LSY] 호그와트 월드 상호작용 오브젝트 배치 좌표는 이 호출부에서 조정한다.
	if (FAILED(SpawnPhysicsDoor(
		{ 134.1f, 3.7f, -93.5f },
		{ 0.f, -60.f, 0.f },
		{ 0.9f, 0.8f, 1.f })))
	{
		return E_FAIL;
	}

	if (FAILED(SpawnPropBarrelPyramid()))
		return E_FAIL;

	{
		// 상점 NPC
		CShopNpc::DESC Desc{};
		Desc.sObjectTag = "Hogsmeade_ShopNpc_GerboldOllivander";
		Desc.LevelTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
		Desc.ReSourceTag = "Model_Resource_NPC_GerboldOllivander";
		Desc.BeHaviorTag = "NPC1";
		Desc.resBeHaviorMajor = "BTJSON";
		Desc.resBeHaviorMinor = "NPC1";
		Desc.TargetHandle = *hPlayer;
		// 인게임에서 잡은 첫 등장 위치. Shot_020의 루트 모션으로 작업대 쪽에 등장한다.
		Desc.vPos = { 108.5f, 2.5f, -82.f };
		Desc.vStartPos = Desc.vPos;
		Desc.vRot = { 0.f, 113.1f, 0.f };
		Desc.vScale = {1.f, 1.f, 1.f};
		Desc.fCCTHeight = 3.6f;
		Desc.fCCTRadius = 0.6f;
		Desc.fCCTStepOffset = 0.1f;
		Desc.vCCTCenterOffset = {0.f, 1.f, 0.f};
		Desc.bPhyx = true;
		Desc.bDonMove = true;
		Desc.SpeakerName = "거볼드 올리밴더";
		// 가게 출입문에서 카운터까지 거리를 포함해 입장 직후 자동 연출을 시작한다.
		Desc.InteractionDistance = 12.f;
		Desc.Repeatable = false;
		Desc.AutoStartOnEnter = true;
		Desc.FadeDuration = 0.6f;
		Desc.FadeHoldDuration = 0.25f;
		Desc.IdleExpressionAnim = "AN_BODY__Idle__Hu_BM_Idle_Casual_Loop.bin";
		Desc.WorldSpaceShop = false;
		Desc.RepositionPlayerForDialogue = false;

		Desc.Dialogue = {
			{
				"오, 샤프 교수님. 오셨군요.",
				"AN_BODY__Meeting__Shot_020_GerboldOllivander.bin",
				false,
				{},
				{},
				CInteractiveNpc::DIALOGUE_ACTION::NONE,
				false,
				true,
				"ShopNpcEntrance"
			},
			{
				"돈은 준비되셨죠?",
				"AN_BODY__DialogueTalk__HU_STN_STND_Conv_Talk.bin",
				true,
				{
					{ "예", 2, CInteractiveNpc::DIALOGUE_ACTION::NONE },
					{ "아니오", 2, CInteractiveNpc::DIALOGUE_ACTION::NONE }
				},
				{},
				CInteractiveNpc::DIALOGUE_ACTION::NONE,
				true,
				false,
				"ShopNpcDialogueCloseUp"
			},
			{
				"아, 돈이 없네요... 호그와트 성에 돈이 있다고 하던데, 그곳으로 가보시죠!",
				"AN_BODY__DialogueTalk__HU_STN_STND_Conv_Talk.bin",
				true,
				{},
				{},
				CInteractiveNpc::DIALOGUE_ACTION::MOVE_TO_DESTINATION
			}
		};
		// 기존 호그와트 쪽 액티비티 시작 지점을 이동 목적지로 사용한다.
		Desc.MoveDestination = {
			{ 1900.461f, 40.9f, 281.991f  }
		};
		Desc.MoveOutcomeAnimation =
			"AN_BODY__Meeting__Shot_180_GerboldOllivander.bin";
		Desc.MoveSpeed = 2.f;
		Desc.MoveStopDistance = 0.2f;

		if (!gameInstance.AddGameObjectToLayer(
				LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_ShopNpc, "02_Npc", &Desc))
			return E_FAIL;
	}

	{
		// 미니게임 NPC
		CInteractiveNpc::DESC Desc{};
		Desc.sObjectTag = "Hogsmeade_MiniGameNpc_Professor";
		Desc.LevelTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
		Desc.ReSourceTag = "PLAYER_MODEL_RESROUCE";
		Desc.BeHaviorTag = "NPC1";
		Desc.resBeHaviorMajor = "BTJSON";
		Desc.resBeHaviorMinor = "NPC1";
		Desc.TargetHandle = *hPlayer;
		Desc.vPos = { 1895.461f, 35.9f, 268.991f };
		Desc.vStartPos = Desc.vPos;
		Desc.vRot = {/*33.5f*/0.f, 24.5f, 0.f };
		Desc.vScale = { 1.f, 1.f, 1.f };
		Desc.fCCTHeight = 3.6f;
		Desc.fCCTRadius = 0.6f;
		Desc.fCCTStepOffset = 0.1f;
		Desc.vCCTCenterOffset = { 0.f, 1.f, 0.f };
		Desc.bPhyx = false;
		Desc.bDonMove = true;
		Desc.SpeakerName = "미니게임";
		Desc.InteractionDistance = 3.f;
		Desc.Repeatable = true;
		Desc.IdleExpressionAnim = "AN_ProfessorSharp_MasterRig_Hu_HUD_Idle_Casual_Loop_anm.bin";

		Desc.Dialogue = {// 0
						 {"어서 와, 대회에 참가하려고?",
						  "",
						  true,
						  {{"네!", 1, CInteractiveNpc::DIALOGUE_ACTION::CONTINUE_DIALOGUE},
						   {"다른 용무가 있습니다.", 3, CInteractiveNpc::DIALOGUE_ACTION::CANCEL_DIALOGUE}}},

			// 1
			{
				"그렇구나! 두 가지 종목이 있는데 어떤 것부터 시작해 볼래?",
				"",
				true,
				{
					{
						"소환사의 코트", 4,
						CInteractiveNpc::DIALOGUE_ACTION::CONTINUE_DIALOGUE
					},
					{
						"부릉! 브룸!", 2,
						CInteractiveNpc::DIALOGUE_ACTION::START_COIN_MINIGAME
					}
				}
			},

						 // 2
						 {"행운을 빌어.", "", true},

			// 3
			{ "곧 대회가 시작하니 늦기 전에 와야 해!", "", true },
			//4
			{ "알겠어, 뒤에 있는 경기장으로 가서 여학생에게 말을 걸어봐.", "", true },

		};

		Desc.ResolveStartDialogueIndex = []()
		{
			// 만약 플레이어가 이미 지팡이를 샀으면
			// 대화 6번으로
			// 아직 안샀으면 0번으로

			// auto* pPlayer = E::CGameInstance::Get().
			//	GetGameObjectByHandleT<CPlayer>(hDialoguePlayer);

			//
			return 0u;
		};
		// Facing direction (Y 38.342 degrees), approximately five metres ahead.
		Desc.MoveDestination = {
			{ 323.512f, 44.703f, 85.749f }, // 0: 아씨오
			{ 1953.605f, 60.391f, -188.274f } // 1: 코인
		};
		Desc.CoinMoveRotationEuler = { 2.034f, 13.171f, 0.f };
		Desc.MoveFadeHoldDuration = 10.f;
		Desc.MoveSpeed = 2.f;
		Desc.MoveStopDistance = 0.2f;

		if (!gameInstance.AddGameObjectToLayer(
				LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_MiniGameNpc, "02_Npc", &Desc))
			return E_FAIL;
	}
	if (FAILED(SpawnFlyCamera()) || FAILED(SpawnUICamera()) || FAILED(SpawnPlayerCamera(*hPlayer)))
	{
		return E_FAIL;
	}
	if (FAILED(SpawnLightPlacement()))
		return E_FAIL;

	if (FAILED(SpawnSkyBox()))
		return E_FAIL;

	// if (FAILED(SpawnWater()))
	//	return E_FAIL;

	if (FAILED(SpawnMonster(*hPlayer)))
		return E_FAIL;
	if (FAILED(SpawnNpcPlacements(*hPlayer, "./Resources/json/NPC/NpcSpawnIdle.json")))
		return E_FAIL;
	if (FAILED(SpawnNpcPlacements(*hPlayer, "./Resources/json/NPC/NpcSpawnWalk.json")))
		return E_FAIL;
	if (FAILED(SpawnNpcPlacements(*hPlayer, "./Resources/json/NPC/Cat.json")))
		return E_FAIL;
	if (FAILED(SpawnNpcPlacements(*hPlayer, "./Resources/json/NPC/RunSpider.json")))
		return E_FAIL;
	if (FAILED(SpawnNpcPlacements(*hPlayer, "./Resources/json/NPC/Birds.json")))
		return E_FAIL;

	if (FAILED(SpanwWorldAgent()))
		return E_FAIL;
	// gameInstance.Add_DirectionalLight({ 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, 10.f);

	if (FAILED(Initialize_VolumetricFog()))
		return E_FAIL;

	if (FAILED(Initialize_EnviromentLight()))
		return E_FAIL;
	if (FAILED(Initialize_LoopEffect()))
		return E_FAIL;

	{
		CWaterWheel::DESC desc{};
		desc.sObjectTag = "HOGWART_WORLD_WATERWHEEL";
		desc.vInitialPosition = _float3(478.4f, 92.577f, 322.32f);
		desc.vInitialRotation = _float3(0.f, 48.f, 0.f);
		desc.vInitialScale = _float3(3.f, 3.f, 3.f);
		if (!E::CGameInstance::Get().AddGameObjectToLayer(
				LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_WaterWheel, "Hogsmeade_WaterWheel", &desc))
			return E_FAIL;
	}

	// 레벨 진입 후 3초 동안 검은 화면을 유지하고,
	// 이후 2초 동안 검은 UI를 사라지게 해 게임 화면을 드러낸다.
	GET_SINGLE(UIManager)->CreateFadeOut(3.f, 2.f);

	return S_OK;
}

void CLevelHogwartWorld::Update(E::_float fTimeDelta)
{
	UNREFERENCED_PARAMETER(fTimeDelta);
	UpdateRuntimeActivitySpawnShortcut();

	if (!m_bCreatePlayScreenUI)
	{
		CGameObject::GAMEOBJECT_DESC desc{};
		desc.sObjectTag = "UIController";

		const auto hUIController = E::CGameInstance::Get().AddGameObjectToLayer(
			"LEVEL_HOGWART_WORLD", "Prototype_GameObject_UIController", "UIController", &desc);
		GET_SINGLE(UIManager)->SetUIController(hUIController);
		m_bCreatePlayScreenUI = hUIController.has_value();
	}

	GET_SINGLE(UIManager)->UpdateRootUIHandles();
	UpdateDebugWarp();
}

void CLevelHogwartWorld::UpdateDebugWarp()
{
	auto& gameInstance = CGameInstance::Get();
	if (!gameInstance.KeyPressing(DIK_LSHIFT) ||
		!gameInstance.KeyDown(DIK_F9))
	{
		return;
	}

	auto* pPlayer = gameInstance.GetGameObjectByHandleT<CPlayer>(m_hDebugPlayer);
	if (!pPlayer)
	{
		DEBUG_LOG("[HogwartWorld] Debug warp failed: Player is invalid.\n");
		return;
	}

	auto* pMoveIntent = pPlayer->GetComponent<CComCharacterMoveIntent>(
		"ComCharacterMoveIntent");
	if (!pMoveIntent)
	{
		DEBUG_LOG("[HogwartWorld] Debug warp failed: MoveIntent is missing.\n");
		return;
	}

	constexpr _float3 vHogwartsCastlePosition{ 1890.f, 42.f, 245.f };
	pMoveIntent->RequestWarp(vHogwartsCastlePosition);
	DEBUG_LOG("[HogwartWorld] Debug warp: Hogwarts Castle (Shift + F9).\n");
}

HRESULT CLevelHogwartWorld::Render()
{
	return S_OK;
}

void CLevelHogwartWorld::UpdateGUI()
{
}

void CLevelHogwartWorld::UpdateRuntimeActivitySpawnShortcut()
{
	auto& gameInstance = CGameInstance::Get();
	const _bool bShiftPressed =
		gameInstance.KeyPressing(DIK_LSHIFT) ||
		gameInstance.KeyPressing(DIK_RSHIFT);
	if (!bShiftPressed)
		return;

	if (gameInstance.KeyDown(DIK_F10))
	{
		PruneInvalidRuntimeHandles(m_AccioActivityHandles);
		//PruneInvalidRuntimeHandles(m_CoinCollisionHandles);

		const HRESULT hrAccio = m_AccioActivityHandles.empty()
			? SpawnAccioActivity(
				m_hDebugPlayer,
				{ 1912.f, 24.8f, 316.f },
				180.f,
				1.f)
			: S_FALSE;
		//const HRESULT hrCoin = m_CoinCollisionHandles.empty()
		//	? SpawnCoinCollision()
		//	: S_FALSE;

		if (FAILED(hrAccio) /*|| FAILED(hrCoin)*/)
		{
			DespawnRuntimeObjects(m_AccioActivityHandles);
			//DespawnRuntimeObjects(m_CoinCollisionHandles);
			DEBUG_LOG("[HogwartWorld] Shift + F10 runtime activity spawn failed.\n");
		}
		else
		{
			DEBUG_LOG("[HogwartWorld] Shift + F10 spawned Accio Activity and Coin Collision.\n");
		}
	}
	else if (gameInstance.KeyDown(DIK_F11))
	{
		DespawnRuntimeObjects(m_AccioActivityHandles);
		DespawnRuntimeObjects(m_CoinCollisionHandles);
		DEBUG_LOG("[HogwartWorld] Shift + F11 despawned Accio Activity and Coin Collision.\n");
	}
}

void CLevelHogwartWorld::DespawnRuntimeObjects(
	std::vector<CHandle>& Handles)
{
	for (auto iter = Handles.rbegin(); iter != Handles.rend(); ++iter)
	{
		if (auto* pObject = CGameInstance::Get().GetGameObjectByHandle(*iter))
			pObject->SetPendingDestroyCascade();
	}
	Handles.clear();
}

void CLevelHogwartWorld::PruneInvalidRuntimeHandles(
	std::vector<CHandle>& Handles)
{
	std::erase_if(
		Handles,
		[](const CHandle& hObject)
		{
			return CGameInstance::Get().GetGameObjectByHandle(hObject) == nullptr;
		});
}

std::optional<CHandle> CLevelHogwartWorld::SpawnPlayer()
{
	CPlayer::DESC desc{};
	desc.sObjectTag = "Player";
	desc.vInitialPosition = { 64.f, -18.f, -378.f };
	desc.LevelTag = LEVEL::HOGWART_WORLD;
	desc.tFilter = PX_FILTER_DESC{
		.iLayer = ETOUI(COLLISION_LAYER::PLAYER_BODY),
		.iSimulationMask = PX_ALL_LAYERS,
		.iQueryMask =
			ETOUI(COLLISION_LAYER::WORLD_STATIC) |
			ETOUI(COLLISION_LAYER::MOVING_PLATFORM) |
			ETOUI(COLLISION_LAYER::DOOR_DYNAMIC) |
			ETOUI(COLLISION_LAYER::DOOR_HINGE_BLOCKER)
	};

	return E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Player, "03_Player", &desc);
}

HRESULT CLevelHogwartWorld::SpawnPhysicsDoor(
	const _float3& vPosition,
	const _float3& vRotationEulerDegrees,
	const _float3& vScale)
{
	CPhysicsDoor::DESC desc{};
	desc.sObjectTag = "HogwartWorld_PhysicsDoor";
	desc.sModelResourceGroup = LEVEL::HOGWART_WORLD;
	desc.vInitialPosition = vPosition;
	desc.vInitialRotation = vRotationEulerDegrees;
	desc.vInitialScale = vScale;
	// [LSY] 월드 배치 문은 복귀 힘을 낮추고 감쇠를 높여 천천히 닫히게 한다.
	desc.fTwistDriveStiffness = 220.f;
	desc.fTwistDriveDamping = 100.f;

	return CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_PhysicsDoor,
		"01_PhysicsInteraction",
		&desc)
		? S_OK
		: E_FAIL;
}

HRESULT CLevelHogwartWorld::SpawnAccioActivity(
	CHandle hPlayer,
	const _float3& vOrigin,
	_float fYawDegrees,
	_float fUniformScale)
{
	if (fUniformScale <= 0.f)
		return E_INVALIDARG;
	if (!m_AccioActivityHandles.empty())
		return S_FALSE;

	const _float3 vSetRotation{ 0.f, fYawDegrees, 0.f };
	const _matrix setWorld =
		XMMatrixRotationY(XMConvertToRadians(fYawDegrees)) *
		XMMatrixTranslation(vOrigin.x, vOrigin.y, vOrigin.z);
	const auto makeWorldPosition = [&setWorld](const _float3& vLocalPosition)
	{
		_float3 vWorldPosition{};
		XMStoreFloat3(
			&vWorldPosition,
			XMVector3TransformCoord(
				XMLoadFloat3(&vLocalPosition),
				setWorld));
		return vWorldPosition;
	};
	const auto scaleVector = [fUniformScale](const _float3& value)
	{
		return _float3{
			value.x * fUniformScale,
			value.y * fUniformScale,
			value.z * fUniformScale
		};
	};
	const auto scaleBox = [&scaleVector](
		ACCIO_ACTIVITY_BOX_COLLIDER_DESC& box)
	{
		box.vHalfExtents = scaleVector(box.vHalfExtents);
		box.vLocalOffset = scaleVector(box.vLocalOffset);
	};

	std::vector<CHandle> spawnedHandles{};
	spawnedHandles.reserve(11);
	// [LSY] 세트 생성은 전부 성공했을 때만 유효하다. 중간 실패 시 이미
	// 생성된 구성요소를 역순으로 정리하고 실패 코드를 반환한다.
	const auto rollbackSpawnedObjects = [&spawnedHandles]() -> HRESULT
	{
		for (auto iter = spawnedHandles.rbegin();
			iter != spawnedHandles.rend();
			++iter)
		{
			if (auto* pObject = CGameInstance::Get().GetGameObjectByHandle(*iter))
				pObject->SetPendingDestroyCascade();
		}
		return E_FAIL;
	};

	CAccioActivity_Base::DESC baseDesc{};
	baseDesc.sObjectTag = "HogwartWorld_AccioActivity_Base";
	baseDesc.sResourceGroup = LEVEL::HOGWART_WORLD;
	baseDesc.vInitialPosition = vOrigin;
	baseDesc.vInitialRotation = vSetRotation;
	baseDesc.vInitialScale = {
		fUniformScale,
		fUniformScale,
		fUniformScale
	};
	for (auto& box : baseDesc.BoxColliders)
		scaleBox(box);
	scaleBox(baseDesc.Score10Trigger);
	scaleBox(baseDesc.Score20Trigger);
	scaleBox(baseDesc.Score30Trigger);
	scaleBox(baseDesc.Score50Trigger);
	const auto hBase = CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_Base,
		"01_PhysicsInteraction",
		&baseDesc);
	if (!hBase)
		return E_FAIL;
	spawnedHandles.push_back(*hBase);

	CAccioActivity_Platform::DESC platformDesc{};
	platformDesc.sObjectTag = "HogwartWorld_AccioActivity_Platform";
	platformDesc.sResourceGroup = LEVEL::HOGWART_WORLD;
	platformDesc.vInitialPosition = vOrigin;
	platformDesc.vInitialRotation = vSetRotation;
	platformDesc.vInitialScale = {
		fUniformScale,
		fUniformScale,
		fUniformScale
	};
	scaleBox(platformDesc.BoxCollider);
	platformDesc.WedgeCollider.vScale =
		scaleVector(platformDesc.WedgeCollider.vScale);
	platformDesc.WedgeCollider.vLocalOffset =
		scaleVector(platformDesc.WedgeCollider.vLocalOffset);
	scaleBox(platformDesc.NpcMoveAreaTrigger);
	const auto hPlatform = CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_Platform,
		"01_PhysicsInteraction",
		&platformDesc);
	if (!hPlatform)
	{
		return rollbackSpawnedObjects();
	}
	spawnedHandles.push_back(*hPlatform);

	struct ACCIO_BALL_PLACEMENT
	{
		const _char* pObjectTag;
		const _char* pResourceTag;
		CAccioBall::COLOR eColor;
		_float3 vLocalPosition;
	};

	constexpr ACCIO_BALL_PLACEMENT ballPlacements[] =
	{
		{ "HogwartWorld_AccioBall_Blue_1", "Static_AccioBall_Blue_Resource", CAccioBall::COLOR::BLUE, { -7.f, 4.25f, 26.f } },
		{ "HogwartWorld_AccioBall_Red_1", "Static_AccioBall_Red_Resource", CAccioBall::COLOR::RED, { -4.f, 4.25f, 26.f } },
		{ "HogwartWorld_AccioBall_Blue_2", "Static_AccioBall_Blue_Resource", CAccioBall::COLOR::BLUE, { -1.f, 4.25f, 26.f } },
		{ "HogwartWorld_AccioBall_Red_2", "Static_AccioBall_Red_Resource", CAccioBall::COLOR::RED, { 2.f, 4.25f, 26.f } },
		{ "HogwartWorld_AccioBall_Blue_3", "Static_AccioBall_Blue_Resource", CAccioBall::COLOR::BLUE, { 5.f, 4.25f, 26.f } },
		{ "HogwartWorld_AccioBall_Red_3", "Static_AccioBall_Red_Resource", CAccioBall::COLOR::RED, { 8.f, 4.25f, 26.f } }
	};

	std::array<CHandle, std::size(ballPlacements)> ballHandles{};
	for (size_t i = 0; i < std::size(ballPlacements); ++i)
	{
		const auto& placement = ballPlacements[i];
		CAccioBall::DESC ballDesc{};
		ballDesc.sObjectTag = placement.pObjectTag;
		ballDesc.sResourceGroup = LEVEL::HOGWART_WORLD;
		ballDesc.sModelResourceTag = placement.pResourceTag;
		ballDesc.eColor = placement.eColor;
		ballDesc.vInitialPosition = makeWorldPosition(
			scaleVector(placement.vLocalPosition));
		ballDesc.vInitialRotation = vSetRotation;
		ballDesc.vInitialScale = {
			3.f * fUniformScale,
			3.f * fUniformScale,
			3.f * fUniformScale
		};
		ballDesc.fSphereRadius *= fUniformScale;

		const auto hBall = CGameInstance::Get().AddGameObjectToLayer(
			LEVEL::HOGWART_WORLD,
			PROTO_GAMEOBJECT::Prototype_GameObject_AccioBall,
			"02_AccioBall",
			&ballDesc);
		if (!hBall)
		{
			return rollbackSpawnedObjects();
		}

		ballHandles[i] = *hBall;
		spawnedHandles.push_back(*hBall);
	}

	auto* pActivity = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_Base>(*hBase);
	if (!pActivity)
	{
		return rollbackSpawnedObjects();
	}
	for (const auto& hBall : ballHandles)
	{
		if (!pActivity->RegisterBall(hBall))
		{
			return rollbackSpawnedObjects();
		}
	}

	CAccioActivity_NpcCharacter::DESC npcCharacterDesc{};
	CAccioActivity_NpcController::DESC npcControllerDesc{};
	npcControllerDesc.sObjectTag = "HogwartWorld_AccioActivity_NpcController";
	npcControllerDesc.hActivity = *hBase;
	npcControllerDesc.hPlatform = *hPlatform;
	npcControllerDesc.hInteractionPlayer = hPlayer;
	npcControllerDesc.SpeakerName = "호그와트 학생";
	npcControllerDesc.fInteractionDistance = 10.f;
	npcControllerDesc.Dialogue =
	{
		{ "준비됐어? 아씨오로 공을 끌어 점수를 겨뤄 보자.", {}, true },
		{ "높은 점수 구역에 공을 멈추면 이겨. 네가 먼저 시작해.", {}, true }
	};
	npcControllerDesc.PlayerWinDialogue =
	{
		{ "잘했어. 이번 승부는 네가 이겼네.", {}, true }
	};
	npcControllerDesc.NpcWinDialogue =
	{
		{ "이번 승부는 내가 이겼네. 다시 도전해 봐.", {}, true }
	};
	npcControllerDesc.DrawDialogue =
	{
		{ "무승부네. 꽤 좋은 승부였어.", {}, true }
	};

	constexpr _float fNpcSpawnClearance = 0.5f;
	const _float fCCTBottomFromObjectOrigin =
		npcCharacterDesc.vCCTCenterOffset.y -
		(npcCharacterDesc.fCCTHeight * 0.5f + npcCharacterDesc.fCCTRadius);
	const _float fNpcSideLocalX =
		platformDesc.NpcMoveAreaTrigger.vLocalOffset.x +
		std::max(
			platformDesc.NpcMoveAreaTrigger.vHalfExtents.x -
			npcControllerDesc.fMoveAreaMargin -
			npcControllerDesc.fSideStandbyInset,
			0.f);
	const _float3 vNpcLocalPosition{
		fNpcSideLocalX,
		platformDesc.BoxCollider.vLocalOffset.y +
		platformDesc.BoxCollider.vHalfExtents.y +
		fNpcSpawnClearance - fCCTBottomFromObjectOrigin,
		platformDesc.NpcMoveAreaTrigger.vLocalOffset.z
	};
	npcControllerDesc.vInitialPosition = makeWorldPosition(vNpcLocalPosition);
	npcControllerDesc.vInitialRotation = {
		vSetRotation.x,
		vSetRotation.y - 90.f,
		vSetRotation.z
	};

	npcCharacterDesc.sObjectTag = "HogwartWorld_AccioActivity_NpcCharacter";
	npcCharacterDesc.sResourceGroup =
		_string{ MagicEnumToStringView(LEVEL::HOGWART_WORLD) };
	npcCharacterDesc.sModelResourceTag =
		"ACCIO_ACTIVITY_STUDENT_MODEL_RESOURCE";
	npcCharacterDesc.vInitialPosition = npcControllerDesc.vInitialPosition;
	npcCharacterDesc.vInitialRotation = npcControllerDesc.vInitialRotation;
	const auto hNpcCharacter = CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_NpcCharacter,
		"02_Npc",
		&npcCharacterDesc);
	if (!hNpcCharacter)
	{
		return rollbackSpawnedObjects();
	}
	spawnedHandles.push_back(*hNpcCharacter);

	auto* pNpcCharacter = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_NpcCharacter>(*hNpcCharacter);
	if (!pNpcCharacter || pNpcCharacter->GetWeaponHandle() == CHandle{})
	{
		return rollbackSpawnedObjects();
	}
	spawnedHandles.push_back(pNpcCharacter->GetWeaponHandle());

	npcControllerDesc.hNpcCharacter = *hNpcCharacter;
	const auto hNpcController = CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_AccioActivity_NpcController,
		"02_Npc",
		&npcControllerDesc);
	if (!hNpcController)
	{
		return rollbackSpawnedObjects();
	}
	spawnedHandles.push_back(*hNpcController);

	pActivity->SetParticipantHandle(
		CAccioActivity_Base::PARTICIPANT::PLAYER,
		hPlayer);
	auto* pNpcController = CGameInstance::Get().
		GetGameObjectByHandleT<CAccioActivity_NpcController>(*hNpcController);
	if (!pNpcController)
		return rollbackSpawnedObjects();

	pNpcController->SetInteractionPlayerHandle(hPlayer);
	m_AccioActivityHandles = std::move(spawnedHandles);

	return S_OK;
}

HRESULT CLevelHogwartWorld::SpawnLightPlacement()
{
	CLightPlacementObject::DESC desc{};
	desc.sObjectTag = "HogwartWorldLightPlacement";
	desc.sLightFileName = "Level_HogwartWorld";

	return CGameInstance::Get().AddGameObjectToLayer(ES_EngineProtoMajorType::PERMANENT,
													 ES_EngineProtoGameObject::Prototype_GameObject_LightPlacement,
													 "Layer_LightPlacement",
													 &desc)
			   ? S_OK
			   : E_FAIL;
}
HRESULT CLevelHogwartWorld::SpawnPlayerCape(CHandle hPlayer)
{
	CNvClothCape::DESC desc{};
	desc.sObjectTag = "NvClothCape";
	desc.hTarget = hPlayer;
	desc.sResourceGroup = LEVEL::HOGWART_WORLD;
	desc.sModelResourceTag = "PLAYER_CAPE_MODEL_RESOURCE";
	desc.sClothMeshResourceTag = "PLAYER_CAPE_CLOTH_RESOURCE";
	desc.sTargetModelComponentTag = "ComCModelIntance";
	desc.sAttachBoneName = "Spine3";
	desc.vLocalPosition = {0.05f, 0.08f, 0.f};

	E::CGameInstance::Get().JsonDeSerialize("./Resources/NvCloth/CollisionRigs/ProfessorCape.nvclothcollision.json",
											desc.tBodyCollisionRig,
											E::NVCLOTH_COLLISION_RIG_ROOT,
											false);
	E::CGameInstance::Get().JsonDeSerialize(
		"./Resources/NvCloth/CollisionRigs/ProfessorCape_Broom.nvclothcollision.json",
		desc.tBroomBodyCollisionRig,
		E::NVCLOTH_COLLISION_RIG_ROOT,
		false);
	E::CGameInstance::Get().JsonDeSerialize(
		"./Resources/NvCloth/CollisionRigs/ProfessorCape_BroomObject.nvclothcollision.json",
		desc.tBroomObjectCollisionRig,
		E::NVCLOTH_COLLISION_RIG_ROOT,
		false);

	if (auto hCape = E::CGameInstance::Get().AddGameObjectToLayer(
			LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_NvClothCape, "03_Player", &desc))
	{
		if (!hCape)
			return E_FAIL;

		if (auto pPlayer = CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(hPlayer))
		{
			pPlayer->SetCapeHandle(hCape.value());
		}
	}

	return S_OK;
}

HRESULT CLevelHogwartWorld::SpawnPropBarrelPyramid()
{
	// 전달받은 위치를 바닥 줄의 오른쪽 끝으로 두고 왼쪽으로 펼친다.
	const _float3 vRightBasePosition{ 273.554f, 38.809f, 130.496f };
	// 배럴을 세운 크기에 맞춰 초기 Convex가 서로 겹치지 않게 여유를 둔다.
	const _float fHorizontalSpacing = 4.4f;
	const _float fVerticalSpacing = 4.6f;
	const uint32_t iRowCounts[]{ 5u, 3u, 1u };
	const _float fPyramidCenterX =
		vRightBasePosition.x - fHorizontalSpacing * 2.f;

	uint32_t iBarrelIndex{};
	for (uint32_t iRow = 0; iRow < std::size(iRowCounts); ++iRow)
	{
		const uint32_t iBarrelCount = iRowCounts[iRow];
		const _float fHalfRow =
			static_cast<_float>(iBarrelCount - 1u) * 0.5f;

		for (uint32_t iColumn = 0; iColumn < iBarrelCount; ++iColumn)
		{
			CPropBarrel::DESC Desc{};
			Desc.sObjectTag =
				"Hogwart_PropBarrel_Pyramid_" + std::to_string(iBarrelIndex++);
			Desc.sResourceGroup = "PERMANENT";
			Desc.vInitialRotation = { 90.f, 0.f, 0.f };
			// 위쪽 배럴이 자리를 잡는 동안 발생하는 초기 접촉으로는 파괴하지 않는다.
			Desc.fCollisionDestroyGraceTime = 2.f;
			Desc.vInitialPosition =
			{
				fPyramidCenterX +
					(static_cast<_float>(iColumn) - fHalfRow) * fHorizontalSpacing,
				vRightBasePosition.y + static_cast<_float>(iRow) * fVerticalSpacing,
				vRightBasePosition.z
			};

			if (!CGameInstance::Get().AddGameObjectToLayer(
				"PERMANENT",
				PROTO_GAMEOBJECT::Prototype_GameObject_PropBarrel,
				"PropBarrel",
				&Desc))
			{
				return E_FAIL;
			}
		}
	}

	return S_OK;
}

HRESULT CLevelHogwartWorld::SpawnTerrain(std::optional<CHandle> hPlayer)
{
	E::CTerrain::DESC desc{};
	desc.sObjectTag = "HogwartWorldTerrain";
	desc.textureGroup = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
	desc.textureTag = "TEX2D_Terrain_Tile0";
	desc.tPhysicsFilter = PX_FILTER_DESC{
		.iLayer = ETOUI(COLLISION_LAYER::WORLD_STATIC), .iSimulationMask = PX_ALL_LAYERS, .iQueryMask = PX_ALL_LAYERS};

	const auto hTerrain = E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Terrain, "01_Terrain", &desc);
	if (!hTerrain)
		return E_FAIL;

	auto *terrain = E::CGameInstance::Get().GetGameObjectByHandleT<E::CTerrain>(*hTerrain);
	if (!terrain)
		return E_FAIL;

	if (FAILED(terrain->LoadTerrain(CLevelHogwartWorldLoader::TERRAIN_PATH, hPlayer)))
		return E_FAIL;

	return SpawnNaviMesh(terrain);
}

HRESULT CLevelHogwartWorld::SpawnNaviMesh(E::CTerrain *pTerrain)
{
	if (nullptr == pTerrain)
		return E_FAIL;

	auto *pNavMesh = CGameInstance::Get().GetNavMeshManager();

	if (nullptr == pNavMesh)
		return E_FAIL;

	const std::string strPath =
		(std::filesystem::path(CLevelHogwartWorldLoader::MAP_PATH) / "navmesh.json").generic_string();

	// 에디터에서 지정한 Blocked 영역 불러오기
	if (FAILED(pNavMesh->Load(strPath)))
		return E_FAIL;

	std::vector<_float3> Vertices{};
	Vertices.reserve(pTerrain->GetVertices().size());

	const _matrix TerrainWorld = pTerrain->GetTransform().GetLoadedCombinedWorldMatrix();

	// Terrain 정점은 로컬 좌표이므로 월드 좌표로 변환
	for (const auto &Vertex : pTerrain->GetVertices())
	{
		_float3 vWorldPosition{};

		XMStoreFloat3(&vWorldPosition, XMVector3TransformCoord(XMLoadFloat3(&Vertex.pos), TerrainWorld));

		Vertices.push_back(vWorldPosition);
	}

	NAVMESH_BUILD_DESC Desc{};

	// 기존 0.3이면 약 5500 × 4000 셀이라 너무 큼
	Desc.cellSize = 2.f;
	Desc.cellHeight = 0.5f;

	Desc.agentHeight = 2.f;
	Desc.agentRadius = 0.f;
	Desc.agentMaxClimb = 0.6f;
	Desc.agentMaxSlope = 45.f;

	// if (!pNavMesh->Build(
	//	Vertices,
	//	pTerrain->GetIndices(),
	//	Desc))
	//{
	//	return E_FAIL;
	// }
	NAVMESH_BUILD_DESC StaticNaviDesc{};
	if (FAILED(pNavMesh->Load(strPath, &StaticNaviDesc)))
		return E_FAIL;

	// 내비메시가 저장되지 않은 맵도 레벨 진입은 허용한다.
	// 이전 레벨의 내비메시가 남지 않도록 런타임 데이터는 비운다.
	if (pNavMesh->GetManualTriangles().empty())
	{
		pNavMesh->Clear();
		return S_OK;
	}

	if (!pNavMesh->BuildManual(StaticNaviDesc))
		return E_FAIL;
	return S_OK;
}

HRESULT CLevelHogwartWorld::SpawnFlyCamera()
{
	E::CCameraObject::CAMERA_DESC desc{};
	desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
	desc.vAt = {200.f, 48.f, 80.f};
	desc.vEye = {200.f, 65.f, 60.f};
	desc.fAspect = g_iWinSizeX / static_cast<E::_float>(g_iWinSizeY);
	desc.fFovY = 75.f;
	desc.fNear = 0.1f;
	desc.fFar = 3000.f;
	desc.sObjectTag = "FlyCam";

	const auto hCamera =
		E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_FlyCamera", "99_CAMERA", &desc);
	return hCamera && SUCCEEDED(E::CGameInstance::Get().RegistCamera("FLY", *hCamera)) ? S_OK : E_FAIL;
}

HRESULT CLevelHogwartWorld::SpawnUICamera()
{
	E::CCameraObject::CAMERA_DESC desc{};
	desc.eProj = E::CCameraObject::PROJ::ORTHOGRAPHIC;
	desc.fNear = 0.f;
	desc.fFar = 1.f;
	desc.fWidth = g_iWinSizeX;
	desc.fHeight = g_iWinSizeY;
	desc.sObjectTag = "UICam";
	desc.vEye = {0.f, 0.f, -0.1f};

	const auto hCamera =
		E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_UICamera", "99_CAMERA", &desc);
	return hCamera && SUCCEEDED(E::CGameInstance::Get().RegistCamera("UI", *hCamera)) ? S_OK : E_FAIL;
}

HRESULT CLevelHogwartWorld::SpawnPlayerCamera(CHandle hPlayer)
{
	CPlayerThirdPersonCamera::DESC desc{};
	desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
	desc.vAt = { 64.f, -18.f, -378.f };
	desc.vEye = { 64.f, -15.f, -385.f };
	desc.fAspect = g_iWinSizeX / static_cast<E::_float>(g_iWinSizeY);
	desc.fFovY = 75.f;
	desc.fNear = 0.1f;
	desc.fFar = 3000.f;
	desc.sObjectTag = "PlayerCamera";
	desc.hTarget = hPlayer;

	const auto hCamera = E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerThirdPersonCamera, "101_CAMERA", &desc);
	if (!hCamera || FAILED(E::CGameInstance::Get().RegistCamera("PlayerCamera", *hCamera)))
		return E_FAIL;

	E::CGameInstance::Get().SetActiveCamera("PlayerCamera");
	return S_OK;
}

HRESULT CLevelHogwartWorld::SpawnSkyBox()
{
	CSkyCloudyCube::SKY_DESC skyDesc{};
	skyDesc.sObjectTag = "SkyCloudyCube";
	if (!CGameInstance::Get().AddGameObjectToLayer(
			"PERMANENT", "Prototype_GameObject_SkyCloudyCube", "00_SKYBOX", &skyDesc))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CLevelHogwartWorld::SpawnWater()
{
	E::CWater::WATER_DESC desc{};
	desc.sObjectTag = "HogwartWorldOcean";
	desc.vPosition = {200.f, -75.f, 80.f};
	desc.vSize = {8000.f, 8000.f};
	desc.vWaterColor = {0.012f, 0.055f, 0.16f, 0.9f};
	desc.vShallowColor = {0.018f, 0.10f, 0.24f, 1.f};
	desc.vDeepColor = {0.002f, 0.012f, 0.055f, 1.f};
	desc.vReflectionColor = {0.10f, 0.20f, 0.38f, 1.f};
	desc.fUVScale = 0.018f;
	desc.fSecondaryNormalScale = 2.7f;
	desc.fWaveIntensity = 0.95f;
	desc.fFollowSnap = 50.f;
	desc.bFollowCamera = true;

	return E::CGameInstance::Get().AddGameObjectToLayer(
			   LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Water, "00_WATER", &desc)
			   ? S_OK
			   : E_FAIL;
}

HRESULT CLevelHogwartWorld::SpawnMonster(std::optional<CHandle> hPlayer)
{
	CMon_Spawner::MON_SPAWNER_DESC MonS{};
	MonS.sObjectTag = "MonSpawn";
	MonS.handle = hPlayer.value();
	if (!CGameInstance::Get().AddGameObjectToLayer(
			LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_MonSpawner, "00.MonSpawn", &MonS))
	{
		return E_FAIL;
	}

}
HRESULT CLevelHogwartWorld::SpawnStaticCollision()
{
	auto handles = CGameInstance::Get().GetPhysXManager()->CreateCollisionProxyObjectsFromFile("Level_HogwartWorld",
																							   "00_MapCollision");

	if (handles.empty())
		return E_FAIL;

	return S_OK;
}

HRESULT CLevelHogwartWorld::SpawnCoinCollision()
{
	if (!m_CoinCollisionHandles.empty())
		return S_FALSE;

	auto handles = CGameInstance::Get()
		.GetPhysXManager()
		->CreateCollisionProxyObjectsFromFile(
			"Level_HogwartCoin",
			"00_CoinCollision");

	if (handles.empty())
		return E_FAIL;
	m_CoinCollisionHandles = std::move(handles);
	return S_OK;
}

HRESULT CLevelHogwartWorld::SpawnNpcPlacements(CHandle hPlayer, const _string &Path)
{
	E::NPC_PLACEMENT_FILE File{};
	if (FAILED(E::CGameInstance::Get().JsonDeSerialize(Path, File, "NpcPlacements")))
		return E_FAIL;

	if (File.iVersion != 1)
		return E_FAIL;

	for (const auto &Placement : File.Placements)
	{
		if (Placement.eRuntimeType == E::NPC_RUNTIME_TYPE::GPU_CROWD_AMBIENT)
			return E_NOTIMPL;

		CWorldNpc::WORLD_AGENT_DESC Desc{};
		Desc.sObjectTag = Placement.sPrototypeTag;
		Desc.TargetHandle = hPlayer;
		Desc.LevelTag = Placement.sModelGroupTag.empty() ? Placement.sPrototypeGroupTag : Placement.sModelGroupTag;
		Desc.ReSourceTag = Placement.sModelResourceTag;
		Desc.BeHaviorTag = Placement.sBehaviorMinorTag;
		Desc.resBeHaviorMajor = Placement.sBehaviorMajorTag;
		Desc.resBeHaviorMinor = Placement.sBehaviorMinorTag;
		Desc.vPos = Placement.vPosition;
		Desc.vStartPos = Placement.vPatrolStartPosition;
		Desc.vEndPos = Placement.vPatrolEndPosition;
		Desc.vRot = Placement.vRotation;
		Desc.vScale = Placement.vScale;
		Desc.fSpeed = Placement.fSpeed;
		Desc.bPhyx = Placement.bPhyx;
		Desc.AnimName = Placement.strAnimName;
		Desc.bDonMove = Placement.eRuntimeType == E::NPC_RUNTIME_TYPE::CPU_ACTOR_AMBIENT;

		if (!E::CGameInstance::Get().AddGameObjectToLayer(
				Placement.sPrototypeGroupTag, Placement.sPrototypeTag, Placement.sLayerTag, &Desc))
			return E_FAIL;
	}
	return S_OK;
}

HRESULT CLevelHogwartWorld::SpanwWorldAgent()
{
	CGriff::GRIFF_DESC Griff{};
	Griff.sObjectTag = "Griff";
	Griff.LevelTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
	Griff.ReSourceTag = "Model_Resource_Griff";
	Griff.vPos = _float3(30.f, 75.f, -326.f);
	Griff.ChildModelTag = "Model_Resource_Griff";
	Griff.ChildObjectTag = "GriffChild";
	//Griff.WayName = ""; //넣을거
	auto Handle = CGameInstance::Get().AddGameObjectToLayer(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Griff, "02_Griff", &Griff);
	if (!Handle)
	{
		MSG_BOX("Create Failed to Griff in Hogwart");
		return E_FAIL;
	}
	else
	{
		auto pGiff = CGameInstance::Get().GetGameObjectByHandleT<CGriff>(Handle.value());
		if (nullptr == pGiff)
			return E_FAIL;
		pGiff->Set_Child();
	}
	{
	
		CGriff::GRIFF_DESC Phoneix{};
		Phoneix.sObjectTag = "Phoneix";
		Phoneix.LevelTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
		Phoneix.ReSourceTag = "Model_Resource_Bird_Phoneix";
		Phoneix.ChildModelTag = "Model_Resource_Bird_Phoneix";
		Phoneix.ChildObjectTag = "PhoneixChild";
		Phoneix.WayName = "Phoneix"; //넣을거
		auto Handle = CGameInstance::Get().AddGameObjectToLayer(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Griff, "02_Phoneix", &Phoneix);
		if (!Handle)
		{
			MSG_BOX("Create Failed to Phoneix in Hogwart");
			return E_FAIL;
		}
		else
		{
			auto Phoneix = CGameInstance::Get().GetGameObjectByHandleT<CGriff>(Handle.value());
			if (nullptr == Phoneix)
				return E_FAIL;
			Phoneix->Set_Child();
		}
	}
	return S_OK;
}

HRESULT CLevelHogwartWorld::Initialize_VolumetricFog()
{

	CB_VLFOG FogOption{};

	FogOption.g_fFogColor = {255.f / 255.f, 227.f / 255.f, 184.f / 255.f};
	FogOption.g_fFogIntensity = 0.70f;
	FogOption.g_fFogDensity = 0.004f;
	FogOption.g_fFogNoiseScale = 0.1f;
	FogOption.g_fFogScattering = 1.f;
	FogOption.g_fFogBaseBrightness = 0.05f;

	FogOption.g_fFogLightColor = {255.f / 255.f, 230.f / 255.f, 180.f / 255.f};
	FogOption.g_fFogLightDirection = {0.577f, -0.577f, 0.577f};

	FogOption.g_fFogBaseHeight = 300.f;
	FogOption.g_fFogMaxHeight = 600.f;
	FogOption.g_fFogHeightFallOff = 0.1f;

	FogOption.g_fFogStartDistance = 250.f;
	FogOption.g_fFogEndDistance = 500.f;

	CGameInstance::Get().Set_VolumetricFogOption(FogOption);

	return S_OK;
}

HRESULT CLevelHogwartWorld::Initialize_EnviromentLight()
{

	CB_ENVLIGHT EnviromentLightOption{};

	EnviromentLightOption.m_fEnviromentIntensity = 0.75f;
	EnviromentLightOption.m_fFillLightBrightness = 0.25f;
	EnviromentLightOption.m_fDirectLightBrightness = 0.60f;

	CGameInstance::Get().Set_EnviromentLight(EnviromentLightOption);

	return S_OK;
}

HRESULT CLevelHogwartWorld::Initialize_LoopEffect()
{

	CGameInstance::Get().Spawn("CGY_HogwartSteam.json",
							   XMMatrixTranslation(106.42f, -7.5f, -212.875f) * XMMatrixScaling(2.5f, 2.5f, 2.5f),
							   _vector{},
							   true);

	return S_OK;
}

UPtr<CLevelHogwartWorld> CLevelHogwartWorld::Create()
{
	auto instance = UPtr<CLevelHogwartWorld>(new CLevelHogwartWorld{});
	if (FAILED(instance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevelHogwartWorld");
		return nullptr;
	}
	return instance;
}

void CLevelHogwartWorld::Free()
{
	if (auto *pNpcManager = E::CGameInstance::Get().GetNpcPlacementManager())
		pNpcManager->ClearNpcOptions();

	CLevel::Free();
}
