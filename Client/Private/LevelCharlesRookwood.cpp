#include "pch.h"
#include "LevelCharlesRookwood.h"
#include "GameInstance.h"
#include "Level_Defines.h"
#include "FlyCamera.h"

#include "ResCBuffer.h"
#include "BackGround.h"
#include "UiCamera.h"

#include "LevelCharlesRookwoodLoader.h"

NS_USING(Client)

CLevelCharlesRookwood::CLevelCharlesRookwood()
	: CLevel{ ETOUI(LEVEL::CHARLES_ROOKWOOD) }
{
}

CLevelCharlesRookwood::~CLevelCharlesRookwood()
{
}

HRESULT CLevelCharlesRookwood::Initialize()
{
	E::CGameInstance::Get().GameObjectAllReset();

	CGameInstance::Get().LoadMap("./Resources/json/MapSaved/Tomb12345", true);

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
				MSG_BOX("FailedToRegistCamera");
			}
		}
	}


	return S_OK;
}

void CLevelCharlesRookwood::Update(E::_float fTimeDelta)
{
}

HRESULT CLevelCharlesRookwood::Render()
{
	return S_OK;
}

void CLevelCharlesRookwood::UpdateGUI()
{
	ImGui::Begin("level: CharlesRookwood");

	ImGui::End();
}

void CLevelCharlesRookwood::FrameStart(E::_float fTimeDelta)
{

}

Engine::UPtr<CLevelCharlesRookwood> CLevelCharlesRookwood::Create()
{
	auto	pInstance = Engine::UPtr<CLevelCharlesRookwood>(new CLevelCharlesRookwood{});

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevel_Logo");
	}

	return pInstance;
}

void CLevelCharlesRookwood::Free()
{
	CLevel::Free();
}
