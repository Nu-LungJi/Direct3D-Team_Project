#include "pch.h"
#include "LevelUIEditor.h"
#include "GameInstance.h"
#include "LevelLoading.h"

#include "FlyCamera.h"
#include "UiCamera.h"
#include "ResCBuffer.h"
#include "ResTexture2D.h"

NS_USING(Client)

CLevelUIEditor::CLevelUIEditor()
{
}

CLevelUIEditor::~CLevelUIEditor()
{
}

HRESULT CLevelUIEditor::Initialize()
{
	E::CGameInstance::Get().GameObjectAllReset();

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

void CLevelUIEditor::Update(E::_float fTimeDelta)
{
}

HRESULT CLevelUIEditor::Render()
{
	return E_NOTIMPL;
}

void CLevelUIEditor::UpdateGUI()
{
	ImGui::Begin("LEVEL: CLevelUIEditor"); 
	
	const auto& srv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>("LEVEL_UIEDITOR", "TEX_SHM");
	const auto& srv2 = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>("LEVEL_UIEDITOR", "TEX_MAP");

	if (ImGui::ImageButton((ImTextureID)srv->GetSRV().Get(), ImVec2(100, 100)))
	{
		// 버튼 클릭
	}

	ImGui::SameLine();

	if (ImGui::ImageButton((ImTextureID)srv2->GetSRV().Get(), ImVec2(100, 100)))
	{
		// 버튼 클릭
	}

	ImGui::End();
}

void CLevelUIEditor::FrameStart(E::_float fTimeDelta)
{
}

Engine::UPtr<CLevelUIEditor> CLevelUIEditor::Create()
{
	auto	pInstance = Engine::UPtr<CLevelUIEditor>(new CLevelUIEditor{});

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevelUIEditor");
	}

	return pInstance;
}

void CLevelUIEditor::Free()
{
	E::CGameInstance::Get().DelPrototype("LEVEL_UIEditor");
	E::CGameInstance::Get().DelResource("LEVEL_UIEditor");
	CLevel::Free();
}
