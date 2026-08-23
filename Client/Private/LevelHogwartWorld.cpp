#include "pch.h"
#include "LevelHogwartWorld.h"

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
#include "Griff.h"
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

	if (FAILED(SpawnPlayerCape(*hPlayer)))
		return E_FAIL;

	if (FAILED(gameInstance.LoadMap(CLevelHogwartWorldLoader::MAP_PATH, true)))
		return E_FAIL;

	if (FAILED(SpawnStaticCollision()))
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

	if (FAILED(SpawnWater()))
		return E_FAIL;

	if (FAILED(SpawnMonster(*hPlayer)))
		return E_FAIL;
	if (FAILED(SpanwNpc(*hPlayer)))
		return E_FAIL;
	if (FAILED(SpanwAnimal()))
		return E_FAIL;
	gameInstance.Add_DirectionalLight({ 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, 10.f);


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

HRESULT CLevelHogwartWorld::SpawnTerrain(CHandle hPlayer)
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
	CGameObject::GAMEOBJECT_DESC skyDesc{};
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

HRESULT CLevelHogwartWorld::SpanwNpc(CHandle hPlayer)
{
	//일단 거미 모ㅓ델로
	CWorldNpc::NPC_DESC Npc{};
	Npc.sObjectTag = "Npc";
	Npc.TargetHandle = hPlayer;
	Npc.LevelTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
	Npc.ReSourceTag = "Model_Resource_Spider";
	Npc.resBeHaviorMajor = "BTJSON";
	Npc.resBeHaviorMinor = "NPC1";
	//////////NPC Navi 타는 시작,끝점/////////
	Npc.vPos = _float3(226.097f, 48.242f, 122.760f);
	Npc.vStartPos = Npc.vPos;
	Npc.vEndPos = _float3(296.751f, 63.623f, 307.855f);

	auto Npce = CGameInstance::Get().AddGameObjectToLayer(LEVEL::HOGWART_WORLD, PROTO_GAMEOBJECT::Prototype_GameObject_WorldNpc, "02_Npc", &Npc);
	if (!Npce)
	{
		MSG_BOX("Create Failed to Npc in Hogwart");
		return E_FAIL;
	}
	return S_OK;
}

HRESULT CLevelHogwartWorld::SpanwAnimal()
{
	CGriff::ANIMAL_DESC Griff{};
	Griff.sObjectTag = "Griff";
	Griff.LevelTag = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
	Griff.ReSourceTag = "Model_Resource_Griff";
	Griff.vPos = _float3(226.097f, 48.242f, 122.760f);

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
	CLevel::Free();
}
