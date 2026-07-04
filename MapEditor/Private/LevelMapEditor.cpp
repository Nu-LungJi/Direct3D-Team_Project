#include "pch.h"
#include "LevelLogo.h"
#include "GameInstance.h"
#include "LevelMapEditor.h"
#include "FlyCamera.h"
#include "ResCBuffer.h"
#include "UiCamera.h"


#include "TestGuizmo.h"

NS_USING(Client)

CLevelMapEditor::CLevelMapEditor()

{
}

CLevelMapEditor::~CLevelMapEditor()
{
}

HRESULT CLevelMapEditor::Initialize()
{
	Engine::CGameInstance::Get().GameObjectAllReset();

	{
		CTestGuizmo::GAMEOBJECT_DESC Desc{};
		Desc.sObjectTag = "TestGuizmo";
		if (auto hObject = E::CGameInstance::Get().AddGameObjectToLayer("MAPEDITOR", "Prototype_GameObject_TestGuizmo",
			"00_OBJECTS", &Desc))
		{
			m_SelectedObject = hObject.value();
		}
		else
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
				MSG_BOX("MSG_BOX_123_");
			}
			//E::CGameInstance::Get().SetActiveUICamera("UI");
		}
	}

	m_pMapEditorGUI = CMapEditorGUI::Create(&m_SelectedObject);
	if (m_pMapEditorGUI == nullptr)
	{
		return E_FAIL;
	}

	return S_OK;
}

void CLevelMapEditor::Update(E::_float fTimeDelta)
{
}

HRESULT CLevelMapEditor::Render()
{
	return S_OK;
}

void CLevelMapEditor::UpdateGUI()
{
	m_pMapEditorGUI->UpdateGUI(0.f);
}

void CLevelMapEditor::FrameStart(E::_float fTimeDelta)
{

}

Engine::UPtr<CLevelMapEditor> CLevelMapEditor::Create()
{
	auto	pInstance = Engine::UPtr<CLevelMapEditor>(new CLevelMapEditor{});

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevel_MapEditor");
	}

	return pInstance;
}

void CLevelMapEditor::Free()
{
	E::CGameInstance::Get().DelPrototype("LEVEL_MAPEDITOR");
	E::CGameInstance::Get().DelResource("LEVEL_MAPEDITOR");
	CLevel::Free();
}
