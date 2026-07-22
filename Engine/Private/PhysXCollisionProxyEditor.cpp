#include "pch.h"
#include "PhysXCollisionProxyEditor.h"

#include "CameraObject.h"
#include "DbgLineRender.h"
#include "GameInstance.h"

#include <filesystem>

NS_USING(Engine)

namespace
{
	constexpr size_t MAX_UNDO_HISTORY = 64;
	constexpr E::_float MIN_BOX_SIZE = 0.01f;
	constexpr int PX_COLLISION_PROXY_GIZMO_ID = 0x50584350;

	E::_matrix MakeBoxMatrix(const PX_COLLISION_PROXY_BOX& box)
	{
		return XMMatrixScaling(box.vSize.x, box.vSize.y, box.vSize.z) *
			XMMatrixRotationQuaternion(XMLoadFloat4(&box.vRotation)) *
			XMMatrixTranslation(box.vPosition.x, box.vPosition.y, box.vPosition.z);
	}

}

void CPhysXCollisionProxyEditor::UpdateGUI(E::_float fTimeDelta)
{
	DrawWindow();
	DrawDebugBoxes();
	if (!m_bEditMode)
		return;

	RenderGizmo();
	HandleSceneInput();
}

void CPhysXCollisionProxyEditor::DrawWindow()
{
	ImGui::SetNextWindowSize(ImVec2(350.f, 430.f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Collision Proxy"))
	{
		ImGui::End();
		return;
	}

	ImGui::Checkbox("Edit Mode", &m_bEditMode);
	ImGui::SameLine();
	ImGui::Checkbox("Visible", &m_bVisible);
	ImGui::SameLine();
	ImGui::Checkbox("Depth", &m_bDepthTest);
	ImGui::SetNextItemWidth(256.f);
	ImGui::InputText("Collision File", m_CollisionFileName, std::size(m_CollisionFileName));

	if (ImGui::Button("Save Collision", ImVec2(126.f, 0.f)))
	{
		m_Status = SUCCEEDED(Save())
			? "Saved: " + MakePxCollisionFilePath(m_CollisionFileName)
			: "Save failed: " + MakePxCollisionFilePath(m_CollisionFileName);
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Collision", ImVec2(126.f, 0.f)))
		ImGui::OpenPopup("Confirm Collision Load");

	if (ImGui::BeginPopupModal("Confirm Collision Load", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted("Load collision proxies?");
		ImGui::TextDisabled("Unsaved collision proxy changes will be lost.");
		ImGui::Separator();
		if (ImGui::Button("Yes", ImVec2(100.f, 0.f)))
		{
			Load();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("No", ImVec2(100.f, 0.f)))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	ImGui::Separator();

	if (ImGui::Checkbox("Place Box", &m_bPlaceMode))
		m_SelectedID.reset();
	ImGui::SameLine();
	if (ImGui::Button("Create at Camera"))
	{
		auto* camera = E::CGameInstance::Get().GetActiveCamera();
		if (camera)
		{
			const E::_matrix inverseView = XMMatrixInverse(nullptr, camera->GetView());
			E::_float3 position{};
			XMStoreFloat3(&position, inverseView.r[3] + XMVector3Normalize(inverseView.r[2]) * 5.f);
			CreateBox(position);
		}
	}

	ImGui::DragFloat("Placement Y", &m_fPlacementY, 0.1f);
	ImGui::DragFloat3("Default Size", &m_vDefaultSize.x, 0.1f, MIN_BOX_SIZE, 1000.f, "%.2f");
	m_vDefaultSize.x = std::max(m_vDefaultSize.x, MIN_BOX_SIZE);
	m_vDefaultSize.y = std::max(m_vDefaultSize.y, MIN_BOX_SIZE);
	m_vDefaultSize.z = std::max(m_vDefaultSize.z, MIN_BOX_SIZE);

	ImGui::Separator();
	if (ImGui::Button("T", ImVec2(34.f, 0.f))) m_GizmoOperation = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::Button("R", ImVec2(34.f, 0.f))) m_GizmoOperation = ImGuizmo::ROTATE;
	ImGui::SameLine();
	if (ImGui::Button("S", ImVec2(34.f, 0.f))) m_GizmoOperation = ImGuizmo::SCALE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Local", m_GizmoMode == ImGuizmo::LOCAL)) m_GizmoMode = ImGuizmo::LOCAL;
	ImGui::SameLine();
	if (ImGui::RadioButton("World", m_GizmoMode == ImGuizmo::WORLD)) m_GizmoMode = ImGuizmo::WORLD;

	if (ImGui::Button("Undo")) Undo();
	ImGui::SameLine();
	if (ImGui::Button("Redo")) Redo();
	ImGui::SameLine();
	if (ImGui::Button("Duplicate")) DuplicateSelected();
	ImGui::SameLine();
	if (ImGui::Button("Delete")) DeleteSelected();

	ImGui::Text("Boxes: %zu", m_Boxes.size());
	if (auto* box = GetSelectedBox())
	{
		ImGui::Separator();
		ImGui::Text("Selected: %s", box->sName.c_str());
		ImGui::Checkbox("Enabled", &box->bEnabled);
		ImGui::DragFloat3("Position", &box->vPosition.x, 0.1f);
		ImGui::DragFloat3("Size", &box->vSize.x, 0.1f, MIN_BOX_SIZE, 1000.f, "%.3f");
		box->vSize.x = std::max(box->vSize.x, MIN_BOX_SIZE);
		box->vSize.y = std::max(box->vSize.y, MIN_BOX_SIZE);
		box->vSize.z = std::max(box->vSize.z, MIN_BOX_SIZE);
	}
	else
	{
		ImGui::TextDisabled(m_bPlaceMode ? "Click the scene to place a box." : "Click a box to select it.");
	}

	if (!m_Status.empty())
		ImGui::TextWrapped("%s", m_Status.c_str());
	ImGui::End();
}

void CPhysXCollisionProxyEditor::DrawDebugBoxes()
{
	if (!m_bVisible)
		return;

	auto* debug = E::CGameInstance::Get().GetDbgLineRender();
	if (!debug)
		return;

	const auto previousColor = debug->GetColor();
	const auto previousDepth = debug->GetDepthMode();
	debug->SetDepthTest(m_bDepthTest);
	for (const auto& box : m_Boxes)
	{
		if (!box.bEnabled)
			debug->SetColor({ 0.35f, 0.35f, 0.35f, 1.f });
		else if (m_SelectedID && box.iID == *m_SelectedID)
			debug->SetColor({ 1.f, 0.9f, 0.1f, 1.f });
		else
			debug->SetColor({ 0.f, 0.9f, 1.f, 1.f });

		debug->AddBox({ 0.5f, 0.5f, 0.5f }, MakeBoxMatrix(box));
	}
	debug->SetColor(previousColor);
	debug->SetDepthMode(previousDepth);
}

void CPhysXCollisionProxyEditor::RenderGizmo()
{
	auto* box = GetSelectedBox();
	auto* camera = E::CGameInstance::Get().GetActiveCamera();
	if (!box || !camera || m_bPlaceMode)
		return;

	E::_float4x4 view{};
	E::_float4x4 projection{};
	E::_float4x4 world{};
	XMStoreFloat4x4(&view, camera->GetView());
	XMStoreFloat4x4(&projection, camera->GetProj());
	XMStoreFloat4x4(&world, MakeBoxMatrix(*box));

	const E::_float2 clientSize = E::CGameInstance::Get().GetClientScreenSize();
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
	ImGuizmo::SetRect(0.f, 0.f, clientSize.x, clientSize.y);
	ImGuizmo::SetID(PX_COLLISION_PROXY_GIZMO_ID);

	const _bool wasUsing = ImGuizmo::IsUsing();
	if (!wasUsing && ImGuizmo::IsOver() && !m_GizmoStartSnapshot)
		m_GizmoStartSnapshot = SNAPSHOT{ m_Boxes, m_SelectedID };

	if (ImGuizmo::Manipulate(&view._11, &projection._11, m_GizmoOperation,
		m_GizmoOperation == ImGuizmo::SCALE ? ImGuizmo::LOCAL : m_GizmoMode, &world._11))
	{
		if (!m_GizmoStartSnapshot)
			m_GizmoStartSnapshot = SNAPSHOT{ m_Boxes, m_SelectedID };

		E::_vector scale{};
		E::_vector rotation{};
		E::_vector translation{};
		if (XMMatrixDecompose(&scale, &rotation, &translation, XMLoadFloat4x4(&world)))
		{
			XMStoreFloat3(&box->vSize, scale);
			XMStoreFloat4(&box->vRotation, XMQuaternionNormalize(rotation));
			XMStoreFloat3(&box->vPosition, translation);
			box->vSize.x = std::max(fabsf(box->vSize.x), MIN_BOX_SIZE);
			box->vSize.y = std::max(fabsf(box->vSize.y), MIN_BOX_SIZE);
			box->vSize.z = std::max(fabsf(box->vSize.z), MIN_BOX_SIZE);
		}
	}

	const _bool isUsing = ImGuizmo::IsUsing();
	if (m_bWasUsingGizmo && !isUsing && m_GizmoStartSnapshot)
	{
		m_UndoStack.push_back(std::move(*m_GizmoStartSnapshot));
		if (m_UndoStack.size() > MAX_UNDO_HISTORY) m_UndoStack.erase(m_UndoStack.begin());
		m_RedoStack.clear();
		m_GizmoStartSnapshot.reset();
	}
	m_bWasUsingGizmo = isUsing;
}

void CPhysXCollisionProxyEditor::HandleSceneInput()
{
	const ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse || ImGuizmo::IsOver() || ImGuizmo::IsUsing())
		return;

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		if (m_bPlaceMode)
		{
			E::_float3 position{};
			if (IntersectPlacementPlane(position))
				CreateBox(position);
		}
		else
		{
			SelectAtMouse();
		}
	}
}

void CPhysXCollisionProxyEditor::CreateBox(const E::_float3& position)
{
	PushUndo();
	PX_COLLISION_PROXY_BOX box{};
	box.iID = m_iNextID++;
	box.sName = "CollisionBox_" + std::to_string(box.iID);
	box.vPosition = position;
	box.vSize = m_vDefaultSize;
	m_SelectedID = box.iID;
	m_Boxes.push_back(std::move(box));
	m_bPlaceMode = false;
}

void CPhysXCollisionProxyEditor::DuplicateSelected()
{
	const auto* selected = GetSelectedBox();
	if (!selected) return;
	const PX_COLLISION_PROXY_BOX source = *selected;
	PushUndo();
	PX_COLLISION_PROXY_BOX copy = source;
	copy.iID = m_iNextID++;
	copy.sName = "CollisionBox_" + std::to_string(copy.iID);
	copy.vPosition.x += 0.25f;
	copy.vPosition.z += 0.25f;
	m_SelectedID = copy.iID;
	m_Boxes.push_back(std::move(copy));
}

void CPhysXCollisionProxyEditor::DeleteSelected()
{
	if (!m_SelectedID) return;
	PushUndo();
	std::erase_if(m_Boxes, [id = *m_SelectedID](const PX_COLLISION_PROXY_BOX& box) { return box.iID == id; });
	m_SelectedID.reset();
}

void CPhysXCollisionProxyEditor::SelectAtMouse()
{
	E::_float3 origin{};
	E::_float3 direction{};
	if (!MakeMouseRay(origin, direction)) return;

	E::_float nearest = FLT_MAX;
	std::optional<uint64_t> nearestID{};
	for (const auto& box : m_Boxes)
	{
		BoundingOrientedBox bounds{};
		bounds.Center = box.vPosition;
		bounds.Extents = { box.vSize.x * 0.5f, box.vSize.y * 0.5f, box.vSize.z * 0.5f };
		bounds.Orientation = box.vRotation;
		E::_float distance{};
		if (bounds.Intersects(XMLoadFloat3(&origin), XMLoadFloat3(&direction), distance) && distance < nearest)
		{
			nearest = distance;
			nearestID = box.iID;
		}
	}
	m_SelectedID = nearestID;
}

_bool CPhysXCollisionProxyEditor::MakeMouseRay(E::_float3& outOrigin, E::_float3& outDirection) const
{
	auto* camera = E::CGameInstance::Get().GetActiveCamera();
	if (!camera) return false;
	const E::_float2 mouse = E::CGameInstance::Get().GetMousePos();
	const E::_float2 size = E::CGameInstance::Get().GetClientScreenSize();
	if (size.x <= 0.f || size.y <= 0.f || mouse.x < 0.f || mouse.y < 0.f || mouse.x >= size.x || mouse.y >= size.y)
		return false;

	const E::_vector nearPoint = XMVector3Unproject(XMVectorSet(mouse.x, mouse.y, 0.f, 1.f),
		0.f, 0.f, size.x, size.y, 0.f, 1.f, camera->GetProj(), camera->GetView(), XMMatrixIdentity());
	const E::_vector farPoint = XMVector3Unproject(XMVectorSet(mouse.x, mouse.y, 1.f, 1.f),
		0.f, 0.f, size.x, size.y, 0.f, 1.f, camera->GetProj(), camera->GetView(), XMMatrixIdentity());
	XMStoreFloat3(&outOrigin, nearPoint);
	XMStoreFloat3(&outDirection, XMVector3Normalize(farPoint - nearPoint));
	return true;
}

_bool CPhysXCollisionProxyEditor::IntersectPlacementPlane(E::_float3& outPosition) const
{
	E::_float3 origin{};
	E::_float3 direction{};
	if (!MakeMouseRay(origin, direction) || fabsf(direction.y) < 1e-6f) return false;
	const E::_float distance = (m_fPlacementY - origin.y) / direction.y;
	if (distance < 0.f) return false;
	outPosition = { origin.x + direction.x * distance, m_fPlacementY, origin.z + direction.z * distance };
	return true;
}

PX_COLLISION_PROXY_BOX* CPhysXCollisionProxyEditor::GetSelectedBox()
{
	if (!m_SelectedID) return nullptr;
	const auto iter = std::find_if(m_Boxes.begin(), m_Boxes.end(), [id = *m_SelectedID](const auto& box) { return box.iID == id; });
	return iter == m_Boxes.end() ? nullptr : &*iter;
}

const PX_COLLISION_PROXY_BOX* CPhysXCollisionProxyEditor::GetSelectedBox() const
{
	if (!m_SelectedID) return nullptr;
	const auto iter = std::find_if(m_Boxes.begin(), m_Boxes.end(), [id = *m_SelectedID](const auto& box) { return box.iID == id; });
	return iter == m_Boxes.end() ? nullptr : &*iter;
}

void CPhysXCollisionProxyEditor::PushUndo()
{
	m_UndoStack.push_back({ m_Boxes, m_SelectedID });
	if (m_UndoStack.size() > MAX_UNDO_HISTORY) m_UndoStack.erase(m_UndoStack.begin());
	m_RedoStack.clear();
}

void CPhysXCollisionProxyEditor::Undo()
{
	if (m_UndoStack.empty()) return;
	m_RedoStack.push_back({ m_Boxes, m_SelectedID });
	auto snapshot = std::move(m_UndoStack.back());
	m_UndoStack.pop_back();
	RestoreSnapshot(std::move(snapshot));
}

void CPhysXCollisionProxyEditor::Redo()
{
	if (m_RedoStack.empty()) return;
	m_UndoStack.push_back({ m_Boxes, m_SelectedID });
	auto snapshot = std::move(m_RedoStack.back());
	m_RedoStack.pop_back();
	RestoreSnapshot(std::move(snapshot));
}

void CPhysXCollisionProxyEditor::RestoreSnapshot(SNAPSHOT snapshot)
{
	m_Boxes = std::move(snapshot.boxes);
	m_SelectedID = snapshot.selectedID;
	m_iNextID = 1;
	for (const auto& box : m_Boxes) m_iNextID = std::max(m_iNextID, box.iID + 1);
}

HRESULT CPhysXCollisionProxyEditor::Save() const
{
	const std::filesystem::path filePath = MakePxCollisionFilePath(m_CollisionFileName);
	std::error_code error{};
	std::filesystem::create_directories(filePath.parent_path(), error);
	if (error) return E_FAIL;

	PX_COLLISION_PROXY_FILE data{};
	data.boxes = m_Boxes;
	return E::CGameInstance::Get().JsonSerialize(filePath.generic_string(), data, "CollisionProxies");
}

HRESULT CPhysXCollisionProxyEditor::Load()
{
	const std::filesystem::path filePath = MakePxCollisionFilePath(m_CollisionFileName);
	if (!std::filesystem::exists(filePath))
	{
		m_Status = "File not found: " + filePath.generic_string();
		return E_FAIL;
	}

	PX_COLLISION_PROXY_FILE data{};
	if (FAILED(E::CGameInstance::Get().JsonDeSerialize(
		filePath.generic_string(), data, "CollisionProxies")))
	{
		m_Status = "Load failed: " + filePath.generic_string();
		return E_FAIL;
	}

	if (data.iVersion != 1)
	{
		m_Status = "Unsupported collision proxy version";
		return E_FAIL;
	}

	for (auto& box : data.boxes)
	{
		box.vSize.x = std::max(fabsf(box.vSize.x), MIN_BOX_SIZE);
		box.vSize.y = std::max(fabsf(box.vSize.y), MIN_BOX_SIZE);
		box.vSize.z = std::max(fabsf(box.vSize.z), MIN_BOX_SIZE);
		box.vRotation = [&box]()
		{
			E::_float4 normalized{};
			XMStoreFloat4(&normalized, XMQuaternionNormalize(XMLoadFloat4(&box.vRotation)));
			return normalized;
		}();
	}

	m_Boxes = std::move(data.boxes);
	m_SelectedID.reset();
	m_UndoStack.clear();
	m_RedoStack.clear();
	m_iNextID = 1;
	for (const auto& box : m_Boxes) m_iNextID = std::max(m_iNextID, box.iID + 1);
	m_Status = "Loaded: " + filePath.generic_string();
	return S_OK;
}

void CPhysXCollisionProxyEditor::Clear()
{
	m_Boxes.clear();
	m_SelectedID.reset();
	m_UndoStack.clear();
	m_RedoStack.clear();
	m_iNextID = 1;
}

E::UPtr<CPhysXCollisionProxyEditor> CPhysXCollisionProxyEditor::Create()
{
	return E::ToUPtr(new CPhysXCollisionProxyEditor{});
}
