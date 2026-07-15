#include "pch.h"
#include "LevelPhysX.h"
#include "GameInstance.h"
#include "LevelLoading.h"

#include "FlyCamera.h"

#include "ResCBuffer.h"
#include "TestPhysX.h"
#include "UiCamera.h"



#include "TestPhysXTerrain.h"
#include "TestPhysXBox.h"
#include "TestCharacter.h"

#include "LevelPhysXLoader.h"

NS_USING(Client)

CLevelPhysX::CLevelPhysX()

{
}

CLevelPhysX::~CLevelPhysX()
{
}

enum class TestPhysXLayer
{
	_01_Terrain
};

HRESULT CLevelPhysX::Initialize()
{
	Engine::CGameInstance::Get().GameObjectAllReset();
	// Terrain
	if(true)
	{
		CTestPhysXTerrain::DESC Desc{};
		Desc.sObjectTag = "TestPhysXTerrain";

		if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("SAMPLE_CLIENT_PHYSX", "Prototype_GameObject_TestPhysXTerrain",
			TestPhysXLayer::_01_Terrain, &Desc))
		{
			int x = 0;
		}
	}

	//{
	//	//"SAMPLE_CLIENT_PHYSX", "Prototype_GameObject_TestPhysXBox"
	//	CTestPhysXBox::DESC Desc{ };
	//	Desc.sObjectTag = "TestPhysXBox";
	//	if (!(E::CGameInstance::Get().AddGameObjectToLayer("SAMPLE_CLIENT_PHYSX", "Prototype_GameObject_TestPhysXBox",
	//		"00_OBJECTS", &Desc)))
	//	{
	//		return E_FAIL;
	//	}
	//}

	{
		CTestPhysX::DESC Desc{ };
		Desc.sObjectTag = "TestPhysX";
		if (!(E::CGameInstance::Get().AddGameObjectToLayer("SAMPLE_CLIENT_PHYSX", "Prototype_GameObject_TestPhysX",
			"00_OBJECTS", &Desc)))
		{
			return E_FAIL;
		}
	}

	{
		CTestCharacter::DESC Desc{ };
		Desc.sObjectTag = "TestCharacter";
		if (!(E::CGameInstance::Get().AddGameObjectToLayer("SAMPLE_CLIENT_PHYSX", "Prototype_GameObject_TestCharacter",
			"00_OBJECTS", &Desc)))
		{
			return E_FAIL;
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
				MSG_BOX("MSG_BOX_123");
			}
			E::CGameInstance::Get().SetActiveCamera("FLY");
		}
	}

	if (E::CGameInstance::Get().AddPrototype("LIGHT", "Prototype_GameObject_Light", CLight::Create()))	return E_FAIL;
	CGameInstance::Get().Add_DirectionalLight({ 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, 10.f);

	return S_OK;
}

void CLevelPhysX::Update(E::_float fTimeDelta)
{
	
}

void CLevelPhysX::UpdateGUI()
{
	CLevel::UpdateGUI();
	if (ImGui::Button("Spawn"))
	{
		/*{
			auto shape = CComPhysX::SHAPE_TYPE::BOX;
			if (Randf(0.f, 1.f) > 0.5f)
			{
				shape = CComPhysX::SHAPE_TYPE::CAPSULE;
			}
			CComPhysX::DESC comPhysXDesc{};
			comPhysXDesc.eActorType = CComPhysX::ACTOR_TYPE::DYNAMIC;
			comPhysXDesc.eShapeType = shape;
			comPhysXDesc.vPosition = { Randf(-0.f, 55.f), 10.f, Randf(-0.f, 55.f) };
			comPhysXDesc.vHalfExtent = { 1.f, 1.f, 1.0f };
			comPhysXDesc.fRadius = 5.f;
			comPhysXDesc.fHalfHeight = 5.f;

			CTestPhysX::DESC Desc{ .comPhysXDesc = comPhysXDesc };
			Desc.sObjectTag = "TestPhysX";
			if (!(E::CGameInstance::Get().AddGameObjectToLayer("SAMPLE_CLIENT_PHYSX", "Prototype_GameObject_TestPhysX",
				"00_OBJECTS", &Desc)))
			{

			}
		}*/
	}
}

HRESULT CLevelPhysX::Render()
{
	return S_OK;
}


void CLevelPhysX::FrameStart(E::_float fTimeDelta)
{

}

Engine::UPtr<CLevelPhysX> CLevelPhysX::Create()
{
	auto	pInstance = Engine::UPtr<CLevelPhysX>(new CLevelPhysX{});

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevel_Logo");
	}

	return pInstance;
}

void CLevelPhysX::Free()
{
	CLevelPhysXLoader::UnLoad();
	CLevel::Free();
}
