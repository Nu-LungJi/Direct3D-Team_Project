#include "pch.h"
#include "LevelPlayground.h"
#include "GameInstance.h"
#include "LevelLoading.h"
#include "Level_Defines.h"
#include "FlyCamera.h"
#include "ResCBuffer.h"
#include "BackGround.h"
#include "UiCamera.h"
#include "Terrain.h"
#include "Particle.h"
#include "TestModel.h"
#include "TestGob.h"
#include "LightObject.h"
#include "Weapon.h"
#include "LevelPlayGroundLoader.h"
NS_USING(Client)

CLevelPlayground::CLevelPlayground()
	: CLevel{ ETOUI(LEVEL::PLAYGROUND) }
{
}

CLevelPlayground::~CLevelPlayground()
{
}

HRESULT CLevelPlayground::Initialize()
{
	Engine::CGameInstance::Get().GameObjectAllReset();
	{
		CTerrain::DESC Desc{};
		Desc.sObjectTag = "Terrain";

		if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_PLAYGROUND", "Prototype_GameObject_Terrain",
			"01_Terrain", &Desc))
		{
			int x = 0;
		}
	}

	{
		E::CCameraObject::CAMERA_DESC Desc{};
		Desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
		Desc.vAt = { 0.f, 0.f, 0.f };
		Desc.vEye = { 0.f, 0.f, -5.f };
		Desc.fAspect = { g_iWinSizeX / (E::_float)g_iWinSizeY };
		Desc.fFovY = 75.f;
		Desc.fNear = 0.1f;
		Desc.fFar = 100.f;
		Desc.sObjectTag = "FlyCam";

		if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_FlyCamera",
			"99_CAMERA", &Desc))
		{
			if (FAILED(E::CGameInstance::Get().RegistCamera("FLY", flyCam.value())))
			{
				int x = 0;
			}
			E::CGameInstance::Get().SetActiveCamera("FLY");
		}
	}
	{
		//테스트 고블린
		CTestGob::MONSTER_DESC Desc{};
		Desc.sObjectTag = "Gobline";
		Desc.LevelTag = "LEVEL_PLAYGROUND";
		Desc.ReSourceTag = "Model_Resource_TombProtector";
		auto Gobline = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_PLAYGROUND", "Prototype_GameObject_Gobline", "02_Gobline", &Desc);
		if (!Gobline.has_value())
		{
			MSG_BOX("Craete Failed Gobline");
			return E_FAIL;
		}
		//테스트 고블린 무기 테스트

		CTestGob::MONSTER_DESC MonDesc{};

		MonDesc.bDonMove = true;
		MonDesc.sObjectTag = "Gobline";
		MonDesc.LevelTag = "LEVEL_PLAYGROUND";
		XMStoreFloat3(&MonDesc.vPos, XMVectorSet(0, 0, 3, 1));
		XMStoreFloat3(&MonDesc.vRot, XMVectorSet(0, 1, 0, 1));
		MonDesc.fAngle = 180.f;
		MonDesc.ReSourceTag = "Model_Resource_TombProtector";
		MonDesc.BeHaviorTag = "./Resources/json/BeHavior/BossDef.json";
		XMStoreFloat3(&MonDesc.vScale, XMVectorSet(6.f, 6.f, 6.f, 1));

		auto testBoss = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_PLAYGROUND", "Prototype_GameObject_Gobline", "02_Gobline", &MonDesc);
		
		CWeapon::WEAPON_DESC WeaponDesc{};

		WeaponDesc.sObjectTag = "Weapon";
		WeaponDesc.LevelTag = "LEVEL_PLAYGROUND";
		WeaponDesc.WeaponName = "Static_Wand_Model_Resource";
		WeaponDesc.vScale = _float3(300, 300, 300);
		auto Weapon = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_PLAYGROUND", "Prototype_GameObject_Wand", "03_Weapon", &WeaponDesc);
		if (!Weapon.has_value())
		{
			MSG_BOX("Create Failed Wand");
			return E_FAIL;
		}
	}
	CGameInstance::Get().Add_DirectionalLight({ 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, 10.f);

	if (FAILED(E::CGameInstance::Get().Initialize_EffectLight(MAX_EFFECTLIGHT_COUNT))) {
		MSG_BOX("Cannot Initialize EffectLight.");
	}	// 이펙트용 라이트 풀 생성

	return S_OK;
}

void CLevelPlayground::Update(E::_float fTimeDelta)
{
}

HRESULT CLevelPlayground::Render()
{
	return S_OK;
}

void CLevelPlayground::UpdateGUI()
{
	ImGui::Begin("LEVEL: CLevel_Logo");
	//if (ImGui::Button("ChangeLevelTo: GamePlay"))
	//{
	//	if (FAILED(Engine::CGameInstance::Get().ChangeLevel(CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::GAMEPLAY))))
	//	{
	//		MSG_BOX("ChangeLevelTo: GamePlay Failed");
	//	}
	//}



	ImGui::End();
}

void CLevelPlayground::FrameStart(E::_float fTimeDelta)
{

}

Engine::UPtr<CLevelPlayground> CLevelPlayground::Create()
{
	auto	pInstance = Engine::UPtr<CLevelPlayground>(new CLevelPlayground{});

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevel_Logo");
	}

	return pInstance;
}

void CLevelPlayground::Free()
{
	CLevel::Free();

}
