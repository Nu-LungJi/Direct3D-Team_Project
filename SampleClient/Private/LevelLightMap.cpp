#include "pch.h"
#include "LevelLightMap.h"
#include "GameInstance.h"
#include "LevelLoading.h"

#include "FlyCamera.h"

#include "ResCBuffer.h"
#include "BackGround.h"
#include "Light.h"
#include "Terrain.h"

NS_USING(Client)

CLevelLightMap::CLevelLightMap()
{
}

CLevelLightMap::~CLevelLightMap()
{
}

HRESULT CLevelLightMap::Initialize()
{
	Engine::CGameInstance::Get().GameObjectAllReset();

	{
		CLight::DESC LDesc{};
		LDesc.sObjectTag = "Light";
		if (!(E::CGameInstance::Get().AddGameObjectToLayer("LIGHT", "Prototype_GameObject_Light",
			"00_LIGHTS", &LDesc)))
		{
			return E_FAIL;
		}
	}
	{
		CTerrain::DESC Desc{};
		Desc.sObjectTag = "Terrain";

		if (!(E::CGameInstance::Get().AddGameObjectToLayer("LIGHT", "Prototype_GameObject_Terrain",
			"01_Terrain", &Desc)))
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

	return S_OK;
}

void CLevelLightMap::Update(E::_float fTimeDelta)
{
}

HRESULT CLevelLightMap::Render()
{
	return S_OK;
}

void CLevelLightMap::UpdateGUI()
{
	ImGui::Begin("LEVEL: CLevelLightMap");



	ImGui::End();
}

void CLevelLightMap::FrameStart(E::_float fTimeDelta)
{

}

Engine::UPtr<CLevelLightMap> CLevelLightMap::Create()
{
	auto	pInstance = Engine::UPtr<CLevelLightMap>(new CLevelLightMap{});

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevelLightMap");
	}

	return pInstance;
}

void CLevelLightMap::Free()
{
	CLevel::Free();
}
