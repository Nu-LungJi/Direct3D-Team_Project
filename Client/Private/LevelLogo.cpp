#include "pch.h"
#include "LevelLogo.h"
#include "GameInstance.h"
#include "LevelLoading.h"
#include "Level_Defines.h"

#include "FlyCamera.h"

#include "ResCBuffer.h"
#include "BackGround.h"
#include "UiCamera.h"

#include "LevelLogoLoader.h"

NS_USING(Client)

CLevelLogo::CLevelLogo()
	: CLevel{ ETOUI(LEVEL::LOGO) }
{
}

CLevelLogo::~CLevelLogo()
{
}

HRESULT CLevelLogo::Initialize()
{
	E::CGameInstance::Get().GameObjectAllReset();
	//Engine::CGameInstance::Get().GameObjectLayerInitialize(E::ETOUI(LEVEL_LOADING_LAYERS::END), LevelLoadingLayersToString);

	{
		CBackGround::UIOBJECT_DESC Desc{};
		Desc.sObjectTag = "BackGround";
		if (!(E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_LOGO", "Prototype_GameObject_BackGround",
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

			// dynamic_cast vs static_cast benchmark 
			// dynamic_cast: 472ms, static_cast: 41ms
			if constexpr (false)
			{
				E::CGameObject* volatile val = E::CGameInstance::Get().GetGameObjectByHandle(uiCam.value());
				{
					auto start = std::chrono::high_resolution_clock::now();
					{
						E::CUICamera* volatile sink = nullptr;
						for (size_t i = 0; i < 10'000'000; ++i)
						{
							sink = dynamic_cast<E::CUICamera*>(val);
						}
					}
					auto end = std::chrono::high_resolution_clock::now();
					auto cost = std::chrono::duration<double, std::milli>(end - start).count();
					MSG_BOX_STR(std::to_wstring(cost).c_str());
				}
				{
					auto start = std::chrono::high_resolution_clock::now();
					{
						E::CUICamera* volatile sink = nullptr;
						for (size_t i = 0; i < 10'000'000; ++i)
						{
							if (val->IsA(E::CUICamera::StaticType))
							{
								sink = static_cast<E::CUICamera*>(val);
							}
							else
							{
								sink = nullptr;
							}
						}
					}
					auto end = std::chrono::high_resolution_clock::now();
					auto cost = std::chrono::duration<double, std::milli>(end - start).count();
					MSG_BOX_STR(std::to_wstring(cost).c_str());
				}
			}
			{
				const auto* t = E::CGameInstance::GetConst().GetGameObjectByHandleT<E::CUICamera>(uiCam.value());

				auto* t2 = E::CGameInstance::Get().GetGameObjectByHandleT<E::CUICamera>(uiCam.value());
			}
		}


	}


	return S_OK;
}

void CLevelLogo::Update(E::_float fTimeDelta)
{
}

HRESULT CLevelLogo::Render()
{
	return S_OK;
}

void CLevelLogo::UpdateGUI()
{
	ImGui::Begin("LEVEL: CLevel_Logo");
	ImGui::End();
}

void CLevelLogo::FrameStart(E::_float fTimeDelta)
{

}

Engine::UPtr<CLevelLogo> CLevelLogo::Create()
{
	auto pInstance = Engine::UPtr<CLevelLogo>(new CLevelLogo{});
	pInstance->SetDeferredInitialization();
	return pInstance;
}

void CLevelLogo::Free()
{
	CLevel::Free();
}
