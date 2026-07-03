#include "pch.h"
#include "LevelLogo.h"
#include "GameInstance.h"
#include "LevelMapEditor.h"
#include "FlyCamera.h"
#include "ResCBuffer.h"
#include "UiCamera.h"


#include "TestGuizmo.h"

NS_USING(Client)

namespace
{
	ImGuizmo::OPERATION g_GizmoOperation = ImGuizmo::TRANSLATE;
	ImGuizmo::MODE g_GizmoMode = ImGuizmo::WORLD;

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

	void DrawGizmoToolbar()
	{
		ImGui::TextDisabled("Gizmo");
		ImGui::SameLine();
		if (DrawModeButton("T", g_GizmoOperation == ImGuizmo::TRANSLATE, "Translate"))
		{
			g_GizmoOperation = ImGuizmo::TRANSLATE;
		}
		ImGui::SameLine();
		if (DrawModeButton("R", g_GizmoOperation == ImGuizmo::ROTATE, "Rotate"))
		{
			g_GizmoOperation = ImGuizmo::ROTATE;
		}
		ImGui::SameLine();
		if (DrawModeButton("S", g_GizmoOperation == ImGuizmo::SCALE, "Scale"))
		{
			g_GizmoOperation = ImGuizmo::SCALE;
		}

		if (g_GizmoOperation != ImGuizmo::SCALE)
		{
			ImGui::SameLine();
			ImGui::Dummy(ImVec2(10.f, 0.f));
			ImGui::SameLine();
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

	bool DrawVec3Control(const char* label, E::_float3& value, const E::_float3& resetValue, float speed)
	{
		bool changed = false;

		ImGui::PushID(label);
		ImGui::TextUnformatted(label);
		ImGui::SameLine(82.f);
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 34.f);
		changed = ImGui::DragFloat3("##Value", reinterpret_cast<float*>(&value), speed, 0.f, 0.f, "%.3f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		if (ImGui::Button("R", ImVec2(26.f, 0.f)))
		{
			value = resetValue;
			changed = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Reset %s", label);
		}
		ImGui::PopID();

		return changed;
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

	void DrawSelectedTransform(E::CComTransform& transform)
	{
		E::_float3 position = transform.GetPosition();
		E::_float3 rotation = transform.GetRotationEuler();
		E::_float3 scale = transform.GetScale();

		if (DrawVec3Control("Position", position, E::_float3{ 0.f, 0.f, 0.f }, 0.1f))
		{
			transform.SetPosition(position);
		}
		if (DrawVec3Control("Rotation", rotation, E::_float3{ 0.f, 0.f, 0.f }, 0.5f))
		{
			transform.SetRotationEuler(rotation);
		}
		if (DrawVec3Control("Scale", scale, E::_float3{ 1.f, 1.f, 1.f }, 0.1f))
		{
			transform.SetScale(scale);
		}

		transform.Update();
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
	ImGuizmo::BeginFrame();

	ImGui::SetNextWindowSize(ImVec2(360.f, 520.f), ImGuiCond_FirstUseEver);
	ImGui::Begin("MapEditor");
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, 7.f));

	if (ImGui::Button("Level Save", ImVec2(112.f, 0.f)))
	{
		ImGui::OpenPopup("SaveCheck");
	}
	ImGui::SameLine();
	if (ImGui::Button("Level Load", ImVec2(112.f, 0.f)))
	{
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
	ImGui::TextDisabled("Hierarchy");
	ImGui::BeginChild("##ObjectList", ImVec2(0.f, 156.f), true);
	{
		const auto& layers = E::CGameInstance::Get().GetGameObjectLayers();
		for (const auto& [layerName, handles] : layers)
		{
			ImGui::PushID(layerName.c_str());
			const bool bOpen = ImGui::TreeNodeEx(
				layerName.c_str(),
				ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth,
				"%s (%zu)",
				layerName.c_str(),
				handles.size());

			if (bOpen)
			{
				for (const auto& handle : handles)
				{
					auto* pObject = E::CGameInstance::Get().GetGameObjectByHandle(handle);
					if (pObject == nullptr)
					{
						continue;
					}

					const bool bSelected = (handle == m_SelectedObject);
					ImGui::PushID(static_cast<int>(handle.GetIndex()));
					if (ImGui::Selectable(pObject->GetObjectTag().data(), bSelected))
					{
						m_SelectedObject = handle;
					}
					ImGui::PopID();
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}
	ImGui::EndChild();

	ImGui::Separator();
	ImGui::TextDisabled("Inspector");

	auto* pSelectedObject = E::CGameInstance::Get().GetGameObjectByHandle(m_SelectedObject);
	if (pSelectedObject == nullptr)
	{
		ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, 0.55f), "Select an object to view details.");
		ImGui::PopStyleVar(2);
		ImGui::End();
		return;
	}

	ImGui::BeginChild("##Inspector", ImVec2(0.f, 0.f), true);
	ImGui::TextUnformatted("Name");
	ImGui::SameLine(82.f);
	ImGui::TextUnformatted(pSelectedObject->GetObjectTag().data());

	ImGui::TextUnformatted("Handle");
	ImGui::SameLine(82.f);
	ImGui::Text("%zu : %u", m_SelectedObject.GetIndex(), m_SelectedObject.GetGeneration());

	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		DrawSelectedTransform(pSelectedObject->GetTransform());
	}
	ImGui::EndChild();

	ImGui::PopStyleVar(2);
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

	auto& selectedTransform = pSelectedObject->GetTransform();
	selectedTransform.Update();
	E::_float4x4 gizmoMatrix = *selectedTransform.GetWorldMatrix();
	
	ImGuiViewport* pViewport = ImGui::GetMainViewport();
	const ImVec2 viewportPos = pViewport->Pos;
	const ImVec2 viewportSize = pViewport->Size;
	
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList(pViewport));
	ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);
	if (ImGuizmo::Manipulate(&view._11, &proj._11, g_GizmoOperation, g_GizmoMode, &gizmoMatrix._11))
	{
		ApplyMatrixToTransform(selectedTransform, gizmoMatrix);
	}
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
