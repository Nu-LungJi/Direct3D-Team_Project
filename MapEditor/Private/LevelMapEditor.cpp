#include "pch.h"
#include "LevelLogo.h"
#include "GameInstance.h"
#include "LevelMapEditor.h"
#include "FlyCamera.h"
#include "ResCBuffer.h"
#include "UiCamera.h"

NS_USING(Client)

namespace
{
	ImGuizmo::OPERATION g_GizmoOperation = ImGuizmo::TRANSLATE;
	ImGuizmo::MODE g_GizmoMode = ImGuizmo::WORLD;
	E::_float4x4 g_GizmoMatrix{};
	bool g_GizmoMatrixInitialized = false;

	void InitializeGizmoMatrix()
	{
		if (!g_GizmoMatrixInitialized)
		{
			XMStoreFloat4x4(&g_GizmoMatrix, XMMatrixIdentity());
			g_GizmoMatrixInitialized = true;
		}
	}

	void DrawGizmoToolbar()
	{
		if (ImGui::RadioButton("Translate", g_GizmoOperation == ImGuizmo::TRANSLATE))
		{
			g_GizmoOperation = ImGuizmo::TRANSLATE;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Rotate", g_GizmoOperation == ImGuizmo::ROTATE))
		{
			g_GizmoOperation = ImGuizmo::ROTATE;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Scale", g_GizmoOperation == ImGuizmo::SCALE))
		{
			g_GizmoOperation = ImGuizmo::SCALE;
		}

		if (g_GizmoOperation != ImGuizmo::SCALE)
		{
			if (ImGui::RadioButton("Local", g_GizmoMode == ImGuizmo::LOCAL))
			{
				g_GizmoMode = ImGuizmo::LOCAL;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("World", g_GizmoMode == ImGuizmo::WORLD))
			{
				g_GizmoMode = ImGuizmo::WORLD;
			}
		}
	}
}

CLevelMapEditor::CLevelMapEditor()

{
}

CLevelMapEditor::~CLevelMapEditor()
{
}

HRESULT CLevelMapEditor::Initialize()
{
	Engine::CGameInstance::Get().GameObjectAllReset();

	/*{
		CBackGround::UIOBJECT_DESC Desc{};
		Desc.sObjectTag = "BackGround";
		if (!(E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_LOGO", "Prototype_GameObject_BackGround",
			"00_OBJECTS", &Desc)))
		{
			return E_FAIL;
		}
	}*/

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
	InitializeGizmoMatrix();
	ImGuizmo::BeginFrame();

	ImGui::Begin("MapEditor Tools");
	DrawGizmoToolbar();
	ImGui::End();

	auto pActiveCamera = E::CGameInstance::Get().GetActiveCamera();
	if (pActiveCamera == nullptr)
	{
		return;
	}

	E::_float4x4 view{};
	E::_float4x4 proj{};
	XMStoreFloat4x4(&view, pActiveCamera->GetView());
	XMStoreFloat4x4(&proj, pActiveCamera->GetProj());

	ImGuiViewport* pViewport = ImGui::GetMainViewport();
	const ImVec2 viewportPos = pViewport->Pos;
	const ImVec2 viewportSize = pViewport->Size;

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList(pViewport));
	ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);
	ImGuizmo::Manipulate(&view._11, &proj._11, g_GizmoOperation, g_GizmoMode, &g_GizmoMatrix._11);
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
