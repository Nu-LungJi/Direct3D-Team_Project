#include "LevelHogwartWorld.h"
#include "SkyCloudyCube.h"
#include "pch.h"

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

namespace
{
	_bool g_bOllivanderMoneyTripStarted{};
}

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
		struct NPC_TAGS { _string TagName{}; _string Resources{}; _string AnimPath{}; };
		NPC_TAGS Tags[]{
		{ "VictorRookwood","Model_Resource_NPC_VictorRookwood" },
		{"LeopoldBabcocke","Model_Resource_NPC_LeopoldBabcocke"},
		{"SolomonSallow","Model_Resource_NPC_SolomonSallow"}, 
		{"TownCrier","Model_Resource_NPC_TownCrier"},
		{"CrispinDunn","Model_Resource_NPC_CrispinDunn"}, 
		{"AugustusHill","Model_Resource_NPC_AugustusHill"}, 
		{ "FEMALE_AdelaideOakes", "Model_Resource_NPC_AdelaideOakes","./Resources/SampleClient/Models/Skeleton/NPC_AdelaideOakes/"},
		{ "FEMALE_MirabelGarlick", "Model_Resource_NPC_MirabelGarlick","./Resources/SampleClient/Models/Skeleton/NPC_MirabelGarlick/" }, 
		{ "FEMALE_AnneSallow", "Model_Resource_NPC_AnneSallow","./Resources/SampleClient/Models/Skeleton/NPC_AnneSallow/" } };
		pNpcManager->RegisterNpcOption("World NPC", NpcOption);
		for(auto& Tag :Tags)
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, Tag.TagName, NpcOption.sModelGroupTag,Tag.Resources, Tag.AnimPath);

		pNpcManager->RegisterBehaviorOption("World NPC", "BTJSON", "NPC1");
		{
			NpcOption.sPrototypeTag = MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_WorldAnimal);
			NpcOption.sLayerTag = "02_Animal";
			pNpcManager->RegisterNpcOption("Animal", NpcOption);
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "Spider",NpcOption.sModelGroupTag, "Model_Resource_Spider");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "Cat", NpcOption.sModelGroupTag, "Model_Resource_Cat","./Resources/SampleClient/Models/Skeleton/Cat/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "Bird_Kestrel", NpcOption.sModelGroupTag, "Model_Resource_Bird_Kestrel", "./Resources/SampleClient/Models/Skeleton/Birds_Kestrel/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "BlueButterfly_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_BlueButterfly_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/BlueButterfly_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "CaptureBag_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_CaptureBag_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/CaptureBag_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "ChompingCabbage_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_ChompingCabbage_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/ChompingCabbage_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "DisillusionmentChest_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_DisillusionmentChest_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/DisillusionmentChest_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "FlyingMagicPaper_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_FlyingMagicPaper_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/FlyingMagicPaper_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "GACTreasureChest_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_GACTreasureChest_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/GACTreasureChest_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "GiantPendulumClock_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_GiantPendulumClock_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/GiantPendulumClock_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "GlowingLumosMoth_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_GlowingLumosMoth_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/GlowingLumosMoth_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "Hippogriff_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_Hippogriff_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/Hippogriff_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "HoppingPot_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_HoppingPot_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/HoppingPot_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "IdentificationStation_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_IdentificationStation_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/IdentificationStation_Animated_Blender_4_3/");
			/*pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "MagicChoppingIngredients_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_MagicChoppingIngredients_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/MagicChoppingIngredients_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "MagicChoppingStation_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_MagicChoppingStation_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/MagicChoppingStation_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "MagicMaterialRefinerTools_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_MagicMaterialRefinerTools_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/MagicMaterialRefinerTools_Animated_Blender_4_3/");*/
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "OrangeButterfly_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_OrangeButterfly_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/OrangeButterfly_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "OutdoorDiricawlBird_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_OutdoorDiricawlBird_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/OutdoorDiricawlBird_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "OutdoorFwooperBird_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_OutdoorFwooperBird_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/OutdoorFwooperBird_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "SanctuaryToyBox_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_SanctuaryToyBox_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/SanctuaryToyBox_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "SelfWrappingPaper_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_SelfWrappingPaper_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/SelfWrappingPaper_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "ShopCounterHandBell_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_ShopCounterHandBell_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/ShopCounterHandBell_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "StirCrazyTeaSpoon_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_StirCrazyTeaSpoon_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/StirCrazyTeaSpoon_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "StreetRabbit_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_StreetRabbit_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/StreetRabbit_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "StreetRat_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_StreetRat_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/StreetRat_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "StreetRaven_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_StreetRaven_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/StreetRaven_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "StreetSquirrel_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_StreetSquirrel_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/StreetSquirrel_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "TeaShopTeaCup_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_TeaShopTeaCup_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/TeaShopTeaCup_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "ThestralStreetCarriage_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_ThestralStreetCarriage_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/ThestralStreetCarriage_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "VillageGiantToad_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_VillageGiantToad_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/VillageGiantToad_Animated_Blender_4_3/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "WizardingDeck_Animated_Blender_4_3", NpcOption.sModelGroupTag, "Model_Resource_WizardingDeck_Animated_Blender_4_3", "./Resources/SampleClient/Models/Skeleton/WizardingDeck_Animated_Blender_4_3/");
			
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
				if (Placement.eRuntimeType == E::NPC_RUNTIME_TYPE::GPU_CROWD_AMBIENT)
					return E::NPC_PLACEMENT_RESULT{
						Placement.iPlacementId, false, {}, "GPU Crowd runtime is not implemented."};
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
		Desc.Repeatable = true;
		Desc.AutoStartOnEnter = true;
		Desc.AutoAdvanceOpeningLineCount = 2u;
		// 입장 시네마틱 10초 동안 첫 두 대사를 각각 5초씩 자동 표시한다.
		Desc.OpeningLineAutoAdvanceDelay = 5.f;
		Desc.HideDialogueInteractionPrompt = true;
		Desc.FadeDuration = 0.6f;
		Desc.FadeHoldDuration = 0.25f;
		Desc.IdleExpressionAnim = "AN_BODY__Idle__Hu_BM_Idle_Casual_Loop.bin";
		Desc.WorldSpaceShop = false;
		Desc.RepositionPlayerForDialogue = false;
		Desc.ResolveStartDialogueIndex = []()
		{
			return g_bOllivanderMoneyTripStarted ? 4u : 0u;
		};

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
				"일이 많아서.. 잠시만 기다려주십쇼!",
				"",
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
					{ "예", 3, CInteractiveNpc::DIALOGUE_ACTION::NONE,
						[]() { g_bOllivanderMoneyTripStarted = true; return 3u; } },
					{ "아니오", 3, CInteractiveNpc::DIALOGUE_ACTION::NONE,
						[]() { g_bOllivanderMoneyTripStarted = true; return 3u; } }
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
			},
			{
				"오우, 돈을 다 모아오셨군요!",
				"AN_BODY__DialogueTalk__HU_STN_STND_Conv_Talk.bin",
				true,
				{},
				{},
				CInteractiveNpc::DIALOGUE_ACTION::NONE,
				false,
				false,
				"ShopNpcDialogueCloseUp",
				true,
				3.f
			},
			{
				"그럼 자, 여기 지팡이를 한번 골라보십쇼!",
				"AN_BODY__DialogueTalk__HU_STN_STND_Conv_Talk.bin",
				true,
				{},
				{},
				CInteractiveNpc::DIALOGUE_ACTION::OPEN_SHOP,
				false,
				false,
				"ShopNpcDialogueCloseUp",
				true,
				3.5f
			}
		};
		// 기존 호그와트 쪽 액티비티 시작 지점을 이동 목적지로 사용한다.
		Desc.MoveDestination = {
			{ 1900.461f, 40.9f, 281.991f  }
		};
		Desc.MoveOutcomeAnimation =
			"AN_BODY__Meeting__Shot_180_GerboldOllivander.bin";
		Desc.OnMoveDestinationApplied = [this, hPlayer = *hPlayer]()
		{
			RequestSummonersCourtSpawn(hPlayer);
			GET_SINGLE(UIManager)->CreateOrChangeQuest(
				"미니게임 참여하기");
			GET_SINGLE(UIManager)->SetMiniMapObjectiveActive(
				"Hogwart_MiniGameNpcQuest", true);
		};
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
						CInteractiveNpc::DIALOGUE_ACTION::CONTINUE_DIALOGUE,
						[]()
						{
							GET_SINGLE(UIManager)->CreateOrChangeQuest(
								"소환사의 코트 참여하기");
							GET_SINGLE(UIManager)->SetMiniMapObjectiveActive(
								"Hogwart_AccioStudentQuest", true);
							return 4u;
						}
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

	GET_SINGLE(UIManager)->SetRaceReturnToShopCallback([this]()
	{
		RequestSummonersCourtDespawn();
	});

	// 레벨 진입 후 3초 동안 검은 화면을 유지하고,
	// 이후 2초 동안 검은 UI를 사라지게 해 게임 화면을 드러낸다.
	GET_SINGLE(UIManager)->CreateFadeOut(3.f, 2.f);

	return S_OK;
}

void CLevelHogwartWorld::Update(E::_float fTimeDelta)
{
	UNREFERENCED_PARAMETER(fTimeDelta);
	UpdateRequestedSummonersCourtDespawn();
	UpdateRequestedSummonersCourtSpawn();
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

	if (m_bCreatePlayScreenUI && !m_bQuestCreated)
	{
		m_bQuestCreated = true;
		GET_SINGLE(UIManager)->CreateOrChangeQuest(
			"호그스미스 둘러보기");
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
		//PruneInvalidRuntimeHandles(m_CoinCollisionHandles);

		const HRESULT hrAccio = SpawnSummonersCourtIfNeeded(m_hDebugPlayer);
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

void CLevelHogwartWorld::RequestSummonersCourtSpawn(CHandle hPlayer)
{
	m_hPendingSummonersCourtPlayer = hPlayer;
}

void CLevelHogwartWorld::UpdateRequestedSummonersCourtSpawn()
{
	if (!m_hPendingSummonersCourtPlayer)
		return;

	const CHandle hPlayer = *m_hPendingSummonersCourtPlayer;
	m_hPendingSummonersCourtPlayer.reset();
	if (FAILED(SpawnSummonersCourtIfNeeded(hPlayer)))
	{
		DespawnRuntimeObjects(m_AccioActivityHandles);
		DEBUG_LOG("[HogwartWorld] Shop NPC failed to spawn Summoner's Court.\n");
	}
}

HRESULT CLevelHogwartWorld::SpawnSummonersCourtIfNeeded(CHandle hPlayer)
{
	PruneInvalidRuntimeHandles(m_AccioActivityHandles);
	if (!m_AccioActivityHandles.empty())
		return S_FALSE;

	return SpawnAccioActivity(
		hPlayer,
		{ 1935.f, 24.8f, 325.f },
		180.f,
		1.f);
}

void CLevelHogwartWorld::RequestSummonersCourtDespawn()
{
	m_bSummonersCourtDespawnRequested = true;
}

void CLevelHogwartWorld::UpdateRequestedSummonersCourtDespawn()
{
	if (!m_bSummonersCourtDespawnRequested)
		return;

	m_bSummonersCourtDespawnRequested = false;
	DespawnRuntimeObjects(m_AccioActivityHandles);
	DEBUG_LOG("[HogwartWorld] Race return removed Summoner's Court.\n");
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

HRESULT CLevelHogwartWorld::SpawnPropBarrelBlock()
{
	if (m_bPropBarrelBlockSpawned)
		return S_OK;

	// 지정한 두 좌표를 바닥 줄의 정확한 양 끝점으로 사용한다.
	const _float3 vBaseStart{ 270.173f, 39.265f, 121.917f };
	const _float3 vBaseEnd{ 280.773f, 39.376f, 134.895f };
	const _float fBaseHeightOffset = 1.f;
	const _float fHorizontalSpacingScale = 1.05f;
	const _float fVerticalSpacing = 3.9f;
	// 기존 6열 간격은 유지하되 오른쪽 끝 열만 제외한다.
	constexpr uint32_t iOriginalColumnCount = 6u;
	constexpr uint32_t iColumnCount = 5u;
	constexpr uint32_t iRowCount = 3u;

	std::vector<CHandle> SpawnedHandles{};
	SpawnedHandles.reserve(iColumnCount * iRowCount);
	uint32_t iBarrelIndex{};
	for (uint32_t iRow = 0; iRow < iRowCount; ++iRow)
	{
		for (uint32_t iColumn = 0; iColumn < iColumnCount; ++iColumn)
		{
			const _float fColumnRatio = static_cast<_float>(iColumn) /
				static_cast<_float>(iOriginalColumnCount - 1u) *
				fHorizontalSpacingScale;
			CPropBarrel::DESC Desc{};
			Desc.sObjectTag =
				"Hogwart_PropBarrel_Block_" + std::to_string(iBarrelIndex++);
			Desc.sResourceGroup = "PERMANENT";
			Desc.vInitialRotation = { 90.f, 0.f, 0.f };
			// 다이나믹 상태로 배치하되 첫 충돌 전까지 현재 자세에서 잠재운다.
			Desc.bStartSleeping = true;
			Desc.fCollisionDestroyGraceTime = 2.f;
			Desc.vInitialPosition =
			{
				vBaseStart.x + (vBaseEnd.x - vBaseStart.x) * fColumnRatio,
				vBaseStart.y + (vBaseEnd.y - vBaseStart.y) * fColumnRatio +
					fBaseHeightOffset +
					static_cast<_float>(iRow) * fVerticalSpacing,
				vBaseStart.z + (vBaseEnd.z - vBaseStart.z) * fColumnRatio
			};

			const auto hBarrel = CGameInstance::Get().AddGameObjectToLayer(
				"PERMANENT",
				PROTO_GAMEOBJECT::Prototype_GameObject_PropBarrel,
				"PropBarrel",
				&Desc);
			if (!hBarrel)
			{
				DespawnRuntimeObjects(SpawnedHandles);
				return E_FAIL;
			}
			SpawnedHandles.push_back(*hBarrel);
		}
	}

	m_bPropBarrelBlockSpawned = true;
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
	MonS.OnBeforeTrollSpawn = [this]()
	{
		return SpawnPropBarrelBlock();
	};
	if (!CGameInstance::Get().AddGameObjectToLayer(
			LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_MonSpawner, "00.MonSpawn", &MonS))
	{
		return E_FAIL;
	}

	return S_OK;
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
	GET_SINGLE(UIManager)->SetRaceReturnToShopCallback({});
	if (auto *pNpcManager = E::CGameInstance::Get().GetNpcPlacementManager())
		pNpcManager->ClearNpcOptions();

	CLevel::Free();
}
