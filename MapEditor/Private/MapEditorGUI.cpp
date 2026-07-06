#include "pch.h"
#include "MapEditorGUI.h"
#include "GameInstance.h"

NS_USING(Client)

namespace
{
	constexpr const char* MAP_SAVE_ROOT = "./Resources/Engine/MapSaved/";

	std::string MakeMapPath(const char* mapName)
	{
		std::string cleanName = mapName;
		if (cleanName.empty())
		{
			cleanName = "Default";
		}

		for (char& ch : cleanName)
		{
			switch (ch)
			{
			case '/':
			case '\\':
			case ':':
			case '*':
			case '?':
			case '"':
			case '<':
			case '>':
			case '|':
				ch = '_';
				break;
			default:
				break;
			}
		}

		return std::string(MAP_SAVE_ROOT) + cleanName + "/";
	}

	bool DrawModeButton(const char* label, bool selected, const char* tooltip)
	{
		if (selected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.42f, 0.78f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.50f, 0.92f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.34f, 0.66f, 1.f));
		}

		const bool clicked = ImGui::Button(label, ImVec2(34.f, 0.f));

		if (selected)
		{
			ImGui::PopStyleColor(3);
		}

		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", tooltip);
		}

		return clicked;
	}

	void ApplyMatrixToTransform(E::CComTransform& transform, const E::_float4x4& matrix)
	{
		E::_vector scale{};
		E::_vector rotation{};
		E::_vector translation{};

		if (XMMatrixDecompose(&scale, &rotation, &translation, XMLoadFloat4x4(&matrix)))
		{
			transform.SetScale(scale);
			transform.SetQuaternion(rotation);
			transform.SetPosition(translation);
			transform.Update();
		}
	}
}

CMapEditorGUI::CMapEditorGUI()
{
}

CMapEditorGUI::~CMapEditorGUI()
{
}

void CMapEditorGUI::UpdateGUI(E::_float fTimeDelta)
{
	ImGuizmo::BeginFrame();

	ImGui::SetNextWindowSize(ImVec2(360.f, 520.f), ImGuiCond_FirstUseEver);
	ImGui::Begin("MapEditorGUI");
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, 7.f));

	ImGui::SetNextItemWidth(236.f);
	ImGui::InputText("Map", m_MapName, sizeof(m_MapName));

	if (ImGui::Button("Level Save", ImVec2(112.f, 0.f)))
	{
		CGameInstance::Get().SaveMap(MakeMapPath(m_MapName));
		ImGui::OpenPopup("SaveCheck");
	}
	ImGui::SameLine();
	if (ImGui::Button("Level Load", ImVec2(112.f, 0.f)))
	{
		CGameInstance::Get().LoadMap(MakeMapPath(m_MapName), true);
		AddCamera();
		ImGui::OpenPopup("LoadCheck");
	}

	if (ImGui::BeginPopupModal("SaveCheck", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted("Save Complete!");
		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120.f, 0.f)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	if (ImGui::BeginPopupModal("LoadCheck", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted("Load Complete!");
		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120.f, 0.f)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::Separator();
	DrawGizmoToolbar();

	ImGui::Separator();
	m_pHierarchy->UpdateGUI(fTimeDelta);

	ImGui::Separator();
	m_pInspector->UpdateGUI(fTimeDelta);

	ImGui::PopStyleVar(2);
	ImGui::End();

	m_pResourceGUI->UpdateGUI(fTimeDelta);
	m_pMapChunkGUI->UpdateGUI(fTimeDelta);
	RenderGizmo();
}

E::UPtr<CMapEditorGUI> CMapEditorGUI::Create(E::CHandle* pSelectedObject)
{
	auto pInstance = E::UPtr<CMapEditorGUI>(new CMapEditorGUI{});
	if (FAILED(pInstance->Initialize(pSelectedObject)))
	{
		MSG_BOX("Failed to Created : CMapEditorGUI");
		return nullptr;
	}

	pInstance->m_pHierarchy = CHierarchy::Create(pSelectedObject);
	if (pInstance->m_pHierarchy == nullptr)
	{
		return nullptr;
	}

	pInstance->m_pInspector = CInspector::Create(pSelectedObject);
	if (pInstance->m_pInspector == nullptr)
	{
		return nullptr;
	}

	pInstance->m_pResourceGUI = CResourceGUI::Create(pSelectedObject);
	if (pInstance->m_pResourceGUI == nullptr)
	{
		return nullptr;
	}

	pInstance->m_pMapChunkGUI = CMapChunkGUI::Create(pSelectedObject);
	if (pInstance->m_pMapChunkGUI == nullptr)
	{
		return nullptr;
	}

	return pInstance;
}

void CMapEditorGUI::DrawGizmoToolbar()
{
	ImGui::TextDisabled("Gizmo");
	ImGui::SameLine();
	if (DrawModeButton("T", m_GizmoOperation == ImGuizmo::TRANSLATE, "Translate"))
	{
		m_GizmoOperation = ImGuizmo::TRANSLATE;
	}
	ImGui::SameLine();
	if (DrawModeButton("R", m_GizmoOperation == ImGuizmo::ROTATE, "Rotate"))
	{
		m_GizmoOperation = ImGuizmo::ROTATE;
	}
	ImGui::SameLine();
	if (DrawModeButton("S", m_GizmoOperation == ImGuizmo::SCALE, "Scale"))
	{
		m_GizmoOperation = ImGuizmo::SCALE;
	}

	if (m_GizmoOperation != ImGuizmo::SCALE)
	{
		ImGui::SameLine();
		ImGui::Dummy(ImVec2(10.f, 0.f));
		ImGui::SameLine();
		if (ImGui::RadioButton("Local", m_GizmoMode == ImGuizmo::LOCAL))
		{
			m_GizmoMode = ImGuizmo::LOCAL;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("World", m_GizmoMode == ImGuizmo::WORLD))
		{
			m_GizmoMode = ImGuizmo::WORLD;
		}
	}
}

void CMapEditorGUI::RenderGizmo()
{
	auto pActiveCamera = E::CGameInstance::Get().GetActiveCamera();
	if (pActiveCamera == nullptr)
	{
		return;
	}

	auto* pSelectedObject = GetSelectedObject();
	if (pSelectedObject == nullptr)
	{
		return;
	}

	E::_float4x4 view{};
	E::_float4x4 proj{};
	XMStoreFloat4x4(&view, pActiveCamera->GetView());
	XMStoreFloat4x4(&proj, pActiveCamera->GetProj());

	auto& selectedTransform = pSelectedObject->GetTransform();
	selectedTransform.Update();
	E::_float4x4 gizmoMatrix = *selectedTransform.GetWorldMatrix();

	ImGuiViewport* pViewport = ImGui::GetMainViewport();
	const ImVec2 viewportPos = pViewport->Pos;
	const ImVec2 viewportSize = pViewport->Size;

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList(pViewport));
	ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);
	if (ImGuizmo::Manipulate(&view._11, &proj._11, m_GizmoOperation, m_GizmoMode, &gizmoMatrix._11))
	{
		ApplyMatrixToTransform(selectedTransform, gizmoMatrix);
	}
}

void CMapEditorGUI::AddCamera()
{
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
}
