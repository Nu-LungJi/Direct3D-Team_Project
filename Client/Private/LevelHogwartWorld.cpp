#include "pch.h"
#include "LevelHogwartWorld.h"
#include "SkyCloudyCube.h"

#include "GameInstance.h"
#include "LevelHogwartWorldLoader.h"
#include "FlyCamera.h"
#include "UiCamera.h"
#include "Player.h"
#include "PlayerThirdPersonCamera.h"
#include "NvClothCape.h"
#include "UIController.h"
#include "UIManager.h"
#include "Mon_Spawner.h"
#include "WorldNpc.h"
#include "WorldAgent.h"
#include "Griff.h"
#include "NpcPlacementData.h"
#include "NpcPlacementManager.h"
#include "Troll.h"
// Client에도 같은 이름의 Terrain.h가 있으므로 Engine SDK 헤더를 명시한다.
#include "../../EngineSDK/Inc/Terrain.h"
#include "Water.h"

NS_USING(Client)

CLevelHogwartWorld::CLevelHogwartWorld()
	: CLevel{ ETOUI(LEVEL::HOGWART_WORLD) }
{
}

HRESULT CLevelHogwartWorld::Initialize()
{
	auto& gameInstance = E::CGameInstance::Get();
	gameInstance.GameObjectAllResetExceptLayers({
		"00_ENGINE_CINEMATIC_CAMERA"
	});

	if (FAILED(gameInstance.Initialize_EffectLight(15)))
		return E_FAIL;

	const auto hPlayer = SpawnPlayer();
	if (!hPlayer)
		return E_FAIL;
	if (auto* pNpcManager = gameInstance.GetNpcPlacementManager())
	{
		pNpcManager->ClearNpcOptions();
		pNpcManager->SetPickingQueryMask(ETOUI(COLLISION_LAYER::WORLD_STATIC));
		E::NPC_PLACEMENT_DESC NpcOption{};
		NpcOption.sPrototypeGroupTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
		NpcOption.sPrototypeTag = MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_WorldNpc);
		NpcOption.sLayerTag = "02_Npc";
		NpcOption.sModelGroupTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
		NpcOption.sModelResourceTag = "Model_Resource_Spider";
		NpcOption.sBehaviorMajorTag = "BTJSON";
		NpcOption.sBehaviorMinorTag = "NPC1";
		pNpcManager->RegisterNpcOption("World NPC", NpcOption);
		struct NPC_SKELETON_OPTION { const char* pName; const char* pTag; };
		static constexpr NPC_SKELETON_OPTION NpcSkeletons[] =
		{
			{ "Augustus Hill (Single NPC Test)", "Model_Resource_NPC_AugustusHill" },
		};
		for (const auto& Option : NpcSkeletons)
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, Option.pName, NpcOption.sModelGroupTag, Option.pTag);
		pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "NPC_MirabelGarlick", NpcOption.sModelGroupTag, "Model_Resource_NPC_MirabelGarlick","./Resources/SampleClient/Models/Skeleton/NPC_MirabelGarlick/" );


		pNpcManager->RegisterBehaviorOption("World NPC", "BTJSON", "NPC1");
		{
			NpcOption.sPrototypeTag = MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_WorldAnimal);
			NpcOption.sLayerTag = "02_Animal";
			pNpcManager->RegisterNpcOption("Animal", NpcOption);
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "Spider",NpcOption.sModelGroupTag, "Model_Resource_Spider");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "Cat", NpcOption.sModelGroupTag, "Model_Resource_Cat","./Resources/SampleClient/Models/Skeleton/Cat/");
			pNpcManager->RegisterNpcSkeletonOption(NpcOption.sPrototypeTag, "Bird_Kestrel", NpcOption.sModelGroupTag, "Model_Resource_Bird_Kestrel", "./Resources/SampleClient/Models/Skeleton/Birds_Kestrel/");

			
		}

		pNpcManager->SetSpawnCallback([hTarget = *hPlayer](const E::NPC_PLACEMENT_DESC& Placement)
		{
			if (Placement.sPrototypeTag != MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_WorldNpc) &&
				Placement.sPrototypeTag != MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_WorldAnimal))
				return E::NPC_PLACEMENT_RESULT{ Placement.iPlacementId, false, {}, "Unsupported Hogwarts NPC." };
			
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
				return E::NPC_PLACEMENT_RESULT{ Placement.iPlacementId, false, {}, "Spawn failed." };

			return E::NPC_PLACEMENT_RESULT{ Placement.iPlacementId, true, *hNpc, "Spawn succeeded." };
		});
	}

	if (FAILED(SpawnPlayerCape(*hPlayer)))
		return E_FAIL;

	if (FAILED(gameInstance.LoadMap(CLevelHogwartWorldLoader::MAP_PATH, true)))
		return E_FAIL;

	if (FAILED(SpawnStaticCollision()))
		return E_FAIL;

	if (FAILED(SpawnCoinCollision()))
		return E_FAIL;

	if (FAILED(SpawnTerrain(*hPlayer)))
		return E_FAIL;

	if (FAILED(SpawnFlyCamera()) ||
		FAILED(SpawnUICamera()) ||
		FAILED(SpawnPlayerCamera(*hPlayer)))
	{
		return E_FAIL;
	}

	if (FAILED(SpawnSkyBox()))
		return E_FAIL;

	//if (FAILED(SpawnWater()))
	//	return E_FAIL;

	if (FAILED(SpawnMonster(*hPlayer)))
		return E_FAIL;
	//if (FAILED(SpawnNpcPlacements(*hPlayer, "./Resources/json/NPC/NpcSpawnIdle.json")))
	//	return E_FAIL;
	//if (FAILED(SpawnNpcPlacements(*hPlayer, "./Resources/json/NPC/NpcSpawnWalk.json")))
	//	return E_FAIL;
	if (FAILED(SpawnNpcPlacements(*hPlayer, "./Resources/json/NPC/Cat.json")))
		return E_FAIL;
	if (FAILED(SpawnNpcPlacements(*hPlayer, "./Resources/json/NPC/RunSpider.json")))
		return E_FAIL;
	if (FAILED(SpawnNpcPlacements(*hPlayer, "./Resources/json/NPC/Birds.json")))
		return E_FAIL;

	if (FAILED(SpanwWorldAgent()))
		return E_FAIL;
	//gameInstance.Add_DirectionalLight({ 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, 10.f);

	if (FAILED(Initialize_VolumetricFog()))
		return E_FAIL;

	if (FAILED(Initialize_EnviromentLight()))
		return E_FAIL;

	// 레벨 진입 후 3초 동안 검은 화면을 유지하고,
	// 이후 2초 동안 검은 UI를 사라지게 해 게임 화면을 드러낸다.
	GET_SINGLE(UIManager)->CreateFadeOut(3.f, 2.f);

	return S_OK;
}

void CLevelHogwartWorld::Update(E::_float fTimeDelta)
{
	UNREFERENCED_PARAMETER(fTimeDelta);

	if (!m_bCreatePlayScreenUI)
	{
		CGameObject::GAMEOBJECT_DESC desc{};
		desc.sObjectTag = "UIController";

		const auto hUIController = E::CGameInstance::Get().AddGameObjectToLayer(
			"LEVEL_HOGWART_WORLD",
			"Prototype_GameObject_UIController",
			"UIController",
			&desc);
		GET_SINGLE(UIManager)->SetUIController(hUIController);
		m_bCreatePlayScreenUI = hUIController.has_value();
	}

	GET_SINGLE(UIManager)->UpdateRootUIHandles();
}

HRESULT CLevelHogwartWorld::Render()
{
	return S_OK;
}

void CLevelHogwartWorld::UpdateGUI()
{
	ImGui::Begin("Level: Hogwart World");
	ImGui::End();
}

std::optional<CHandle> CLevelHogwartWorld::SpawnPlayer()
{
	CPlayer::DESC desc{};
	desc.sObjectTag = "Player";
	// Hogsmeade 중심부의 Terrain 높이(약 48)보다 조금 위에서 시작한다.
	desc.vInitialPosition = { 200.f, 55.f, 80.f };
	desc.LevelTag = LEVEL::HOGWART_WORLD;
	desc.tFilter = PX_FILTER_DESC{
		.iLayer = ETOUI(COLLISION_LAYER::PLAYER_BODY),
		.iSimulationMask = PX_ALL_LAYERS,
		.iQueryMask =
			ETOUI(COLLISION_LAYER::WORLD_STATIC) |
			ETOUI(COLLISION_LAYER::MOVING_PLATFORM)
	};

	return E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_Player,
		"03_Player",
		&desc);
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
	desc.vLocalPosition = { 0.05f, 0.08f, 0.f };

	E::CGameInstance::Get().JsonDeSerialize(
		"./Resources/NvCloth/CollisionRigs/ProfessorCape.nvclothcollision.json",
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
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_NvClothCape,
		"03_Player",
		&desc))
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

HRESULT CLevelHogwartWorld::SpawnTerrain(std::optional<CHandle> hPlayer)
{
	E::CTerrain::DESC desc{};
	desc.sObjectTag = "HogwartWorldTerrain";
	desc.textureGroup = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
	desc.textureTag = "TEX2D_Terrain_Tile0";
	desc.tPhysicsFilter = PX_FILTER_DESC{
		.iLayer = ETOUI(COLLISION_LAYER::WORLD_STATIC),
		.iSimulationMask = PX_ALL_LAYERS,
		.iQueryMask = PX_ALL_LAYERS
	};

	const auto hTerrain = E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_Terrain,
		"01_Terrain",
		&desc);
	if (!hTerrain)
		return E_FAIL;

	auto* terrain = E::CGameInstance::Get().GetGameObjectByHandleT<E::CTerrain>(*hTerrain);
	if (!terrain)
		return E_FAIL;

	if (FAILED(terrain->LoadTerrain(CLevelHogwartWorldLoader::TERRAIN_PATH, hPlayer)))
		return E_FAIL;

	return SpawnNaviMesh(terrain); 
}

HRESULT CLevelHogwartWorld::SpawnNaviMesh(E::CTerrain* pTerrain)
{
	if (nullptr == pTerrain)
		return E_FAIL;

	auto* pNavMesh =
		CGameInstance::Get().GetNavMeshManager();

	if (nullptr == pNavMesh)
		return E_FAIL;

	const std::string strPath =
		(std::filesystem::path(
			CLevelHogwartWorldLoader::MAP_PATH) /
			"navmesh.json").generic_string();

	// 에디터에서 지정한 Blocked 영역 불러오기
	if (FAILED(pNavMesh->Load(strPath)))
		return E_FAIL;

	std::vector<_float3> Vertices{};
	Vertices.reserve(
		pTerrain->GetVertices().size());

	const _matrix TerrainWorld =
		pTerrain->GetTransform()
		.GetLoadedCombinedWorldMatrix();

	// Terrain 정점은 로컬 좌표이므로 월드 좌표로 변환
	for (const auto& Vertex :
		pTerrain->GetVertices())
	{
		_float3 vWorldPosition{};

		XMStoreFloat3(
			&vWorldPosition,
			XMVector3TransformCoord(
				XMLoadFloat3(&Vertex.pos),
				TerrainWorld));

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

	//if (!pNavMesh->Build(
	//	Vertices,
	//	pTerrain->GetIndices(),
	//	Desc))
	//{
	//	return E_FAIL;
	//}
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
	desc.vAt = { 200.f, 48.f, 80.f };
	desc.vEye = { 200.f, 65.f, 60.f };
	desc.fAspect = g_iWinSizeX / static_cast<E::_float>(g_iWinSizeY);
	desc.fFovY = 75.f;
	desc.fNear = 0.1f;
	desc.fFar = 3000.f;
	desc.sObjectTag = "FlyCam";

	const auto hCamera = E::CGameInstance::Get().AddGameObjectToLayer(
		"CAMERAS", "Prototype_GameObject_FlyCamera", "99_CAMERA", &desc);
	return hCamera && SUCCEEDED(E::CGameInstance::Get().RegistCamera("FLY", *hCamera))
		? S_OK
		: E_FAIL;
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
	desc.vEye = { 0.f, 0.f, -0.1f };

	const auto hCamera = E::CGameInstance::Get().AddGameObjectToLayer(
		"CAMERAS", "Prototype_GameObject_UICamera", "99_CAMERA", &desc);
	return hCamera && SUCCEEDED(E::CGameInstance::Get().RegistCamera("UI", *hCamera))
		? S_OK
		: E_FAIL;
}

HRESULT CLevelHogwartWorld::SpawnPlayerCamera(CHandle hPlayer)
{
	CPlayerThirdPersonCamera::DESC desc{};
	desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
	desc.vAt = { 200.f, 55.f, 80.f };
	desc.vEye = { 200.f, 58.f, 73.f };
	desc.fAspect = g_iWinSizeX / static_cast<E::_float>(g_iWinSizeY);
	desc.fFovY = 75.f;
	desc.fNear = 0.1f;
	desc.fFar = 3000.f;
	desc.sObjectTag = "PlayerCamera";
	desc.hTarget = hPlayer;

	const auto hCamera = E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_PlayerThirdPersonCamera,
		"101_CAMERA",
		&desc);
	if (!hCamera || FAILED(E::CGameInstance::Get().RegistCamera("PlayerCamera", *hCamera)))
		return E_FAIL;

	E::CGameInstance::Get().SetActiveCamera("PlayerCamera");
	return S_OK;
}

HRESULT CLevelHogwartWorld::SpawnSkyBox()
{
	CSkyCloudyCube::SKY_DESC skyDesc{};
	skyDesc.sObjectTag = "SkyCloudyCube";
	if (!CGameInstance::Get().AddGameObjectToLayer("PERMANENT", "Prototype_GameObject_SkyCloudyCube", "00_SKYBOX", &skyDesc))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CLevelHogwartWorld::SpawnWater()
{
	E::CWater::WATER_DESC desc{};
	desc.sObjectTag = "HogwartWorldOcean";
	desc.vPosition = { 200.f, -75.f, 80.f };
	desc.vSize = { 8000.f, 8000.f };
	desc.vWaterColor = { 0.012f, 0.055f, 0.16f, 0.9f };
	desc.vShallowColor = { 0.018f, 0.10f, 0.24f, 1.f };
	desc.vDeepColor = { 0.002f, 0.012f, 0.055f, 1.f };
	desc.vReflectionColor = { 0.10f, 0.20f, 0.38f, 1.f };
	desc.fUVScale = 0.018f;
	desc.fSecondaryNormalScale = 2.7f;
	desc.fWaveIntensity = 0.95f;
	desc.fFollowSnap = 50.f;
	desc.bFollowCamera = true;

	return E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_Water,
		"00_WATER",
		&desc)
		? S_OK
		: E_FAIL;
}

HRESULT CLevelHogwartWorld::SpawnMonster(std::optional<CHandle> hPlayer)
{
	CMon_Spawner::MON_SPAWNER_DESC MonS{};
	MonS.sObjectTag = "MonSpawn";
	MonS.handle = hPlayer.value();
	if (!CGameInstance::Get().AddGameObjectToLayer(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_MonSpawner,"00.MonSpawn", & MonS))
	{
		return E_FAIL;
	}

	CTroll::TROLL_DESC Troll{};
	Troll.sObjectTag = "Troll";
	Troll.TargetHandle = hPlayer.value();
	Troll.LevelTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
	Troll.vPos = _float3(260.353f, 40.679f, 138.799f);
	Troll.ReSourceTag = "Model_Resource_Troll";
	Troll.WeaponProtoName = MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_TrollWeapon);
	Troll.WeaponResourceName = "Model_Resource_TrollWeapon";
	//Troll.resBeHaviorMajor = "BTJSON";
	//Troll.resBeHaviorMinor = "ENDERDRAGON";
	Troll.MonType = MONSTER_TYPE::BOSS;

	if (!CGameInstance::Get().AddGameObjectToLayer(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_Troll, "02.Troll", &Troll))
	{
		return E_FAIL;
	}
}
HRESULT CLevelHogwartWorld::SpawnStaticCollision()
{
	auto handles = CGameInstance::Get()
		.GetPhysXManager()
		->CreateCollisionProxyObjectsFromFile(
			"Level_HogwartWorld",
			"00_MapCollision");

	if (handles.empty())
		return E_FAIL;

	return S_OK;
}

HRESULT CLevelHogwartWorld::SpawnCoinCollision()
{
	auto handles = CGameInstance::Get()
		.GetPhysXManager()
		->CreateCollisionProxyObjectsFromFile(
			"Level_HogwartCoin",
			"00_CoinCollision");

	if (handles.empty())
		return E_FAIL;

	return S_OK;
}

HRESULT CLevelHogwartWorld::SpawnNpcPlacements(CHandle hPlayer, const _string& Path)
{
	E::NPC_PLACEMENT_FILE File{};
	if (FAILED(E::CGameInstance::Get().JsonDeSerialize(
		Path,
		File,
		"NpcPlacements")))
		return E_FAIL;

	if (File.iVersion != 1)
		return E_FAIL;

	for (const auto& Placement : File.Placements)
	{
		if (Placement.eRuntimeType == E::NPC_RUNTIME_TYPE::GPU_CROWD_AMBIENT)
			return E_NOTIMPL;

		CWorldNpc::WORLD_AGENT_DESC Desc{};
		Desc.sObjectTag = Placement.sPrototypeTag;
		Desc.TargetHandle = hPlayer;
		Desc.LevelTag = Placement.sModelGroupTag.empty()
			? Placement.sPrototypeGroupTag
			: Placement.sModelGroupTag;
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
			Placement.sPrototypeGroupTag,
			Placement.sPrototypeTag,
			Placement.sLayerTag,
			&Desc))
			return E_FAIL;
	}
	return S_OK;
}

HRESULT CLevelHogwartWorld::SpanwWorldAgent()
{
	CGriff::WORLD_AGENT_DESC Griff{};
	Griff.sObjectTag = "Griff";
	Griff.LevelTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
	Griff.ReSourceTag = "Model_Resource_Griff";
	Griff.vPos = _float3(30.f, 75.f, -326.f);

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
	return S_OK;
}

HRESULT CLevelHogwartWorld::Initialize_VolumetricFog() {

	CB_VLFOG FogOption{};

	FogOption.g_fFogColor			= { 255.f / 255.f, 227.f / 255.f, 184.f / 255.f };
	FogOption.g_fFogIntensity		= 0.25f;
	FogOption.g_fFogDensity			= 0.02f;
	FogOption.g_fFogNoiseScale		= 0.05f;
	FogOption.g_fFogScattering		= 0.5f;
	FogOption.g_fFogBaseBrightness	= 0.01f;

	FogOption.g_fFogLightColor		= { 255.f / 255.f, 230.f / 255.f, 180.f / 255.f };
	FogOption.g_fFogLightDirection	= { 0.577f, -0.577f, 0.577f };

	FogOption.g_fFogBaseHeight		= 300.f;
	FogOption.g_fFogMaxHeight		= 500.f;
	FogOption.g_fFogHeightFallOff	= 0.05f;

	FogOption.g_fFogStartDistance	= 100.f;
	FogOption.g_fFogEndDistance		= 250.f;

	CGameInstance::Get().Set_VolumetricFogOption(FogOption);

	return S_OK;
}

HRESULT CLevelHogwartWorld::Initialize_EnviromentLight() {

	CB_ENVLIGHT EnviromentLightOption{};

	EnviromentLightOption.m_fEnviromentIntensity = 0.75f;
	EnviromentLightOption.m_fFillLightBrightness = 0.25f;
	EnviromentLightOption.m_fDirectLightBrightness = 0.60f;

	CGameInstance::Get().Set_EnviromentLight(EnviromentLightOption);


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
	if (auto* pNpcManager = E::CGameInstance::Get().GetNpcPlacementManager())
		pNpcManager->ClearNpcOptions();



	CLevel::Free();
}
