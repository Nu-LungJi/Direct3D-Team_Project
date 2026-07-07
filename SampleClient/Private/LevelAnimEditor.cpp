#include "pch.h"
#include "LevelAnimEditor.h"
#include "GameInstance.h"
#include "LevelLoading.h"
#include "FlyCamera.h"
#include "ResCBuffer.h"
#include "BackGround.h"
#include "TestModel.h"
#include "Test_StaticModel.h"
#include "LightObject.h"

NS_USING(Client)

CLevelAnimEditor::CLevelAnimEditor()

{
}

CLevelAnimEditor::~CLevelAnimEditor()
{
}

HRESULT CLevelAnimEditor::Initialize()
{
	Engine::CGameInstance::Get().GameObjectAllReset();

	{
		CTest_StaticModel::DESC Desc{};
		Desc.sObjectTag = "TestStaticModel";

		if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_TEST", "Prototype_GameObject_TestStaticModel",
			"TestStaticModelLayer", &Desc))
		{
			int x = 0;
		}
	}


	{
		CTestModel::DESC Desc{};
		Desc.sObjectTag = "TestModel";

		if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_TEST", "Prototype_GameObject_TestModel",
			"TestModelLayer", &Desc))
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
	if (E::CGameInstance::Get().AddPrototype("LIGHT", "Prototype_GameObject_Light", CLight::Create()))	return E_FAIL;
	CGameInstance::Get().Add_DirectionalLight({ 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, 10.f);


	CGameInstance::Get().SetupTestModel();;
	return S_OK;
}

void CLevelAnimEditor::Update(E::_float fTimeDelta)
{
}

HRESULT CLevelAnimEditor::Render()
{
	return S_OK;
}

void CLevelAnimEditor::UpdateGUI()
{
	ImGui::Begin("LEVEL: CLevel_Anim");
	//if (ImGui::Button("ChangeLevelTo: GamePlay"))
	//{
	//	if (FAILED(Engine::CGameInstance::Get().ChangeLevel(CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::GAMEPLAY))))
	//	{
	//		MSG_BOX("ChangeLevelTo: GamePlay Failed");
	//	}
	//}



	ImGui::End();
}

void CLevelAnimEditor::FrameStart(E::_float fTimeDelta)
{

}



Engine::UPtr<CLevelAnimEditor> CLevelAnimEditor::Create()
{
	auto	pInstance = Engine::UPtr<CLevelAnimEditor>(new CLevelAnimEditor{});

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevelAnimEditor");
	}

	return pInstance;
}

void CLevelAnimEditor::Free()
{
	CGameInstance::Get().Clear_DynamicLightList();
	CLevel::Free();
}
