#include "pch.h"
#include "LevelAnimEditor.h"
#include "GameInstance.h"
#include "LevelLoading.h"
#include "Level_Defines.h"
#include "FlyCamera.h"
#include "ResCBuffer.h"
#include "BackGround.h"
#include "TestModel.h"
#include "Test_StaticModel.h"
#include "LightObject.h"
#include "ComAnimator.h"
#include "LevelAnimatorLoader.h"
NS_USING(Client)

CLevelAnimEditor::CLevelAnimEditor()
	: CLevel{ ETOUI(LEVEL::ANIMEDITOR) }
{
}

CLevelAnimEditor::~CLevelAnimEditor()
{
}

HRESULT CLevelAnimEditor::Initialize()
{
	Engine::CGameInstance::Get().GameObjectAllReset();

	//{
	//	CTest_StaticModel::DESC Desc{};
	//	Desc.sObjectTag = "TestStaticModel";

	//	if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_TEST", "Prototype_GameObject_TestStaticModel",
	//		"TestStaticModelLayer", &Desc))
	//	{
	//		int x = 0;
	//	}
	//}

	//{
	//	CTest_StaticModel::DESC Desc{};
	//	Desc.sObjectTag = "TestStaticModel";

	//	if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_TEST", "Prototype_GameObject_TestStaticModel2",
	//		"TestStaticModelLayer", &Desc))
	//	{
	//		int x = 0;
	//	}
	//}


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
	CGameInstance::Get().Add_DirectionalLight({ 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, 1.f);


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
	ImGui::Begin("LEVEL: CLevelAnimEditor");


	if (ImGui::Button("Spawn TestModel x10"))
	{
	

		for (uint32_t i = 0; i < 10; ++i)
		{
			CTestModel::DESC Desc{};

			// 버튼을 여러 번 눌러도 이름이 겹치지 않게 설정
			Desc.sObjectTag ="TestModel_" + std::to_string(iTestCount++);

			auto addedObject =
				E::CGameInstance::Get().AddGameObjectToLayer(
					"LEVEL_TEST",
					"Prototype_GameObject_TestModel",
					"TestModelLayer",
					&Desc
				);

			if (!addedObject)
				continue;
	
			_float fRandomX = Randf(-50.f, 50.f);
			_float fRandomZ = Randf(-50.f, 50.f);
			auto pSampleObj = CGameInstance::Get().GetGameObjectByHandle(addedObject.value());
			auto& pTransform = pSampleObj->GetTransform();
		
			pTransform.SetPosition(_float3{fRandomX,0.f,fRandomZ});
			
			auto anim = pSampleObj->GetComponent<CComAnimator>("ComCModelAnimator");
			

			anim->Play_Anim((int32_t)Randf(0.f, 100.f), true, 0.2f);
		}
	}

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
	//
	//	

	
	CLevelAnimatorLoader::UnLoad();



	CGameInstance::Get().Clear_DynamicLightList();
	CLevel::Free();
}
