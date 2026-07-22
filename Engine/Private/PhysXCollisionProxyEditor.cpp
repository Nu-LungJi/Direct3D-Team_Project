#include "pch.h"
#include "PhysXCollisionProxyEditor.h"

#include "CameraObject.h"
#include "DbgLineRender.h"
#include "Engine_PhysxDefines.h"
#include "GameInstance.h"
#include "PhysXCollisionProxyObject.h"
#include "ResPhysXConvexGeometry.h"
#include "ResPhysXTriMeshGeometry.h"

#include <filesystem>

#pragma push_macro("new")
#undef new
#include "PxPhysicsAPI.h"
#pragma pop_macro("new")

NS_USING(Engine)

namespace
{
	constexpr size_t MAX_UNDO_HISTORY = 64;
	constexpr E::_float MIN_SHAPE_SIZE = 0.01f;
	constexpr int PX_COLLISION_PROXY_GIZMO_ID = 0x50584350;

	const char* ACTOR_TYPE_NAMES[] = { "Static", "Dynamic", "Kinematic" };
	const char* SHAPE_TYPE_NAMES[] = { "Box", "Sphere", "Capsule", "Convex Mesh", "Triangle Mesh" };

	_bool IsUnitCylinderConvexPath(const std::string& path)
	{
		if (path.empty())
			return false;

		return std::filesystem::path{ path }.lexically_normal().generic_string() ==
			std::filesystem::path{ E::PX_UNIT_CYLINDER_CONVEX_PATH }.lexically_normal().generic_string();
	}

	_bool IsUnitWedgeConvexPath(const std::string& path)
	{
		if (path.empty())
			return false;

		return std::filesystem::path{ path }.lexically_normal().generic_string() ==
			std::filesystem::path{ E::PX_UNIT_WEDGE_CONVEX_PATH }.lexically_normal().generic_string();
	}

	E::_matrix MakeActorMatrix(const E::PX_COLLISION_PROXY_ACTOR& actor)
	{
		return XMMatrixRotationQuaternion(XMLoadFloat4(&actor.vRotation)) *
			XMMatrixTranslation(actor.vPosition.x, actor.vPosition.y, actor.vPosition.z);
	}

	E::_matrix MakeShapePoseMatrix(const E::PX_COLLISION_PROXY_SHAPE& shape)
	{
		return XMMatrixRotationQuaternion(XMLoadFloat4(&shape.vLocalRotation)) *
			XMMatrixTranslation(shape.vLocalPosition.x, shape.vLocalPosition.y, shape.vLocalPosition.z);
	}

	E::_float3 GetShapeDimensions(const E::PX_COLLISION_PROXY_SHAPE& shape)
	{
		switch (shape.eType)
		{
		case E::PX_COLLISION_PROXY_SHAPE_TYPE::BOX:
			return shape.vSize;
		case E::PX_COLLISION_PROXY_SHAPE_TYPE::SPHERE:
			return { shape.fRadius * 2.f, shape.fRadius * 2.f, shape.fRadius * 2.f };
		case E::PX_COLLISION_PROXY_SHAPE_TYPE::CAPSULE:
			return { (shape.fHalfHeight + shape.fRadius) * 2.f,
				shape.fRadius * 2.f, shape.fRadius * 2.f };
		case E::PX_COLLISION_PROXY_SHAPE_TYPE::CONVEX_MESH:
		case E::PX_COLLISION_PROXY_SHAPE_TYPE::TRIANGLE_MESH:
			return shape.vScale;
		default:
			return { 1.f, 1.f, 1.f };
		}
	}

	E::_matrix MakeShapeWorldMatrix(
		const E::PX_COLLISION_PROXY_ACTOR& actor,
		const E::PX_COLLISION_PROXY_SHAPE& shape)
	{
		const E::_float3 dimensions = GetShapeDimensions(shape);
		return XMMatrixScaling(dimensions.x, dimensions.y, dimensions.z) *
			MakeShapePoseMatrix(shape) * MakeActorMatrix(actor);
	}

	E::_matrix MakeShapePoseWorldMatrix(
		const E::PX_COLLISION_PROXY_ACTOR& actor,
		const E::PX_COLLISION_PROXY_SHAPE& shape)
	{
		return MakeShapePoseMatrix(shape) * MakeActorMatrix(actor);
	}

	void NormalizeQuaternion(E::_float4& rotation)
	{
		const E::_vector quaternion = XMLoadFloat4(&rotation);
		if (XMVectorGetX(XMVector4LengthSq(quaternion)) <= 1e-8f)
			rotation = { 0.f, 0.f, 0.f, 1.f };
		else
			XMStoreFloat4(&rotation, XMQuaternionNormalize(quaternion));
	}

	E::_float3 QuaternionToEulerDegrees(const E::_float4& quaternion)
	{
		E::_float4 q = quaternion;
		NormalizeQuaternion(q);

		const float sinX = 2.f * (q.w * q.x + q.y * q.z);
		const float cosX = 1.f - 2.f * (q.x * q.x + q.y * q.y);
		const float sinY = std::clamp(2.f * (q.w * q.y - q.z * q.x), -1.f, 1.f);
		const float sinZ = 2.f * (q.w * q.z + q.x * q.y);
		const float cosZ = 1.f - 2.f * (q.y * q.y + q.z * q.z);

		return {
			XMConvertToDegrees(std::atan2(sinX, cosX)),
			XMConvertToDegrees(std::asin(sinY)),
			XMConvertToDegrees(std::atan2(sinZ, cosZ))
		};
	}

	E::_float4 EulerDegreesToQuaternion(const E::_float3& euler)
	{
		E::_float4 quaternion{};
		XMStoreFloat4(&quaternion, XMQuaternionRotationRollPitchYaw(
			XMConvertToRadians(euler.x),
			XMConvertToRadians(euler.y),
			XMConvertToRadians(euler.z)));
		NormalizeQuaternion(quaternion);
		return quaternion;
	}

	_bool EditString(const char* label, std::string& value, size_t capacity = 512)
	{
		std::vector<char> buffer(capacity, '\0');
		strncpy_s(buffer.data(), buffer.size(), value.c_str(), _TRUNCATE);
		if (!ImGui::InputText(label, buffer.data(), buffer.size()))
			return false;
		value = buffer.data();
		return true;
	}

	void ApplyGizmoScale(E::PX_COLLISION_PROXY_SHAPE& shape, const E::_float3& scale)
	{
		const E::_float3 absolute{
			std::max(std::abs(scale.x), MIN_SHAPE_SIZE),
			std::max(std::abs(scale.y), MIN_SHAPE_SIZE),
			std::max(std::abs(scale.z), MIN_SHAPE_SIZE)
		};

		switch (shape.eType)
		{
		case E::PX_COLLISION_PROXY_SHAPE_TYPE::BOX:
			shape.vSize = absolute;
			break;
		case E::PX_COLLISION_PROXY_SHAPE_TYPE::SPHERE:
			shape.fRadius = std::max({ absolute.x, absolute.y, absolute.z }) * 0.5f;
			break;
		case E::PX_COLLISION_PROXY_SHAPE_TYPE::CAPSULE:
			shape.fRadius = std::max((absolute.y + absolute.z) * 0.25f, MIN_SHAPE_SIZE);
			shape.fHalfHeight = std::max(absolute.x * 0.5f - shape.fRadius, MIN_SHAPE_SIZE);
			break;
		case E::PX_COLLISION_PROXY_SHAPE_TYPE::CONVEX_MESH:
		case E::PX_COLLISION_PROXY_SHAPE_TYPE::TRIANGLE_MESH:
			shape.vScale = absolute;
			break;
		}
	}
}

void CPhysXCollisionProxyEditor::UpdateGUI(_float fTimeDelta)
{
	DrawWindow();
	DrawDebugShapes();
	if (!m_bEditMode)
		return;

	RenderGizmo();
	HandleSceneInput();
}

void CPhysXCollisionProxyEditor::SetCollisionLayerNames(
	std::vector<std::pair<uint32_t, std::string>> layerNames)
{
	layerNames.erase(std::remove_if(layerNames.begin(), layerNames.end(),
		[](const auto& entry)
		{
			const uint32_t value = entry.first;
			return entry.second.empty() || (value != 0 && (value & (value - 1)) != 0);
		}), layerNames.end());

	std::sort(layerNames.begin(), layerNames.end(),
		[](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
	layerNames.erase(std::unique(layerNames.begin(), layerNames.end(),
		[](const auto& lhs, const auto& rhs) { return lhs.first == rhs.first; }), layerNames.end());
	m_CollisionLayerNames = std::move(layerNames);
}

void CPhysXCollisionProxyEditor::DrawWindow()
{
	ImGui::SetNextWindowSize(ImVec2(620.f, 620.f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Collision Proxy"))
	{
		ImGui::End();
		return;
	}
	if (m_bOpenResultPopup)
	{
		ImGui::OpenPopup("Collision IO Result");
		m_bOpenResultPopup = false;
	}

	ImGui::Checkbox("Edit Mode", &m_bEditMode);
	ImGui::SameLine();
	ImGui::Checkbox("Visible", &m_bVisible);
	ImGui::SameLine();
	ImGui::Checkbox("Depth", &m_bDepthTest);
	ImGui::Checkbox("Edit File Name", &m_bEditCollisionFileName);
	ImGui::SameLine();
	if (m_bEditCollisionFileName)
	{
		ImGui::SetNextItemWidth(220.f);
		ImGui::InputText("Collision File", m_CollisionFileName, std::size(m_CollisionFileName));
	}
	else
	{
		ImGui::Text("Collision File: %s", m_CollisionFileName);
	}

	if (ImGui::Button("Save", ImVec2(90.f, 0.f)))
		ImGui::OpenPopup("Confirm Collision Save");
	ImGui::SameLine();
	if (ImGui::Button("Load", ImVec2(90.f, 0.f)))
		ImGui::OpenPopup("Confirm Collision Load");
	ImGui::SameLine();
	if (ImGui::Button("Clear", ImVec2(90.f, 0.f)))
		ImGui::OpenPopup("Confirm Collision Clear");

	if (ImGui::BeginPopupModal("Confirm Collision Save", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Save collision scene to:");
		ImGui::TextWrapped("%s", MakePxCollisionFilePath(m_CollisionFileName).c_str());
		ImGui::TextDisabled("An existing file with the same name will be overwritten.");
		if (ImGui::Button("Save", ImVec2(100.f, 0.f)))
		{
			const std::string path = MakePxCollisionFilePath(m_CollisionFileName);
			const _bool success = SUCCEEDED(Save());
			m_Status = success ? "Saved: " + path : "Save failed: " + path;
			QueueResultPopup(m_Status, success);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(100.f, 0.f))) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Confirm Collision Load", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted("Load collision scene? Unsaved changes will be lost.");
		if (ImGui::Button("Yes", ImVec2(100.f, 0.f)))
		{
			const std::string path = MakePxCollisionFilePath(m_CollisionFileName);
			const _bool success = SUCCEEDED(Load());
			if (success) m_Status = "Loaded: " + path;
			QueueResultPopup(m_Status, success);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("No", ImVec2(100.f, 0.f))) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Confirm Collision Clear", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted("Clear every collision actor and shape?");
		if (ImGui::Button("Yes", ImVec2(100.f, 0.f)))
		{
			Clear();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("No", ImVec2(100.f, 0.f))) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Collision IO Result", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		const ImVec4 color = m_bResultPopupSuccess
			? ImVec4(0.25f, 1.f, 0.35f, 1.f)
			: ImVec4(1.f, 0.3f, 0.2f, 1.f);
		ImGui::TextColored(color, "%s", m_bResultPopupSuccess ? "Success" : "Failed");
		ImGui::Separator();
		ImGui::TextWrapped("%s", m_ResultPopupMessage.c_str());
		if (ImGui::Button("OK", ImVec2(120.f, 0.f))) ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	ImGui::Separator();
	if (ImGui::Button("Build PhysX Preview", ImVec2(160.f, 0.f)))
	{
		CreatePhysicsPreview();
		QueueResultPopup(m_Status, !m_PhysicsPreviewHandles.empty());
	}
	ImGui::SameLine();
	if (ImGui::Button("Remove PhysX Preview", ImVec2(170.f, 0.f)))
		RemovePhysicsPreview();
	ImGui::SameLine();
	ImGui::TextDisabled(!m_PhysicsPreviewHandles.empty() ? "Active (rebuild after edits)" : "Inactive");

	ImGui::Separator();
	if (ImGui::Button("Add Actor")) CreateActor();
	ImGui::SameLine();
	int createType = static_cast<int>(m_eCreateShapeType);
	ImGui::SetNextItemWidth(130.f);
	if (ImGui::Combo("##CreateShapeType", &createType, SHAPE_TYPE_NAMES,
		static_cast<int>(std::size(SHAPE_TYPE_NAMES))))
		m_eCreateShapeType = static_cast<PX_COLLISION_PROXY_SHAPE_TYPE>(createType);
	ImGui::SameLine();
	if (ImGui::Button("Create Shape at Camera"))
		CreateShapeAtCamera(m_eCreateShapeType);
	ImGui::SameLine();
	if (ImGui::Button("Create Cylinder at Camera"))
		CreateCylinderAtCamera();
	ImGui::SameLine();
	if (ImGui::Button("Create Wedge at Camera"))
		CreateWedgeAtCamera();
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

	ImGui::Separator();
	if (ImGui::BeginTable("CollisionEditorLayout", 2,
		ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
	{
		ImGui::TableSetupColumn("Hierarchy", ImGuiTableColumnFlags_WidthFixed, 220.f);
		ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableNextColumn();
		DrawHierarchy();
		ImGui::TableNextColumn();
		DrawInspector();
		ImGui::EndTable();
	}

	if (!m_Status.empty())
	{
		ImGui::Separator();
		ImGui::TextWrapped("%s", m_Status.c_str());
	}
	ImGui::End();
}

void CPhysXCollisionProxyEditor::DrawHierarchy()
{
	ImGui::BeginChild("CollisionHierarchy", ImVec2(0.f, 390.f), false);
	for (auto& actor : m_Actors)
	{
		ImGui::PushID(static_cast<int>(actor.iID));
		const bool actorSelected = m_SelectedActorID == actor.iID && !m_SelectedShapeID;
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
		if (actorSelected) flags |= ImGuiTreeNodeFlags_Selected;
		const bool open = ImGui::TreeNodeEx("Actor", flags, "%s", actor.sName.c_str());
		if (ImGui::IsItemClicked())
		{
			m_SelectedActorID = actor.iID;
			m_SelectedShapeID.reset();
		}

		if (open)
		{
			for (auto& shape : actor.shapes)
			{
				ImGui::PushID(static_cast<int>(shape.iID));
				ImGuiTreeNodeFlags shapeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
				if (m_SelectedActorID == actor.iID && m_SelectedShapeID == shape.iID)
					shapeFlags |= ImGuiTreeNodeFlags_Selected;
				ImGui::TreeNodeEx("Shape", shapeFlags, "%s [%s]", shape.sName.c_str(),
					SHAPE_TYPE_NAMES[static_cast<size_t>(shape.eType)]);
				if (ImGui::IsItemClicked())
				{
					m_SelectedActorID = actor.iID;
					m_SelectedShapeID = shape.iID;
				}
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
	}
	ImGui::EndChild();
}

void CPhysXCollisionProxyEditor::DrawInspector()
{
	ImGui::BeginChild("CollisionInspector", ImVec2(0.f, 390.f), false);
	auto* actor = GetSelectedActor();
	if (!actor)
	{
		ImGui::TextDisabled("Select or create an actor.");
		ImGui::EndChild();
		return;
	}

	auto* shape = GetSelectedShape();
	if (!shape)
	{
		ImGui::TextUnformatted("Actor");
		EditString("Name", actor->sName);
		EditString("Prototype Tag", actor->sPrototypeTag);
		ImGui::TextDisabled("Empty: engine default / Set: COLLISION_PROXY group prototype");
		ImGui::Checkbox("Enabled", &actor->bEnabled);
		int actorType = static_cast<int>(actor->eType);
		if (ImGui::Combo("Type", &actorType, ACTOR_TYPE_NAMES,
			static_cast<int>(std::size(ACTOR_TYPE_NAMES))))
			actor->eType = static_cast<PX_COLLISION_PROXY_ACTOR_TYPE>(actorType);
		ImGui::DragFloat3("Position", &actor->vPosition.x, 0.1f);
		_float3 rotation = QuaternionToEulerDegrees(actor->vRotation);
		if (ImGui::DragFloat3("Rotation (deg)", &rotation.x, 0.5f))
			actor->vRotation = EulerDegreesToQuaternion(rotation);
		if (actor->eType != PX_COLLISION_PROXY_ACTOR_TYPE::STATIC)
		{
			ImGui::DragFloat("Mass", &actor->fMass, 0.1f, 0.001f, 100000.f);
			actor->fMass = std::max(actor->fMass, 0.001f);
			ImGui::Checkbox("Gravity", &actor->bGravity);
		}
		ImGui::Text("Shapes: %zu", actor->shapes.size());
		if (m_GizmoOperation == ImGuizmo::SCALE)
			ImGui::TextDisabled("Actor has no scale. Select a shape to scale it.");
	}
	else
	{
		ImGui::Text("Shape / Actor: %s", actor->sName.c_str());
		EditString("Name", shape->sName);
		ImGui::Checkbox("Enabled", &shape->bEnabled);
		int shapeType = static_cast<int>(shape->eType);
		if (ImGui::Combo("Geometry", &shapeType, SHAPE_TYPE_NAMES,
			static_cast<int>(std::size(SHAPE_TYPE_NAMES))))
		{
			shape->eType = static_cast<PX_COLLISION_PROXY_SHAPE_TYPE>(shapeType);
			if (shape->eType == PX_COLLISION_PROXY_SHAPE_TYPE::TRIANGLE_MESH)
				shape->bTrigger = false;
		}

		ImGui::DragFloat3("Local Position", &shape->vLocalPosition.x, 0.1f);
		_float3 rotation = QuaternionToEulerDegrees(shape->vLocalRotation);
		if (ImGui::DragFloat3("Local Rotation (deg)", &rotation.x, 0.5f))
			shape->vLocalRotation = EulerDegreesToQuaternion(rotation);

		switch (shape->eType)
		{
		case PX_COLLISION_PROXY_SHAPE_TYPE::BOX:
			ImGui::DragFloat3("Size", &shape->vSize.x, 0.05f, MIN_SHAPE_SIZE, 10000.f);
			shape->vSize = { std::max(std::abs(shape->vSize.x), MIN_SHAPE_SIZE),
				std::max(std::abs(shape->vSize.y), MIN_SHAPE_SIZE),
				std::max(std::abs(shape->vSize.z), MIN_SHAPE_SIZE) };
			break;
		case PX_COLLISION_PROXY_SHAPE_TYPE::SPHERE:
			ImGui::DragFloat("Radius", &shape->fRadius, 0.05f, MIN_SHAPE_SIZE, 10000.f);
			shape->fRadius = std::max(shape->fRadius, MIN_SHAPE_SIZE);
			break;
		case PX_COLLISION_PROXY_SHAPE_TYPE::CAPSULE:
			ImGui::DragFloat("Radius", &shape->fRadius, 0.05f, MIN_SHAPE_SIZE, 10000.f);
			ImGui::DragFloat("Half Height", &shape->fHalfHeight, 0.05f, MIN_SHAPE_SIZE, 10000.f);
			shape->fRadius = std::max(shape->fRadius, MIN_SHAPE_SIZE);
			shape->fHalfHeight = std::max(shape->fHalfHeight, MIN_SHAPE_SIZE);
			break;
		case PX_COLLISION_PROXY_SHAPE_TYPE::CONVEX_MESH:
		case PX_COLLISION_PROXY_SHAPE_TYPE::TRIANGLE_MESH:
			if (EditString("Cooked Path", shape->sCookedResourcePath, 1024))
				m_CookedMeshDebugCache.clear();
			ImGui::DragFloat3("Mesh Scale", &shape->vScale.x, 0.05f, MIN_SHAPE_SIZE, 10000.f);
			shape->vScale = { std::max(std::abs(shape->vScale.x), MIN_SHAPE_SIZE),
				std::max(std::abs(shape->vScale.y), MIN_SHAPE_SIZE),
				std::max(std::abs(shape->vScale.z), MIN_SHAPE_SIZE) };
			ImGui::TextDisabled("Cooked mesh preview uses the actual PhysX mesh when the file is valid.");
			if (ImGui::Button("Refresh Cooked Preview"))
				m_CookedMeshDebugCache.clear();
			break;
		}

		if (shape->eType == PX_COLLISION_PROXY_SHAPE_TYPE::TRIANGLE_MESH)
		{
			shape->bTrigger = false;
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			ImGui::Checkbox("Trigger", &shape->bTrigger);
			ImGui::PopStyleVar();
			ImGui::PopItemFlag();
			ImGui::TextDisabled("Triangle mesh triggers are not supported by PhysX.");
		}
		else
		{
			ImGui::Checkbox("Trigger", &shape->bTrigger);
		}
		ImGui::Checkbox("Simulation", &shape->bSimulationEnabled);
		ImGui::Checkbox("Scene Query", &shape->bQueryEnabled);
		DrawLayerSelector("Layer", shape->iLayer);
		DrawLayerMaskSelector("Simulation Mask", shape->iSimulationMask);
		DrawLayerMaskSelector("Query Mask", shape->iQueryMask);
		if (ImGui::TreeNode("Advanced Filter Values"))
		{
			ImGui::InputScalar("Layer (Hex)", ImGuiDataType_U32, &shape->iLayer, nullptr, nullptr,
				"%08X", ImGuiInputTextFlags_CharsHexadecimal);
			ImGui::InputScalar("Simulation Mask (Hex)", ImGuiDataType_U32, &shape->iSimulationMask,
				nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
			ImGui::InputScalar("Query Mask (Hex)", ImGuiDataType_U32, &shape->iQueryMask,
				nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
			ImGui::TreePop();
		}

		if (shape->eType == PX_COLLISION_PROXY_SHAPE_TYPE::TRIANGLE_MESH &&
			actor->eType == PX_COLLISION_PROXY_ACTOR_TYPE::DYNAMIC)
		{
			ImGui::TextColored(ImVec4(1.f, 0.4f, 0.2f, 1.f),
				"Triangle Mesh cannot use a Dynamic actor.");
		}
	}
	ImGui::EndChild();
}

void CPhysXCollisionProxyEditor::DrawLayerSelector(const char* label, uint32_t& layer) const
{
	std::string preview = "Unregistered (0x";
	char hexValue[9]{};
	sprintf_s(hexValue, "%08X", layer);
	preview += hexValue;
	preview += ")";
	for (const auto& [value, name] : m_CollisionLayerNames)
	{
		if (value == layer)
		{
			preview = name;
			break;
		}
	}

	if (!ImGui::BeginCombo(label, preview.c_str()))
		return;
	for (const auto& [value, name] : m_CollisionLayerNames)
	{
		const bool selected = layer == value;
		if (ImGui::Selectable(name.c_str(), selected))
			layer = value;
		if (selected)
			ImGui::SetItemDefaultFocus();
	}
	ImGui::EndCombo();
}

void CPhysXCollisionProxyEditor::DrawLayerMaskSelector(const char* label, uint32_t& mask) const
{
	uint32_t selectedCount{};
	for (const auto& [value, name] : m_CollisionLayerNames)
	{
		if (value != 0 && (mask & value) != 0)
			++selectedCount;
	}
	const std::string preview = mask == 0 ? "None" :
		(mask == PX_ALL_LAYERS ? "All" : std::to_string(selectedCount) + " selected");

	if (!ImGui::BeginCombo(label, preview.c_str()))
		return;
	if (ImGui::Button("All")) mask = PX_ALL_LAYERS;
	ImGui::SameLine();
	if (ImGui::Button("None")) mask = 0;
	ImGui::SameLine();
	if (ImGui::Button("Invert")) mask = ~mask;
	ImGui::Separator();
	for (const auto& [value, name] : m_CollisionLayerNames)
	{
		if (value == 0)
			continue;
		bool selected = (mask & value) != 0;
		if (ImGui::Checkbox(name.c_str(), &selected))
		{
			if (selected) mask |= value;
			else mask &= ~value;
		}
	}
	ImGui::EndCombo();
}

void CPhysXCollisionProxyEditor::DrawDebugShapes()
{
	if (!m_bVisible)
		return;

	auto* debug = CGameInstance::Get().GetDbgLineRender();
	if (!debug)
		return;

	const auto previousColor = debug->GetColor();
	const auto previousDepth = debug->GetDepthMode();
	debug->SetDepthTest(m_bDepthTest);

	for (const auto& actor : m_Actors)
	{
		for (const auto& shape : actor.shapes)
		{
			const bool selected = m_SelectedActorID == actor.iID &&
				(!m_SelectedShapeID || m_SelectedShapeID == shape.iID);
			if (!actor.bEnabled || !shape.bEnabled)
				debug->SetColor({ 0.35f, 0.35f, 0.35f, 1.f });
			else if (selected)
				debug->SetColor({ 1.f, 0.9f, 0.1f, 1.f });
			else if (shape.bTrigger)
				debug->SetColor({ 1.f, 0.25f, 0.8f, 1.f });
			else
				debug->SetColor({ 0.f, 0.9f, 1.f, 1.f });

			const _matrix poseWorld = MakeShapePoseWorldMatrix(actor, shape);
			switch (shape.eType)
			{
			case PX_COLLISION_PROXY_SHAPE_TYPE::BOX:
				debug->AddBox({ shape.vSize.x * 0.5f, shape.vSize.y * 0.5f, shape.vSize.z * 0.5f }, poseWorld);
				break;
			case PX_COLLISION_PROXY_SHAPE_TYPE::SPHERE:
				debug->AddSphere(shape.fRadius, poseWorld);
				break;
			case PX_COLLISION_PROXY_SHAPE_TYPE::CAPSULE:
				debug->AddCapsule(shape.fRadius, shape.fHalfHeight,
					XMMatrixRotationZ(XM_PIDIV2) * poseWorld);
				break;
			case PX_COLLISION_PROXY_SHAPE_TYPE::CONVEX_MESH:
				if (IsUnitCylinderConvexPath(shape.sCookedResourcePath))
				{
					debug->AddCylinder(
						PX_UNIT_CYLINDER_RADIUS,
						PX_UNIT_CYLINDER_HALF_HEIGHT,
						XMMatrixScaling(shape.vScale.x, shape.vScale.y, shape.vScale.z) * poseWorld);
					break;
				}
				if (IsUnitWedgeConvexPath(shape.sCookedResourcePath))
				{
					debug->AddWedge(
						XMMatrixScaling(shape.vScale.x, shape.vScale.y, shape.vScale.z) * poseWorld);
					break;
				}
				if (const auto* mesh = GetOrBuildCookedMeshDebugData(
					shape.eType, shape.sCookedResourcePath))
				{
					debug->AddConvexHull(
						mesh->vertices.data(), static_cast<uint32_t>(mesh->vertices.size()),
						mesh->indices.data(), static_cast<uint32_t>(mesh->indices.size() / 3),
						XMMatrixScaling(shape.vScale.x, shape.vScale.y, shape.vScale.z) * poseWorld);
					break;
				}
				debug->AddBox({ shape.vScale.x * 0.5f, shape.vScale.y * 0.5f, shape.vScale.z * 0.5f }, poseWorld);
				break;
			case PX_COLLISION_PROXY_SHAPE_TYPE::TRIANGLE_MESH:
				if (const auto* mesh = GetOrBuildCookedMeshDebugData(
					shape.eType, shape.sCookedResourcePath))
				{
					debug->AddTriangleMesh(
						mesh->vertices.data(), static_cast<uint32_t>(mesh->vertices.size()),
						mesh->indices.data(), static_cast<uint32_t>(mesh->indices.size() / 3),
						XMMatrixScaling(shape.vScale.x, shape.vScale.y, shape.vScale.z) * poseWorld);
					break;
				}
				debug->AddBox({ shape.vScale.x * 0.5f, shape.vScale.y * 0.5f, shape.vScale.z * 0.5f }, poseWorld);
				break;
			}
		}
	}

	debug->SetColor(previousColor);
	debug->SetDepthMode(previousDepth);
}

const CPhysXCollisionProxyEditor::COOKED_MESH_DEBUG_DATA*
CPhysXCollisionProxyEditor::GetOrBuildCookedMeshDebugData(
	PX_COLLISION_PROXY_SHAPE_TYPE eType, const std::string& path)
{
	if (path.empty() || (eType != PX_COLLISION_PROXY_SHAPE_TYPE::CONVEX_MESH &&
		eType != PX_COLLISION_PROXY_SHAPE_TYPE::TRIANGLE_MESH))
		return nullptr;

	const std::filesystem::path normalizedPath = std::filesystem::path{ path }.lexically_normal();
	const std::string key = (eType == PX_COLLISION_PROXY_SHAPE_TYPE::CONVEX_MESH ? "C:" : "T:") +
		normalizedPath.generic_string();

	if (const auto iter = m_CookedMeshDebugCache.find(key); iter != m_CookedMeshDebugCache.end())
	{
		return !iter->second.vertices.empty() && !iter->second.indices.empty()
			? &iter->second : nullptr;
	}

	auto& debugData = m_CookedMeshDebugCache[key];

	if (eType == PX_COLLISION_PROXY_SHAPE_TYPE::CONVEX_MESH)
	{
		auto resource = CResPhysXConvexGeometry::CreateAndLoad(normalizedPath.generic_string());
		auto* mesh = resource ? resource->GetConvexMesh() : nullptr;
		if (!mesh || mesh->getNbVertices() == 0 || mesh->getNbPolygons() == 0)
			return nullptr;

		const physx::PxVec3* vertices = mesh->getVertices();
		debugData.vertices.reserve(mesh->getNbVertices());
		for (physx::PxU32 i = 0; i < mesh->getNbVertices(); ++i)
			debugData.vertices.push_back({ vertices[i].x, vertices[i].y, vertices[i].z });

		const physx::PxU8* indexBuffer = mesh->getIndexBuffer();
		for (physx::PxU32 polygonIndex = 0; polygonIndex < mesh->getNbPolygons(); ++polygonIndex)
		{
			physx::PxHullPolygon polygon{};
			if (!mesh->getPolygonData(polygonIndex, polygon) || polygon.mNbVerts < 3)
				continue;
			const uint32_t first = indexBuffer[polygon.mIndexBase];
			for (physx::PxU32 i = 1; i + 1 < polygon.mNbVerts; ++i)
			{
				debugData.indices.push_back(first);
				debugData.indices.push_back(indexBuffer[polygon.mIndexBase + i]);
				debugData.indices.push_back(indexBuffer[polygon.mIndexBase + i + 1]);
			}
		}
	}
	else
	{
		auto resource = CResPhysXTriMeshGeometry::CreateAndLoad(normalizedPath.generic_string());
		auto* mesh = resource ? resource->GetTriMesh() : nullptr;
		if (!mesh || mesh->getNbVertices() == 0 || mesh->getNbTriangles() == 0)
			return nullptr;

		const physx::PxVec3* vertices = mesh->getVertices();
		debugData.vertices.reserve(mesh->getNbVertices());
		for (physx::PxU32 i = 0; i < mesh->getNbVertices(); ++i)
			debugData.vertices.push_back({ vertices[i].x, vertices[i].y, vertices[i].z });

		const size_t indexCount = static_cast<size_t>(mesh->getNbTriangles()) * 3;
		debugData.indices.resize(indexCount);
		if (mesh->getTriangleMeshFlags().isSet(physx::PxTriangleMeshFlag::e16_BIT_INDICES))
		{
			const auto* indices = static_cast<const physx::PxU16*>(mesh->getTriangles());
			std::transform(indices, indices + indexCount, debugData.indices.begin(),
				[](physx::PxU16 index) { return static_cast<uint32_t>(index); });
		}
		else
		{
			const auto* indices = static_cast<const physx::PxU32*>(mesh->getTriangles());
			std::copy(indices, indices + indexCount, debugData.indices.begin());
		}
	}

	return !debugData.vertices.empty() && !debugData.indices.empty() ? &debugData : nullptr;
}

void CPhysXCollisionProxyEditor::RenderGizmo()
{
	auto* actor = GetSelectedActor();
	auto* camera = CGameInstance::Get().GetActiveCamera();
	if (!actor || !camera)
		return;

	auto* shape = GetSelectedShape();
	if (!shape && m_GizmoOperation == ImGuizmo::SCALE)
		return;

	_float4x4 view{};
	_float4x4 projection{};
	_float4x4 world{};
	XMStoreFloat4x4(&view, camera->GetView());
	XMStoreFloat4x4(&projection, camera->GetProj());
	XMStoreFloat4x4(&world, shape ? MakeShapeWorldMatrix(*actor, *shape) : MakeActorMatrix(*actor));

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	if (!viewport)
		return;

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList(viewport));
	ImGuizmo::SetRect(viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y);
	ImGuizmo::SetID(PX_COLLISION_PROXY_GIZMO_ID);

	if (ImGuizmo::Manipulate(&view._11, &projection._11, m_GizmoOperation,
		m_GizmoOperation == ImGuizmo::SCALE ? ImGuizmo::LOCAL : m_GizmoMode, &world._11))
	{
		if (!m_GizmoStartSnapshot)
			m_GizmoStartSnapshot = SNAPSHOT{ m_Actors, m_SelectedActorID, m_SelectedShapeID };

		_vector scale{};
		_vector rotation{};
		_vector translation{};
		if (shape)
		{
			const _matrix local = XMLoadFloat4x4(&world) * XMMatrixInverse(nullptr, MakeActorMatrix(*actor));
			if (XMMatrixDecompose(&scale, &rotation, &translation, local))
			{
				XMStoreFloat3(&shape->vLocalPosition, translation);
				XMStoreFloat4(&shape->vLocalRotation, XMQuaternionNormalize(rotation));
				_float3 dimensions{};
				XMStoreFloat3(&dimensions, scale);
				ApplyGizmoScale(*shape, dimensions);
			}
		}
		else if (XMMatrixDecompose(&scale, &rotation, &translation, XMLoadFloat4x4(&world)))
		{
			XMStoreFloat3(&actor->vPosition, translation);
			XMStoreFloat4(&actor->vRotation, XMQuaternionNormalize(rotation));
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
		SelectAtMouse();
}

void CPhysXCollisionProxyEditor::CreateActor(const _float3& position)
{
	PushUndo();
	PX_COLLISION_PROXY_ACTOR actor{};
	actor.iID = m_iNextID++;
	actor.sName = "CollisionActor_" + std::to_string(actor.iID);
	actor.vPosition = position;
	m_SelectedActorID = actor.iID;
	m_SelectedShapeID.reset();
	m_Actors.push_back(std::move(actor));
}

void CPhysXCollisionProxyEditor::CreateShapeAtCamera(PX_COLLISION_PROXY_SHAPE_TYPE eType)
{
	auto* camera = CGameInstance::Get().GetActiveCamera();
	if (!camera)
		return;

	const _matrix inverseView = XMMatrixInverse(nullptr, camera->GetView());
	_float3 worldPosition{};
	XMStoreFloat3(&worldPosition,
		inverseView.r[3] + XMVector3Normalize(inverseView.r[2]) * 5.f);

	if (!GetSelectedActor())
		CreateActor(worldPosition);

	auto* actor = GetSelectedActor();
	if (!actor)
		return;

	PushUndo();
	PX_COLLISION_PROXY_SHAPE shape{};
	shape.iID = m_iNextID++;
	shape.sName = std::string{ SHAPE_TYPE_NAMES[static_cast<size_t>(eType)] } + "_" + std::to_string(shape.iID);
	shape.eType = eType;
	_vector localPosition = XMVector3TransformCoord(XMLoadFloat3(&worldPosition),
		XMMatrixInverse(nullptr, MakeActorMatrix(*actor)));
	XMStoreFloat3(&shape.vLocalPosition, localPosition);
	actor->shapes.push_back(std::move(shape));
	m_SelectedShapeID = actor->shapes.back().iID;
}

void CPhysXCollisionProxyEditor::CreateCylinderAtCamera()
{
	CreateShapeAtCamera(PX_COLLISION_PROXY_SHAPE_TYPE::CONVEX_MESH);
	auto* shape = GetSelectedShape();
	if (!shape)
		return;

	shape->sName = "Cylinder_" + std::to_string(shape->iID);
	shape->sCookedResourcePath = PX_UNIT_CYLINDER_CONVEX_PATH;
	shape->vScale = { 1.f, 0.2f, 1.f };
	m_Status = "Cylinder preset created. Build the PhysX preview after cooking the unit cylinder resource.";
}

void CPhysXCollisionProxyEditor::CreateWedgeAtCamera()
{
	CreateShapeAtCamera(PX_COLLISION_PROXY_SHAPE_TYPE::CONVEX_MESH);
	auto* shape = GetSelectedShape();
	if (!shape)
		return;

	shape->sName = "Wedge_" + std::to_string(shape->iID);
	shape->sCookedResourcePath = PX_UNIT_WEDGE_CONVEX_PATH;
	shape->vScale = { 1.f, 0.25f, 1.f };
	m_Status = "Wedge preset created. Build the PhysX preview after cooking the unit wedge resource.";
}

void CPhysXCollisionProxyEditor::DuplicateSelected()
{
	auto* actor = GetSelectedActor();
	if (!actor)
		return;

	if (auto* shape = GetSelectedShape())
	{
		const PX_COLLISION_PROXY_SHAPE source = *shape;
		PushUndo();
		PX_COLLISION_PROXY_SHAPE copy = source;
		copy.iID = m_iNextID++;
		copy.sName += "_Copy";
		copy.vLocalPosition.x += 0.25f;
		copy.vLocalPosition.z += 0.25f;
		actor = GetSelectedActor();
		actor->shapes.push_back(std::move(copy));
		m_SelectedShapeID = actor->shapes.back().iID;
		return;
	}

	const PX_COLLISION_PROXY_ACTOR source = *actor;
	PushUndo();
	PX_COLLISION_PROXY_ACTOR copy = source;
	copy.iID = m_iNextID++;
	copy.sName += "_Copy";
	copy.vPosition.x += 0.5f;
	for (auto& shape : copy.shapes) shape.iID = m_iNextID++;
	m_SelectedActorID = copy.iID;
	m_SelectedShapeID.reset();
	m_Actors.push_back(std::move(copy));
}

void CPhysXCollisionProxyEditor::DeleteSelected()
{
	auto* actor = GetSelectedActor();
	if (!actor)
		return;

	PushUndo();
	if (m_SelectedShapeID)
	{
		std::erase_if(actor->shapes, [id = *m_SelectedShapeID](const auto& shape) { return shape.iID == id; });
		m_SelectedShapeID.reset();
		return;
	}

	std::erase_if(m_Actors, [id = *m_SelectedActorID](const auto& value) { return value.iID == id; });
	m_SelectedActorID.reset();
	m_SelectedShapeID.reset();
}

void CPhysXCollisionProxyEditor::SelectAtMouse()
{
	_float3 origin{};
	_float3 direction{};
	if (!MakeMouseRay(origin, direction))
		return;

	_float nearest = FLT_MAX;
	std::optional<uint64_t> actorID{};
	std::optional<uint64_t> shapeID{};
	for (const auto& actor : m_Actors)
	{
		for (const auto& shape : actor.shapes)
		{
			_vector scale{};
			_vector rotation{};
			_vector translation{};
			if (!XMMatrixDecompose(&scale, &rotation, &translation, MakeShapeWorldMatrix(actor, shape)))
				continue;

			BoundingOrientedBox bounds{};
			XMStoreFloat3(&bounds.Center, translation);
			_float3 dimensions{};
			XMStoreFloat3(&dimensions, scale);
			bounds.Extents = { std::abs(dimensions.x) * 0.5f,
				std::abs(dimensions.y) * 0.5f, std::abs(dimensions.z) * 0.5f };
			XMStoreFloat4(&bounds.Orientation, XMQuaternionNormalize(rotation));

			_float distance{};
			if (bounds.Intersects(XMLoadFloat3(&origin), XMLoadFloat3(&direction), distance) && distance < nearest)
			{
				nearest = distance;
				actorID = actor.iID;
				shapeID = shape.iID;
			}
		}
	}

	m_SelectedActorID = actorID;
	m_SelectedShapeID = shapeID;
}

_bool CPhysXCollisionProxyEditor::MakeMouseRay(_float3& outOrigin, _float3& outDirection) const
{
	auto* camera = CGameInstance::Get().GetActiveCamera();
	if (!camera) return false;
	const _float2 mouse = CGameInstance::Get().GetMousePos();
	const _float2 size = CGameInstance::Get().GetClientScreenSize();
	if (size.x <= 0.f || size.y <= 0.f || mouse.x < 0.f || mouse.y < 0.f || mouse.x >= size.x || mouse.y >= size.y)
		return false;

	const _vector nearPoint = XMVector3Unproject(XMVectorSet(mouse.x, mouse.y, 0.f, 1.f),
		0.f, 0.f, size.x, size.y, 0.f, 1.f, camera->GetProj(), camera->GetView(), XMMatrixIdentity());
	const _vector farPoint = XMVector3Unproject(XMVectorSet(mouse.x, mouse.y, 1.f, 1.f),
		0.f, 0.f, size.x, size.y, 0.f, 1.f, camera->GetProj(), camera->GetView(), XMMatrixIdentity());
	XMStoreFloat3(&outOrigin, nearPoint);
	XMStoreFloat3(&outDirection, XMVector3Normalize(farPoint - nearPoint));
	return true;
}

PX_COLLISION_PROXY_ACTOR* CPhysXCollisionProxyEditor::GetSelectedActor()
{
	return const_cast<PX_COLLISION_PROXY_ACTOR*>(
		static_cast<const CPhysXCollisionProxyEditor*>(this)->GetSelectedActor());
}

const PX_COLLISION_PROXY_ACTOR* CPhysXCollisionProxyEditor::GetSelectedActor() const
{
	if (!m_SelectedActorID) return nullptr;
	const auto iter = std::find_if(m_Actors.begin(), m_Actors.end(),
		[id = *m_SelectedActorID](const auto& actor) { return actor.iID == id; });
	return iter == m_Actors.end() ? nullptr : &*iter;
}

PX_COLLISION_PROXY_SHAPE* CPhysXCollisionProxyEditor::GetSelectedShape()
{
	return const_cast<PX_COLLISION_PROXY_SHAPE*>(
		static_cast<const CPhysXCollisionProxyEditor*>(this)->GetSelectedShape());
}

const PX_COLLISION_PROXY_SHAPE* CPhysXCollisionProxyEditor::GetSelectedShape() const
{
	const auto* actor = GetSelectedActor();
	if (!actor || !m_SelectedShapeID) return nullptr;
	const auto iter = std::find_if(actor->shapes.begin(), actor->shapes.end(),
		[id = *m_SelectedShapeID](const auto& shape) { return shape.iID == id; });
	return iter == actor->shapes.end() ? nullptr : &*iter;
}

void CPhysXCollisionProxyEditor::PushUndo()
{
	m_UndoStack.push_back({ m_Actors, m_SelectedActorID, m_SelectedShapeID });
	if (m_UndoStack.size() > MAX_UNDO_HISTORY) m_UndoStack.erase(m_UndoStack.begin());
	m_RedoStack.clear();
}

void CPhysXCollisionProxyEditor::Undo()
{
	if (m_UndoStack.empty()) return;
	m_RedoStack.push_back({ m_Actors, m_SelectedActorID, m_SelectedShapeID });
	auto snapshot = std::move(m_UndoStack.back());
	m_UndoStack.pop_back();
	RestoreSnapshot(std::move(snapshot));
}

void CPhysXCollisionProxyEditor::Redo()
{
	if (m_RedoStack.empty()) return;
	m_UndoStack.push_back({ m_Actors, m_SelectedActorID, m_SelectedShapeID });
	auto snapshot = std::move(m_RedoStack.back());
	m_RedoStack.pop_back();
	RestoreSnapshot(std::move(snapshot));
}

void CPhysXCollisionProxyEditor::RestoreSnapshot(SNAPSHOT snapshot)
{
	m_Actors = std::move(snapshot.actors);
	m_SelectedActorID = snapshot.selectedActorID;
	m_SelectedShapeID = snapshot.selectedShapeID;
	RecalculateNextID();
}

void CPhysXCollisionProxyEditor::RecalculateNextID()
{
	m_iNextID = 1;
	for (const auto& actor : m_Actors)
	{
		m_iNextID = std::max(m_iNextID, actor.iID + 1);
		for (const auto& shape : actor.shapes)
			m_iNextID = std::max(m_iNextID, shape.iID + 1);
	}
}

HRESULT CPhysXCollisionProxyEditor::Save() const
{
	const std::filesystem::path filePath = MakePxCollisionFilePath(m_CollisionFileName);
	std::error_code error{};
	std::filesystem::create_directories(filePath.parent_path(), error);
	if (error) return E_FAIL;

	PX_COLLISION_PROXY_FILE data{};
	data.actors = m_Actors;
	return CGameInstance::Get().JsonSerialize(filePath.generic_string(), data, "CollisionProxies");
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
	if (FAILED(CGameInstance::Get().JsonDeSerialize(filePath.generic_string(), data, "CollisionProxies")))
	{
		m_Status = "Load failed: " + filePath.generic_string();
		return E_FAIL;
	}
	if (data.iVersion != 3)
	{
		m_Status = "Unsupported collision proxy version (expected 3).";
		return E_FAIL;
	}

	for (auto& actor : data.actors)
	{
		NormalizeQuaternion(actor.vRotation);
		actor.fMass = std::max(actor.fMass, 0.001f);
		for (auto& shape : actor.shapes)
		{
			NormalizeQuaternion(shape.vLocalRotation);
			shape.vSize = { std::max(std::abs(shape.vSize.x), MIN_SHAPE_SIZE),
				std::max(std::abs(shape.vSize.y), MIN_SHAPE_SIZE),
				std::max(std::abs(shape.vSize.z), MIN_SHAPE_SIZE) };
			shape.vScale = { std::max(std::abs(shape.vScale.x), MIN_SHAPE_SIZE),
				std::max(std::abs(shape.vScale.y), MIN_SHAPE_SIZE),
				std::max(std::abs(shape.vScale.z), MIN_SHAPE_SIZE) };
			shape.fRadius = std::max(shape.fRadius, MIN_SHAPE_SIZE);
			shape.fHalfHeight = std::max(shape.fHalfHeight, MIN_SHAPE_SIZE);
			if (shape.eType == PX_COLLISION_PROXY_SHAPE_TYPE::TRIANGLE_MESH)
				shape.bTrigger = false;
		}
	}

	m_Actors = std::move(data.actors);
	RemovePhysicsPreview();
	m_SelectedActorID.reset();
	m_SelectedShapeID.reset();
	m_UndoStack.clear();
	m_RedoStack.clear();
	RecalculateNextID();
	m_Status = "Loaded: " + filePath.generic_string();
	return S_OK;
}

void CPhysXCollisionProxyEditor::Clear()
{
	RemovePhysicsPreview();
	m_CookedMeshDebugCache.clear();
	m_Actors.clear();
	m_SelectedActorID.reset();
	m_SelectedShapeID.reset();
	m_UndoStack.clear();
	m_RedoStack.clear();
	m_iNextID = 1;
}

void CPhysXCollisionProxyEditor::CreatePhysicsPreview()
{
	RemovePhysicsPreview();
	if (m_Actors.empty())
	{
		m_Status = "PhysX preview failed: no collision actors.";
		return;
	}

	size_t iEnabledActorCount{};
	size_t iEnabledShapeCount{};
	for (const auto& actor : m_Actors)
	{
		if (!actor.bEnabled)
			continue;

		++iEnabledActorCount;
		for (const auto& shape : actor.shapes)
		{
			if (!shape.bEnabled)
				continue;

			++iEnabledShapeCount;
			if (shape.eType == PX_COLLISION_PROXY_SHAPE_TYPE::TRIANGLE_MESH &&
				(actor.eType == PX_COLLISION_PROXY_ACTOR_TYPE::DYNAMIC || shape.bTrigger))
			{
				m_Status = "PhysX preview failed: Triangle Mesh cannot be Dynamic or Trigger.";
				return;
			}
			if ((shape.eType == PX_COLLISION_PROXY_SHAPE_TYPE::CONVEX_MESH ||
				shape.eType == PX_COLLISION_PROXY_SHAPE_TYPE::TRIANGLE_MESH) &&
				shape.sCookedResourcePath.empty())
			{
				m_Status = "PhysX preview failed: Cooked Path is empty for a mesh shape.";
				return;
			}
		}
	}

	if (iEnabledActorCount == 0)
	{
		m_Status = "PhysX preview failed: no enabled collision actors.";
		return;
	}
	if (iEnabledShapeCount == 0)
	{
		m_Status = "PhysX preview failed: no enabled collision shapes.";
		return;
	}

	PX_COLLISION_PROXY_FILE data{};
	data.actors = m_Actors;
	m_PhysicsPreviewHandles = CGameInstance::Get().GetPhysXManager()->CreateCollisionProxyObjects(
		data, "__ENGINE_PX_COLLISION_PREVIEW");
	m_Status = !m_PhysicsPreviewHandles.empty()
		? "PhysX preview created from current editor data."
		: "PhysX preview creation failed. Check Shape settings and cooked paths.";
}

void CPhysXCollisionProxyEditor::RemovePhysicsPreview()
{
	for (const CHandle& handle : m_PhysicsPreviewHandles)
	{
		if (auto* preview = CGameInstance::Get().GetGameObjectByHandle(handle))
			preview->SetPendingDestroyCascade();
	}
	m_PhysicsPreviewHandles.clear();
}

void CPhysXCollisionProxyEditor::QueueResultPopup(std::string message, _bool success)
{
	m_ResultPopupMessage = std::move(message);
	m_bResultPopupSuccess = success;
	m_bOpenResultPopup = true;
}

UPtr<CPhysXCollisionProxyEditor> CPhysXCollisionProxyEditor::Create()
{
	return ToUPtr(new CPhysXCollisionProxyEditor{});
}
