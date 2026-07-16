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
#include "TestGob.h"
#include "LightObject.h"
#include "LevelPlayGroundLoader.h"
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
		CGameObject::GAMEOBJECT_DESC Desc{};
		Desc.sObjectTag = "Gobline";

		auto Gobline = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_PLAYGROUND", "Prototype_GameObject_Gobline", "02_Gobline", &Desc);
		if (!Gobline.has_value())
		{
			MSG_BOX("Craete Failed Gobline");
			return E_FAIL;
		}
		//테스트 고블린 무기 테스트
	}
	if (E::CGameInstance::Get().AddPrototype("LIGHT", "Prototype_GameObject_Light", CLight::Create()))	return E_FAIL;
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
	CLevelPlayGroundLoader::UnLoad();
	CGameInstance::Get().Clear_DynamicLightList();
	CLevel::Free();

}
