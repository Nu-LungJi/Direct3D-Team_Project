#include "pch.h"
#include "LevelPercival.h"
#include "GameInstance.h"
#include "Level_Defines.h"
#include "FlyCamera.h"

#include "ResCBuffer.h"
#include "BackGround.h"
#include "UiCamera.h"

#include "LevelPercivalLoader.h"

NS_USING(Client)

CLevelPercival::CLevelPercival()
	: CLevel{ ETOUI(LEVEL::PERCIVAL) }
{
}

CLevelPercival::~CLevelPercival()
{
}

HRESULT CLevelPercival::Initialize()
{
	E::CGameInstance::Get().GameObjectAllReset();

	//{
	//	CBackGround::UIOBJECT_DESC Desc{};
	//	Desc.sObjectTag = "BackGround";
	//	if (!(E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_PERCIVAL", "Prototype_GameObject_BackGround",
	//		"00_OBJECTS", &Desc)))
	//	{
	//		return E_FAIL;
	//	}
	//}

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

void CLevelPercival::Update(E::_float fTimeDelta)
{
}

HRESULT CLevelPercival::Render()
{
	return S_OK;
}

void CLevelPercival::UpdateGUI()
{
	ImGui::Begin("level: Persibal");

	ImGui::End();
}

void CLevelPercival::FrameStart(E::_float fTimeDelta)
{

}

Engine::UPtr<CLevelPercival> CLevelPercival::Create()
{
	auto	pInstance = Engine::UPtr<CLevelPercival>(new CLevelPercival{});

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevel_Logo");
	}

	return pInstance;
}

void CLevelPercival::Free()
{
	CLevelPercivalLoader::UnLoad();
	CLevel::Free();
}
