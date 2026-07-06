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
#include "Gobline.h"
#include "LightObject.h"
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
		//테스트 고블린
		CGameObject::GAMEOBJECT_DESC Desc{};
		Desc.sObjectTag = "Gobline";

		if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_PLAYGROUND", "Prototype_GameObject_Gobline",
			"02_Gobline", &Desc))
		{
			int x = 0;
		}
	}
	{
		CLightObject::DESC LDesc{};
		LDesc.sObjectTag = "LightObject";
		auto ObjectHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_PLAYGROUND", "Prototype_GameObject_LightObject", "01_LightObject", &LDesc);
		auto LightObject = E::CGameInstance::Get().GetGameObjectByHandle(ObjectHandle.value());
		if (!LightObject)	return E_FAIL;
	
		LightObject->GetComponent<CComTransform>("Com_Transform")->SetScale(XMVectorSet(70.f, 70.f, 70.f, 1.f));
		LightObject->GetComponent<CComTransform>("Com_Transform")->SetPosition(XMVectorSet(1.f, 10.f, 0.f, 1.f));
	}
	{
		//if(false)
		//{
		//	CTestModel::DESC Desc{};
		//	Desc.sObjectTag = "TestModel";
		//
		//	if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_PLAYGROUND", "Prototype_GameObject_TestModel",
		//		"02_TestModel", &Desc))
		//	{
		//		int x = 0;
		//	}
		//}

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

	//{
	//	E::CCameraObject::CAMERA_DESC Desc{};
	//	Desc.eProj = E::CCameraObject::PROJ::ORTHOGRAPHIC;
	//	Desc.fNear = 0.f;
	//	Desc.fFar = 1.f;
	//	Desc.fWidth = g_iWinSizeX;
	//	Desc.fHeight = g_iWinSizeY;
	//	Desc.sObjectTag = "UICam";
	//	Desc.vEye = { 0.f, 0.f, -0.1f };
	//
	//	if (auto uiCam = E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_UICamera",
	//		"99_CAMERA", &Desc))
	//	{
	//		if (FAILED(E::CGameInstance::Get().RegistCamera("UI", uiCam.value())))
	//		{
	//			MSG_BOX("Deb");
	//		}
	//	}
	//}
	CGameInstance::Get().Add_DirectionalLight({ 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, 10.f);


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
