#include "pch.h"
#include "LevelBossCharlesRookwood.h"
#include "SkyCloudyCube.h"
#include "GameInstance.h"
#include "Level_Defines.h"
#include "FlyCamera.h"

#include "ResCBuffer.h"
#include "BackGround.h"
#include "UiCamera.h"

#include "LevelCharlesRookwoodLoader.h"


#include "PlayerThirdPersonCamera.h"
#include "Player.h"
#include "NvClothCape.h"

#include "BossTMB.h"

#include "UIManager.h"
#include "UIController.h"

#include "LightPlacementObject.h"
#include "AmbientSound2DObject.h"
#include "ClientEvents.h"
NS_USING(Client)

CLevelBossCharlesRookwood::CLevelBossCharlesRookwood()
	: CLevel{ ETOUI(LEVEL::BOSS_CHARLES_ROOKWOOD) }
{
}

CLevelBossCharlesRookwood::~CLevelBossCharlesRookwood()
{
}

HRESULT CLevelBossCharlesRookwood::Initialize()
{
	E::CGameInstance::Get().GameObjectAllResetExceptLayers({
		"00_ENGINE_CINEMATIC_CAMERA"
	});

	GET_SINGLE(UIManager)->CreateFadeOut(2.f, 3.f);

	if (FAILED(CGameInstance::Get().Initialize_EffectLight(15)))
	{
		return E_FAIL;
	}

	auto hPlayer = SpawnPlayer();
	if (!hPlayer)
	{
		MSG_BOX("Player Handle Failed To CLevelBossCharlesRookwood");
		return E_FAIL;
	}
	if (FAILED(SpawnPlayerCape(*hPlayer)))
		return E_FAIL;

	if (FAILED(CGameInstance::Get().LoadMap("./Resources/json/MapSaved/TombBoss", true)))
		return E_FAIL;

	if (FAILED(SpawnStaticCollision()))
		return E_FAIL;

	if (FAILED(SpawnFlyCamera()))
		return E_FAIL;

	if (FAILED(SpawnUICamera()))
		return E_FAIL;

	if (FAILED(SpawnPlayerCamera(hPlayer)))
		return E_FAIL;

	if (FAILED(SpawnMonster(hPlayer)))
		return E_FAIL;

	if (FAILED(SpawnLightPlacement()))
		return E_FAIL;

	if (FAILED(SpawnSkyBox()))
		return E_FAIL;

	if (FAILED(PlayBGM()))
		return E_FAIL;

	if (FAILED(Initialize_VolumetricFog()))
		return E_FAIL;

	if (FAILED(Initialize_EnviromentLight()))
		return E_FAIL;

	SubscribePlayerDeath(*hPlayer);

	return S_OK;
}

void CLevelBossCharlesRookwood::Update(E::_float fTimeDelta)
{
	{
		if (!m_bCreatePlayScreenUI)
		{
			m_bCreatePlayScreenUI = true;
			CGameObject::GAMEOBJECT_DESC Desc{};
			Desc.sObjectTag = "UIController";

			GET_SINGLE(UIManager)->SetUIController(E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_UIController",
				"UIController", &Desc));
		}
	}

	if (m_bCreatePlayScreenUI && !m_bBossQuestCreated)
	{
		m_bBossQuestCreated = true;
		GET_SINGLE(UIManager)->CreateOrChangeQuest(
			"퍼시벌 랙햄의 시험을 완료하기");
	}

	// TombBossIntro는 행동 트리에서 비동기로 재생된다. 첫 보스 시네마틱의
	// 실제 재생 상태를 추적하여 컷신 전체 구간 동안만 2D UI를 숨긴다.
	if (!m_bBossIntroFinished)
	{
		const _bool cinematicPlaying =
			CGameInstance::Get().IsCinematicPlaying();
		if (cinematicPlaying && !m_bBossIntroPlaying)
		{
			m_bBossIntroPlaying = true;
			GET_SINGLE(UIManager)->PlayFadeOutAll2DUI(0.f, 0.35f);
		}
		else if (!cinematicPlaying && m_bBossIntroPlaying)
		{
			m_bBossIntroPlaying = false;
			m_bBossIntroFinished = true;
			GET_SINGLE(UIManager)->PlayFadeInAll2DUI(0.f, 0.35f);
			GET_SINGLE(UIManager)->CreateOrChangeQuest(
				"펜시브 쓰러트리기");

			if (const auto controllerHandle =
				GET_SINGLE(UIManager)->GetUIController())
			{
				if (auto* controller = CGameInstance::Get().
					GetGameObjectByHandleT<CUIController>(*controllerHandle))
				{
					controller->SetQuestUIGroupActive(
						QUEST_UI_GROUP::BOSS_CHARLES_ROOKWOOD,
						true, "펜시브 쓰러트리기", true, false);
				}
			}
		}
	}

	if (m_bBossIntroFinished && !m_bBossDefeated && m_hBoss)
	{
		auto* boss = CGameInstance::Get().
			GetGameObjectByHandleT<CBossTMB>(*m_hBoss);
		if (!boss || boss->GetPendingDestroy() || boss->Get_CurrentHp() <= 0)
		{
			m_bBossDefeated = true;
			GET_SINGLE(UIManager)->CreateOrChangeQuest(
				"호그스미스로 돌아가기");

			if (const auto controllerHandle =
				GET_SINGLE(UIManager)->GetUIController())
			{
				if (auto* controller = CGameInstance::Get().
					GetGameObjectByHandleT<CUIController>(*controllerHandle))
				{
					controller->SetQuestUIGroupActive(
						QUEST_UI_GROUP::BOSS_CHARLES_ROOKWOOD,
						false, {}, true, false);
				}
			}
		}
	}

	GET_SINGLE(UIManager)->UpdateRootUIHandles();
}

HRESULT CLevelBossCharlesRookwood::Render()
{
	return S_OK;
}

void CLevelBossCharlesRookwood::UpdateGUI()
{
	ImGui::Begin("level: Boss CharlesRookwood");

	ImGui::End();
}

void CLevelBossCharlesRookwood::FrameStart(E::_float fTimeDelta)
{

}

Engine::UPtr<CLevelBossCharlesRookwood> CLevelBossCharlesRookwood::Create()
{
	auto pInstance = Engine::UPtr<CLevelBossCharlesRookwood>(new CLevelBossCharlesRookwood{});
	pInstance->SetDeferredInitialization();
	return pInstance;
}

HRESULT CLevelBossCharlesRookwood::SpawnFlyCamera()
{
	{
		E::CCameraObject::CAMERA_DESC Desc{};
		Desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
		Desc.vAt = { 0.f, 0.f, 0.f };
		Desc.vEye = { 0.f, 0.f, -5.f };
		Desc.fAspect = { g_iWinSizeX / (E::_float)g_iWinSizeY };
		Desc.fFovY = 75.f;
		Desc.fNear = 0.1f;
		Desc.fFar = 1000.f;
		Desc.sObjectTag = "FlyCam";

		if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_FlyCamera",
			"99_CAMERA", &Desc))
		{
			if (FAILED(E::CGameInstance::Get().RegistCamera("FLY", flyCam.value())))
			{
				MSG_BOX("FailedToRegistCamera");
				return E_FAIL;
			}
		}
	}
	return S_OK;
}

HRESULT CLevelBossCharlesRookwood::SpawnUICamera()
{
	{
		E::CCameraObject::CAMERA_DESC Desc{};
		Desc.eProj = E::CCameraObject::PROJ::ORTHOGRAPHIC;
		Desc.fNear = 0.f;
		Desc.fFar = 1.f;
		Desc.fWidth = g_iWinSizeX;
		Desc.fHeight = g_iWinSizeY;
		Desc.sObjectTag = "UICam";
		Desc.vEye = { 0.f, 0.f, -0.1f };

		if (auto uiCam = E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_UICamera",
			"99_CAMERA", &Desc))
		{
			if (FAILED(E::CGameInstance::Get().RegistCamera("UI", uiCam.value())))
			{
				MSG_BOX("FailedToRegistCamera");
				return E_FAIL;
			}
		}
	}
	return S_OK;
}

HRESULT CLevelBossCharlesRookwood::SpawnPlayerCamera(std::optional<CHandle> hPlayer)
{
	if (!hPlayer) return E_FAIL;
	CPlayerThirdPersonCamera::DESC Desc{};
	Desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
	Desc.vAt = { 10.f, 50.f, 10.f };
	Desc.vEye = { 10.f, 53.f, 5.f };
	Desc.fAspect = { g_iWinSizeX / (E::_float)g_iWinSizeY };
	Desc.fFovY = 75.f;
	Desc.fNear = 0.1f;
	Desc.fFar = 1000.f;
	Desc.sObjectTag = "PlayerCamera";
	Desc.hTarget = hPlayer.value();
	Desc.fYaw = 90.f;

	auto hPlayerCamera = E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::BOSS_CHARLES_ROOKWOOD,
		PROTO_GAMEOBJECT::Prototype_GameObject_PlayerThirdPersonCamera,
		"101_CAMERA",
		&Desc);
	if (!hPlayerCamera || FAILED(E::CGameInstance::Get().RegistCamera(
		"PlayerCamera", *hPlayerCamera)))
	{
		return E_FAIL;
	}
	E::CGameInstance::Get().SetActiveCamera("PlayerCamera");
	return S_OK;
}

std::optional<CHandle> CLevelBossCharlesRookwood::SpawnPlayer()
{
	CPlayer::DESC PlayerDesc{};
	PlayerDesc.sObjectTag = "Player";
	PlayerDesc.vInitialPosition = { -80.f, 20.f, 10.f };
	PlayerDesc.vInitialRotation = { 0.f, 90.f, 0.f };
	PlayerDesc.LevelTag = LEVEL::BOSS_CHARLES_ROOKWOOD;
	PlayerDesc.tFilter = PX_FILTER_DESC{
	 .iLayer = ETOUI(COLLISION_LAYER::PLAYER_BODY),
	.iSimulationMask = PX_ALL_LAYERS,
	.iQueryMask =
		ETOUI(COLLISION_LAYER::WORLD_STATIC) |
		ETOUI(COLLISION_LAYER::MOVING_PLATFORM) |
		ETOUI(COLLISION_LAYER::ENEMY_BODY)
	};
	return  E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::BOSS_CHARLES_ROOKWOOD,
		PROTO_GAMEOBJECT::Prototype_GameObject_Player,
		"03_Player",
		&PlayerDesc);
}

HRESULT CLevelBossCharlesRookwood::SpawnPlayerCape(CHandle hPlayer)
{
	CNvClothCape::DESC Desc{};
	Desc.sObjectTag = "NvClothCape";
	Desc.hTarget = hPlayer;
	Desc.sResourceGroup = LEVEL::BOSS_CHARLES_ROOKWOOD;
	Desc.sModelResourceTag = "PLAYER_CAPE_MODEL_RESOURCE";
	Desc.sClothMeshResourceTag = "PLAYER_CAPE_CLOTH_RESOURCE";
	Desc.sTargetModelComponentTag = "ComCModelIntance";
	Desc.sAttachBoneName = "Spine3";
	Desc.vLocalPosition = { 0.05f, 0.08f, 0.f };

	E::CGameInstance::Get().JsonDeSerialize(
		"./Resources/NvCloth/CollisionRigs/ProfessorCape.nvclothcollision.json",
		Desc.tBodyCollisionRig,
		E::NVCLOTH_COLLISION_RIG_ROOT,
		false);
	E::CGameInstance::Get().JsonDeSerialize(
		"./Resources/NvCloth/CollisionRigs/ProfessorCape_Broom.nvclothcollision.json",
		Desc.tBroomBodyCollisionRig,
		E::NVCLOTH_COLLISION_RIG_ROOT,
		false);
	E::CGameInstance::Get().JsonDeSerialize(
		"./Resources/NvCloth/CollisionRigs/ProfessorCape_BroomObject.nvclothcollision.json",
		Desc.tBroomObjectCollisionRig,
		E::NVCLOTH_COLLISION_RIG_ROOT,
		false);

	if (auto hCape = E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::BOSS_CHARLES_ROOKWOOD,
		PROTO_GAMEOBJECT::Prototype_GameObject_NvClothCape,
		"03_Player",
		&Desc))
	{
		if(!hCape)
			return E_FAIL;

		if (auto pPlayer = CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(hPlayer))
		{
			pPlayer->SetCapeHandle(hCape.value());
		}
	}

	return S_OK;
}

//HRESULT CLevelBossCharlesRookwood::SpawnAmbientSound()
//{
//	CAmbientSound2DObject::DESC desc{};
// 	desc.sObjectTag = "Ambient_Wind";
//
//	desc.tSoundData.sBusID = SOUND_BUS::BGM;
//	desc.tSoundData.eLoadType = SOUND_LOAD_TYPE::STREAM;
//	desc.tSoundData.sName = "Bgm";
//	desc.tSoundData.sSoundPath = "./Resources/SampleClient/Sound/BossCharlesRookwood/Ambient/Guardians_Awaken.wav";
//	desc.tSoundData.fVolume = 0.8f;
//	desc.tSoundData.fFadeInDuration = 1.f;
//	desc.tSoundData.fFadeOutDuration = 1.f;
//	desc.tSoundData.bLoop = true;
//	desc.tSoundData.bAutoPlay = true;
//	const auto hAmbientSound = CGameInstance::Get().AddGameObjectToLayer(
//		ES_EngineProtoMajorType::PERMANENT,
//		ES_EngineProtoGameObject::Prototype_GameObject_AmbientSound2D,
//		"Layer_AmbientSound",
//		&desc);
//	if (!hAmbientSound)
//	{
//		return E_FAIL;
//	}
//
//	m_hAmbientSound = *hAmbientSound;
//	return S_OK;
//}
//
//void CLevelBossCharlesRookwood::FadeOutAmbientSound()
//{
//	if (auto* pAmbientSound = CGameInstance::Get().
//		GetGameObjectByHandleT<CAmbientSound2DObject>(m_hAmbientSound))
//	{
//		pAmbientSound->FadeOutAndDetach();
//	}
//
//	m_hAmbientSound = {};
//}

HRESULT CLevelBossCharlesRookwood::PlayBGM()
{
	const _string sSoundPath = "./Resources/SampleClient/Sound/BossCharlesRookwood/Ambient/Guardians_Awaken.wav";
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (pSoundManager == nullptr || !pSoundManager->Preload(sSoundPath))
		return E_FAIL;

	m_bmgID = pSoundManager->Play2D(sSoundPath,
		E::SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::BGM,
			.fVolume = 1.f,
			.fPitch = 1.f,
			.fFadeInDuration = 1.f,
			.iPriority = 64,
			.bLoop = true
		});
	if (m_bmgID == INVALID_SOUND_ID)
		return E_FAIL;

	return S_OK;
}

HRESULT CLevelBossCharlesRookwood::StopBGM(_float fDuration)
{
	if (m_bmgID == INVALID_SOUND_ID)
		return S_OK;

	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (pSoundManager == nullptr ||
		!pSoundManager->FadeOutAndStop(m_bmgID, fDuration))
		return E_FAIL;

	m_bmgID = INVALID_SOUND_ID;

	return S_OK;
}

void CLevelBossCharlesRookwood::SubscribePlayerDeath(const CHandle& hPlayer)
{
	m_hPlayer = hPlayer;
	m_iPlayerDeathListenerID = CGameInstance::Get().EventSubscribe<FPlayerDied>(
		m_hPlayer,
		[this](const FPlayerDied& Event)
		{
			if (Event.hPlayer != m_hPlayer)
				return;

			StopBGM(Event.fLevelBgmFadeDuration);
		});
}

HRESULT CLevelBossCharlesRookwood::SpawnSkyBox()
{
	CSkyCloudyCube::SKY_DESC skyDesc{};
	skyDesc.sObjectTag = "SkyCloudyCube";
	if (!CGameInstance::Get().AddGameObjectToLayer("PERMANENT", "Prototype_GameObject_SkyCloudyCube", "00_SKYBOX", &skyDesc))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CLevelBossCharlesRookwood::Initialize_VolumetricFog(){

	CB_VLFOG FogOption{};

	FogOption.g_fFogColor			= { 63.f  / 255.f, 88.f  / 255.f, 88.f  / 255.f };
	FogOption.g_fFogIntensity		= 1.f;
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

HRESULT CLevelBossCharlesRookwood::Initialize_EnviromentLight(){

	CB_ENVLIGHT EnviromentLightOption{};

	EnviromentLightOption.m_fEnviromentIntensity	= 0.75f;
	EnviromentLightOption.m_fFillLightBrightness	= 0.25f;
	EnviromentLightOption.m_fDirectLightBrightness	= 0.60f;

	CGameInstance::Get().Set_EnviromentLight(EnviromentLightOption);

	return S_OK;
}

HRESULT CLevelBossCharlesRookwood::SpawnStaticCollision()
{
	auto handles = CGameInstance::Get()
		.GetPhysXManager()
		->CreateCollisionProxyObjectsFromFile(
			"Level_BossCharlesRookwood",
			"00_MapCollision");

	if (handles.empty())
		return E_FAIL;

	return S_OK;
}

HRESULT CLevelBossCharlesRookwood::SpawnLightPlacement()
{
	CLightPlacementObject::DESC desc{};
	desc.sObjectTag =
		"BossCharlesRookwoodLightPlacement";
	desc.sLightFileName =
		"BossStage_LightProtoType_A";

	return CGameInstance::Get().
		AddGameObjectToLayer(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoGameObject::
				Prototype_GameObject_LightPlacement,
			"Layer_LightPlacement",
			&desc)
		? S_OK
		: E_FAIL;
}

HRESULT CLevelBossCharlesRookwood::SpawnMonster(std::optional<CHandle> hPlayer)
{
	{
		CBossTMB::TMB_DESC TmbDesc{};
		TmbDesc.TargetHandle = hPlayer.value();
		TmbDesc.sObjectTag = "BossTmb";
		TmbDesc.LevelTag = MagicEnumToStringView(LEVEL::BOSS_CHARLES_ROOKWOOD);
		XMStoreFloat3(&TmbDesc.vPos, XMVectorSet(-28, 15, 7, 1));
		TmbDesc.ReSourceTag = "Model_Resource_TombBoss";
		TmbDesc.resBeHaviorMajor = "BTJSON";
		TmbDesc.resBeHaviorMinor = "TOMB_BT_TOMBBOSS";
		TmbDesc.WeaponProtoName = MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_BossWeapon);
		TmbDesc.WeaponResourceName = "Model_Resource_BossWeapon";
		TmbDesc.vWeaponScale = _float3(1.f, 1.f, 1.f);
		TmbDesc.vScale = _float3(6.f, 6.f, 6.f);
		auto BossTmb = E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_BossTMB, "02_BossTmb", &TmbDesc);

		if (!BossTmb)
		{
			MSG_BOX("Create BossTmb Failed in Rookwood");
			return E_FAIL;
		}

		m_hBoss = *BossTmb;
	}

	return S_OK;
}

void CLevelBossCharlesRookwood::Free()
{
	if (m_iPlayerDeathListenerID != 0)
	{
		CGameInstance::Get().EventUnsubscribe<FPlayerDied>(m_iPlayerDeathListenerID);
		m_iPlayerDeathListenerID = 0;
	}

	StopBGM();
	CLevel::Free();
}
