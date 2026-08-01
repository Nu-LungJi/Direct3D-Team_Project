#include "pch.h"
#include "LevelBossCharlesRookwood.h"
#include "GameInstance.h"
#include "Level_Defines.h"
#include "FlyCamera.h"

#include "ResCBuffer.h"
#include "BackGround.h"
#include "UiCamera.h"

#include "LevelCharlesRookwoodLoader.h"


#include "PlayerThirdPersonCamera.h"
#include "Player.h"

#include "BossTMB.h"
#include "UIManager.h"
#include "UIController.h"

#include "LightPlacementObject.h"
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
	E::CGameInstance::Get().GameObjectAllReset();
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
	auto	pInstance = Engine::UPtr<CLevelBossCharlesRookwood>(new CLevelBossCharlesRookwood{});

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevel_BossCharlesRookwood");
	}

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
	PlayerDesc.LevelTag = LEVEL::BOSS_CHARLES_ROOKWOOD;
	PlayerDesc.tFilter = PX_FILTER_DESC{
	 .iLayer = ETOUI(COLLISION_LAYER::PLAYER_BODY),
	.iSimulationMask = PX_ALL_LAYERS,
	.iQueryMask =
		ETOUI(COLLISION_LAYER::WORLD_STATIC) |
		ETOUI(COLLISION_LAYER::MOVING_PLATFORM)
	};
	return  E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::BOSS_CHARLES_ROOKWOOD,
		PROTO_GAMEOBJECT::Prototype_GameObject_Player,
		"03_Player",
		&PlayerDesc);
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
		"Level_BossCharlesRookwood";

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
		TmbDesc.ReSourceTag = "Model_Resource_TombProtector";
		TmbDesc.BeHaviorTag = "./Resources/json/BeHavior/TombBoss.json";
		XMStoreFloat3(&TmbDesc.vScale, XMVectorSet(6.f, 6.f, 6.f, 1));
		auto BossTmb = E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_BossTMB, "02_BossTmb", &TmbDesc);

		if (!BossTmb)
		{
			MSG_BOX("Create BossTmb Failed in Rookwood");
			return E_FAIL;
		}
	}

	return S_OK;
}

void CLevelBossCharlesRookwood::Free()
{
	CLevel::Free();
}
