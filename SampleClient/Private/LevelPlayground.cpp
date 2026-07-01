#include "pch.h"
#include "LevelPlayground.h"
#include "GameInstance.h"
#include "LevelLoading.h"
#include "FlyCamera.h"
#include "ResCBuffer.h"
#include "BackGround.h"
#include "UiCamera.h"
#include "Terrain.h"
#include "Particle.h"
#include "TestModel.h"


NS_USING(Client)

CLevelPlayground::CLevelPlayground()

{
}

CLevelPlayground::~CLevelPlayground()
{
}

HRESULT CLevelPlayground::Initialize()
{
	Engine::CGameInstance::Get().GameObjectAllReset();
	
	// Terrain
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
		//제거함 일단 오류나서
		if(false)
		{
			CTestModel::DESC Desc{};
			Desc.sObjectTag = "TestModel";

			if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_PLAYGROUND", "Prototype_GameObject_TestModel",
				"02_TestModel", &Desc))
			{
				int x = 0;
			}
		}

	}

	// Particle //문제있는 코드. 접근금지
	if(false){
		CParticle::GAMEOBJECT_DESC Desc{};
		Desc.sObjectTag = "Particle";

		if (auto particle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_PLAYGROUND", "Prototype_GameObject_Particle",
			"01_Particle", &Desc))
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
				int x = 0;
			}
		}
	}


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
