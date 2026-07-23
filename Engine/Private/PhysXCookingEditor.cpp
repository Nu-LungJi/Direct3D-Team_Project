#include "pch.h"
#include "PhysXCookingEditor.h"

#include "Engine_PhysxDefines.h"
#include "ResPhysXConvexGeometry.h"
#include "ResPhysXRTConvexGeometry.h"
#include "ResPhysXTriMeshGeometry.h"
#include "ResPhysXRTTriMeshGeometry.h"

#include <filesystem>
#include <fstream>

NS_USING(Engine)

namespace
{
	constexpr int32_t MIN_CYLINDER_SEGMENTS = 3;
	constexpr int32_t MAX_CYLINDER_SEGMENTS = 64;
	const char* SOURCE_TYPE_NAMES[] = {
		"Procedural Primitive",
		"Static Model BIN"
	};

	const char* PRIMITIVE_TYPE_NAMES[] = {
		"Unit Cylinder",
		"Unit Octagonal Prism",
		"Unit Wedge"
	};

	const char* COOK_TYPE_NAMES[] = {
		"Convex Mesh",
		"Triangle Mesh"
	};

	template<typename T>
	_bool ReadValue(const std::byte*& cursor, const std::byte* end, T& outValue)
	{
		if (static_cast<size_t>(end - cursor) < sizeof(T))
			return false;
		std::memcpy(&outValue, cursor, sizeof(T));
		cursor += sizeof(T);
		return true;
	}

	_bool ReadBytes(const std::byte*& cursor, const std::byte* end, void* destination, size_t size)
	{
		if (size > static_cast<size_t>(end - cursor))
			return false;
		if (size > 0)
			std::memcpy(destination, cursor, size);
		cursor += size;
		return true;
	}
}

CPhysXCookingEditor::CPhysXCookingEditor()
{
	strncpy_s(m_OutputPath, PX_UNIT_CYLINDER_CONVEX_PATH, _TRUNCATE);
}

void CPhysXCookingEditor::UpdateGUI()
{
	if (!m_bOpen)
		return;

	DrawWindow();
}

void CPhysXCookingEditor::DrawWindow()
{
	ImGui::SetNextWindowSize(ImVec2(540.f, 430.f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("PhysX Cooking Editor", &m_bOpen))
	{
		ImGui::End();
		return;
	}
	const auto getDefaultPrimitivePath = [this]() -> const char*
	{
		switch (m_ePrimitiveType)
		{
		case PRIMITIVE_TYPE::CYLINDER:
			return PX_UNIT_CYLINDER_CONVEX_PATH;
		case PRIMITIVE_TYPE::OCTAGONAL_PRISM:
			return PX_UNIT_OCTAGONAL_PRISM_CONVEX_PATH;
		case PRIMITIVE_TYPE::WEDGE:
			return PX_UNIT_WEDGE_CONVEX_PATH;
		default:
			return "";
		}
	};

	int sourceType = static_cast<int>(m_eSourceType);
	if (ImGui::Combo("Source Type", &sourceType, SOURCE_TYPE_NAMES,
		static_cast<int>(std::size(SOURCE_TYPE_NAMES))))
	{
		m_eSourceType = static_cast<SOURCE_TYPE>(sourceType);
		if (m_eSourceType == SOURCE_TYPE::PROCEDURAL_PRIMITIVE)
		{
			strncpy_s(m_OutputPath, getDefaultPrimitivePath(), _TRUNCATE);
		}
		else
		{
			strncpy_s(m_OutputPath, "./Resources/PhysX/Cooked/Model.pxconvex", _TRUNCATE);
		}
	}

	if (m_eSourceType == SOURCE_TYPE::PROCEDURAL_PRIMITIVE)
	{
		int primitiveType = static_cast<int>(m_ePrimitiveType);
		if (ImGui::Combo("Primitive", &primitiveType, PRIMITIVE_TYPE_NAMES,
			static_cast<int>(std::size(PRIMITIVE_TYPE_NAMES))))
		{
			m_ePrimitiveType = static_cast<PRIMITIVE_TYPE>(primitiveType);
			strncpy_s(m_OutputPath, getDefaultPrimitivePath(), _TRUNCATE);
		}

		if (m_ePrimitiveType == PRIMITIVE_TYPE::CYLINDER)
		{
			ImGui::SliderInt("Cylinder Segments", &m_iCylinderSegments,
				MIN_CYLINDER_SEGMENTS, MAX_CYLINDER_SEGMENTS);
			ImGui::TextDisabled("Unit cylinder: radius 0.5, height 1.0, Y axis");
			ImGui::Text("Expected vertices: %d", m_iCylinderSegments * 2);
		}
		else if (m_ePrimitiveType == PRIMITIVE_TYPE::OCTAGONAL_PRISM)
		{
			ImGui::TextDisabled("Unit octagonal prism: radius 0.5, height 1.0, Y axis");
			ImGui::Text("Expected vertices: %d", PX_UNIT_OCTAGONAL_PRISM_SEGMENTS * 2);
		}
		else
		{
			ImGui::TextDisabled("Unit wedge: width 1.0, height 1.0, length 1.0");
			ImGui::TextDisabled("Ramp rises from -Z to +Z. X=width, Y=height, Z=length.");
			ImGui::Text("Expected vertices: 6");
		}
	}
	else
	{
		ImGui::InputText("Model BIN Path", m_ModelInputPath, std::size(m_ModelInputPath));
		if (ImGui::Button("Inspect Model BIN", ImVec2(160.f, 0.f)))
		{
			const _bool success = SUCCEEDED(InspectModelBin());
			QueueResult(m_Status, success);
		}

		const char* meshPreview = "No model inspected";
		if (!m_ModelMeshes.empty())
		{
			meshPreview = m_iSelectedModelMesh < 0
				? "All Meshes (Merged)"
				: m_ModelMeshes[static_cast<size_t>(m_iSelectedModelMesh)].name.c_str();
		}
		if (ImGui::BeginCombo("Mesh", meshPreview))
		{
			if (ImGui::Selectable("All Meshes (Merged)", m_iSelectedModelMesh < 0))
				m_iSelectedModelMesh = -1;
			for (size_t i = 0; i < m_ModelMeshes.size(); ++i)
			{
				const auto& mesh = m_ModelMeshes[i];
				const std::string label = std::to_string(i) + ": " + mesh.name +
					" (V=" + std::to_string(mesh.geometry.vertices.size()) +
					", I=" + std::to_string(mesh.geometry.indices.size()) + ")";
				if (ImGui::Selectable(label.c_str(), m_iSelectedModelMesh == static_cast<int32_t>(i)))
					m_iSelectedModelMesh = static_cast<int32_t>(i);
			}
			ImGui::EndCombo();
		}

		ImGui::DragFloat3("Source Scale", &m_vModelScale.x, 0.01f, -10000.f, 10000.f);
		ImGui::DragFloat3("Source Rotation (Deg)", &m_vModelRotation.x, 0.5f, -360.f, 360.f);
		ImGui::DragFloat3("Source Translation", &m_vModelTranslation.x, 0.01f, -100000.f, 100000.f);
		ImGui::TextDisabled("Transform is baked into the cooked vertices.");
	}

	ImGui::Separator();
	int cookType = static_cast<int>(m_eCookType);
	if (ImGui::Combo("Cook Type", &cookType, COOK_TYPE_NAMES,
		static_cast<int>(std::size(COOK_TYPE_NAMES))))
	{
		m_eCookType = static_cast<COOK_TYPE>(cookType);
		std::filesystem::path path{ m_OutputPath };
		path.replace_extension(m_eCookType == COOK_TYPE::CONVEX_MESH
			? ".pxconvex" : ".pxtrimesh");
		strncpy_s(m_OutputPath, path.generic_string().c_str(), _TRUNCATE);
	}

	ImGui::InputText("Output Path", m_OutputPath, std::size(m_OutputPath));
	ImGui::Checkbox("Overwrite Existing File", &m_bOverwrite);

	const _bool supported =
		(m_eSourceType == SOURCE_TYPE::PROCEDURAL_PRIMITIVE &&
			m_eCookType == COOK_TYPE::CONVEX_MESH) ||
		(m_eSourceType == SOURCE_TYPE::MODEL_RESOURCE && !m_ModelMeshes.empty());

	if (!supported)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}
	if (ImGui::Button("Cook and Validate", ImVec2(180.f, 0.f)))
	{
		const _bool success = SUCCEEDED(Cook());
		QueueResult(m_Status, success);
	}
	if (!supported)
	{
		ImGui::PopStyleVar();
		ImGui::PopItemFlag();
	}

	if (!supported)
		ImGui::TextDisabled("Inspect a static Model BIN first. Procedural primitives support Convex only.");

	ImGui::Separator();
	ImGui::TextWrapped("Status: %s", m_Status.c_str());
	if (m_iLastVertexCount > 0)
	{
		ImGui::Text("Last source: %u vertices, %u indices",
			m_iLastVertexCount, m_iLastIndexCount);
	}

	if (m_bOpenResultPopup)
	{
		ImGui::OpenPopup("PhysX Cooking Result");
		m_bOpenResultPopup = false;
	}
	if (ImGui::BeginPopupModal("PhysX Cooking Result", nullptr,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		const ImVec4 color = m_bResultSuccess
			? ImVec4(0.25f, 1.f, 0.35f, 1.f)
			: ImVec4(1.f, 0.3f, 0.2f, 1.f);
		ImGui::TextColored(color, "%s", m_bResultSuccess ? "Success" : "Failed");
		ImGui::Separator();
		ImGui::TextWrapped("%s", m_ResultMessage.c_str());
		if (ImGui::Button("OK", ImVec2(120.f, 0.f)))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	ImGui::End();
}

HRESULT CPhysXCookingEditor::Cook()
{
	m_iLastVertexCount = 0;
	m_iLastIndexCount = 0;

	if (m_OutputPath[0] == '\0')
	{
		m_Status = "Cooking failed: output path is empty.";
		return E_INVALIDARG;
	}

	const std::filesystem::path outputPath =
		std::filesystem::path{ m_OutputPath }.lexically_normal();
	std::error_code ec{};
	if (std::filesystem::exists(outputPath, ec) && !m_bOverwrite)
	{
		m_Status = "Cooking cancelled: output file already exists. Enable overwrite to replace it.";
		return E_FAIL;
	}
	if (ec)
	{
		m_Status = "Cooking failed: output path could not be inspected.";
		return E_FAIL;
	}

	SOURCE_GEOMETRY geometry{};
	if (FAILED(BuildSourceGeometry(geometry)))
	{
		m_Status = "Cooking failed: source geometry could not be generated.";
		return E_FAIL;
	}

	m_iLastVertexCount = static_cast<uint32_t>(geometry.vertices.size());
	m_iLastIndexCount = static_cast<uint32_t>(geometry.indices.size());
	const _string normalizedOutputPath = outputPath.generic_string();
	if (FAILED(CookGeometry(geometry, normalizedOutputPath)))
	{
		m_Status = "Cooking failed: PhysX convex cooking failed.";
		return E_FAIL;
	}

	if (FAILED(ValidateCookedFile(normalizedOutputPath)))
	{
		m_Status = "Cooking failed: the written file could not be loaded for validation.";
		return E_FAIL;
	}

	m_Status = "Cooked and validated: " + normalizedOutputPath;
	return S_OK;
}

HRESULT CPhysXCookingEditor::InspectModelBin()
{
	m_ModelMeshes.clear();
	m_iSelectedModelMesh = -1;

	if (m_ModelInputPath[0] == '\0')
	{
		m_Status = "Model inspection failed: input path is empty.";
		return E_INVALIDARG;
	}

	std::ifstream file(m_ModelInputPath, std::ios::binary | std::ios::ate);
	if (!file)
	{
		m_Status = "Model inspection failed: file could not be opened.";
		return E_FAIL;
	}

	const std::streamoff fileSize = file.tellg();
	if (fileSize <= 0 || static_cast<uint64_t>(fileSize) > std::numeric_limits<size_t>::max())
	{
		m_Status = "Model inspection failed: invalid file size.";
		return E_FAIL;
	}
	file.seekg(0, std::ios::beg);
	std::vector<std::byte> bytes(static_cast<size_t>(fileSize));
	if (!file.read(reinterpret_cast<char*>(bytes.data()), fileSize))
	{
		m_Status = "Model inspection failed: file read failed.";
		return E_FAIL;
	}

	const std::byte* cursor = bytes.data();
	const std::byte* end = cursor + bytes.size();
	MODEL_FILE_HEADER header{};
	if (!ReadValue(cursor, end, header) || header.MeshCount == 0)
	{
		m_Status = "Model inspection failed: invalid or empty model header.";
		return E_FAIL;
	}
	if (header.bHasBone || header.bHasAnimation)
	{
		m_Status = "Model inspection failed: animated/skeletal BIN is not supported by this cooker.";
		return E_NOTIMPL;
	}
	if (header.MeshCount > 100000u)
	{
		m_Status = "Model inspection failed: unreasonable mesh count.";
		return E_FAIL;
	}

	while (cursor < end)
	{
		CHUCKHEADER chunk{};
		if (!ReadValue(cursor, end, chunk) || chunk.size > static_cast<size_t>(end - cursor))
		{
			m_Status = "Model inspection failed: invalid chunk bounds.";
			m_ModelMeshes.clear();
			return E_FAIL;
		}

		const std::byte* chunkEnd = cursor + chunk.size;
		if (chunk.type != CHUNCK_TYPE::CHUNK_MESH)
		{
			cursor = chunkEnd;
			continue;
		}

		for (uint32_t meshIndex = 0; meshIndex < header.MeshCount; ++meshIndex)
		{
			uint32_t recordSize{};
			if (!ReadValue(cursor, chunkEnd, recordSize) || recordSize > static_cast<size_t>(chunkEnd - cursor))
				break;
			const std::byte* recordEnd = cursor + recordSize;

			uint32_t nameLength{};
			uint32_t materialIndex{};
			_float3 minPosition{};
			_float3 maxPosition{};
			uint32_t vertexCount{};
			uint32_t indexCount{};
			if (!ReadValue(cursor, recordEnd, nameLength) || nameLength > static_cast<size_t>(recordEnd - cursor))
				break;

			MODEL_MESH_SOURCE mesh{};
			mesh.name.resize(nameLength);
			if (!ReadBytes(cursor, recordEnd, mesh.name.data(), nameLength) ||
				!ReadValue(cursor, recordEnd, materialIndex) ||
				!ReadValue(cursor, recordEnd, minPosition) ||
				!ReadValue(cursor, recordEnd, maxPosition) ||
				!ReadValue(cursor, recordEnd, vertexCount) ||
				!ReadValue(cursor, recordEnd, indexCount) ||
				vertexCount < 3 || indexCount < 3 || indexCount % 3 != 0 ||
				vertexCount > 100000000u || indexCount > 300000000u)
				break;

			const size_t vertexBytes = static_cast<size_t>(vertexCount) * sizeof(VTXMESH);
			const size_t indexBytes = static_cast<size_t>(indexCount) * sizeof(uint32_t);
			if (vertexBytes > static_cast<size_t>(recordEnd - cursor))
				break;

			mesh.geometry.vertices.resize(vertexCount);
			for (uint32_t i = 0; i < vertexCount; ++i)
			{
				VTXMESH vertex{};
				if (!ReadValue(cursor, recordEnd, vertex))
					break;
				mesh.geometry.vertices[i] = vertex.vPosition;
			}
			mesh.geometry.indices.resize(indexCount);
			if (indexBytes > static_cast<size_t>(recordEnd - cursor) ||
				!ReadBytes(cursor, recordEnd, mesh.geometry.indices.data(), indexBytes))
				break;

			const _bool indicesValid = std::all_of(mesh.geometry.indices.begin(), mesh.geometry.indices.end(),
				[vertexCount](uint32_t index) { return index < vertexCount; });
			if (!indicesValid || cursor != recordEnd)
				break;

			if (mesh.name.empty())
				mesh.name = "Mesh_" + std::to_string(meshIndex);
			m_ModelMeshes.push_back(std::move(mesh));
		}

		cursor = chunkEnd;
		break;
	}

	if (m_ModelMeshes.size() != header.MeshCount)
	{
		m_ModelMeshes.clear();
		m_Status = "Model inspection failed: mesh records do not match the static BIN format.";
		return E_FAIL;
	}

	m_Status = "Model inspected: " + std::to_string(m_ModelMeshes.size()) + " meshes.";
	return S_OK;
}

HRESULT CPhysXCookingEditor::BuildSourceGeometry(SOURCE_GEOMETRY& outGeometry) const
{
	outGeometry = {};
	if (m_eSourceType != SOURCE_TYPE::PROCEDURAL_PRIMITIVE)
		return BuildModelGeometry(outGeometry);

	switch (m_ePrimitiveType)
	{
	case PRIMITIVE_TYPE::CYLINDER:
		return BuildUnitCylinder(outGeometry);
	case PRIMITIVE_TYPE::OCTAGONAL_PRISM:
		return BuildUnitOctagonalPrism(outGeometry);
	case PRIMITIVE_TYPE::WEDGE:
		return BuildUnitWedge(outGeometry);
	default:
		return E_NOTIMPL;
	}
}

HRESULT CPhysXCookingEditor::BuildUnitCylinder(SOURCE_GEOMETRY& outGeometry) const
{
	const uint32_t segmentCount = static_cast<uint32_t>(
		std::clamp(m_iCylinderSegments, MIN_CYLINDER_SEGMENTS, MAX_CYLINDER_SEGMENTS));
	return BuildUnitPrism(outGeometry, segmentCount);
}

HRESULT CPhysXCookingEditor::BuildUnitOctagonalPrism(SOURCE_GEOMETRY& outGeometry) const
{
	return BuildUnitPrism(outGeometry, PX_UNIT_OCTAGONAL_PRISM_SEGMENTS, XM_PI / 8.f);
}

HRESULT CPhysXCookingEditor::BuildUnitPrism(
	SOURCE_GEOMETRY& outGeometry, uint32_t segmentCount, _float angleOffset) const
{
	if (segmentCount < static_cast<uint32_t>(MIN_CYLINDER_SEGMENTS))
		return E_INVALIDARG;

	outGeometry.vertices.clear();
	outGeometry.indices.clear();
	outGeometry.vertices.reserve(segmentCount * 2);

	for (uint32_t i = 0; i < segmentCount; ++i)
	{
		const _float angle = angleOffset + XM_2PI * static_cast<_float>(i) /
			static_cast<_float>(segmentCount);
		const _float x = std::cos(angle) * PX_UNIT_CYLINDER_RADIUS;
		const _float z = std::sin(angle) * PX_UNIT_CYLINDER_RADIUS;
		outGeometry.vertices.push_back({ x, -PX_UNIT_CYLINDER_HALF_HEIGHT, z });
		outGeometry.vertices.push_back({ x, PX_UNIT_CYLINDER_HALF_HEIGHT, z });
	}

	return outGeometry.vertices.size() >= 6 ? S_OK : E_FAIL;
}

HRESULT CPhysXCookingEditor::BuildUnitWedge(SOURCE_GEOMETRY& outGeometry) const
{
	constexpr _float h = PX_UNIT_WEDGE_HALF_EXTENT;
	outGeometry.vertices = {
		{ -h, -h, -h }, { h, -h, -h },
		{ -h, -h,  h }, { h, -h,  h },
		{ -h,  h,  h }, { h,  h,  h }
	};
	outGeometry.indices.clear();
	return S_OK;
}

HRESULT CPhysXCookingEditor::BuildModelGeometry(SOURCE_GEOMETRY& outGeometry) const
{
	if (m_ModelMeshes.empty() || m_iSelectedModelMesh >= static_cast<int32_t>(m_ModelMeshes.size()))
		return E_FAIL;

	const _matrix transform =
		XMMatrixScaling(m_vModelScale.x, m_vModelScale.y, m_vModelScale.z) *
		XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(m_vModelRotation.x),
			XMConvertToRadians(m_vModelRotation.y),
			XMConvertToRadians(m_vModelRotation.z)) *
		XMMatrixTranslation(m_vModelTranslation.x, m_vModelTranslation.y, m_vModelTranslation.z);

	auto appendMesh = [&](const MODEL_MESH_SOURCE& mesh) -> _bool
	{
		if (outGeometry.vertices.size() > std::numeric_limits<uint32_t>::max() - mesh.geometry.vertices.size())
			return false;
		const uint32_t baseVertex = static_cast<uint32_t>(outGeometry.vertices.size());
		outGeometry.vertices.reserve(outGeometry.vertices.size() + mesh.geometry.vertices.size());
		for (const _float3& position : mesh.geometry.vertices)
		{
			_float3 transformed{};
			XMStoreFloat3(&transformed, XMVector3TransformCoord(XMLoadFloat3(&position), transform));
			outGeometry.vertices.push_back(transformed);
		}
		outGeometry.indices.reserve(outGeometry.indices.size() + mesh.geometry.indices.size());
		for (const uint32_t index : mesh.geometry.indices)
			outGeometry.indices.push_back(baseVertex + index);
		return true;
	};

	if (m_iSelectedModelMesh >= 0)
	{
		if (!appendMesh(m_ModelMeshes[static_cast<size_t>(m_iSelectedModelMesh)]))
			return E_FAIL;
	}
	else
	{
		for (const auto& mesh : m_ModelMeshes)
		{
			if (!appendMesh(mesh))
				return E_FAIL;
		}
	}

	return !outGeometry.vertices.empty() && !outGeometry.indices.empty() ? S_OK : E_FAIL;
}

HRESULT CPhysXCookingEditor::CookGeometry(
	const SOURCE_GEOMETRY& geometry, const _string& outputPath) const
{
	if (m_eCookType != COOK_TYPE::CONVEX_MESH)
	{
		const auto desc = CResPhysXRTTriMeshGeometry::MakeDesc(
			geometry.vertices, geometry.indices, 0);
		return CResPhysXRTTriMeshGeometry::CookToFile(desc, outputPath);
	}

	const auto desc = CResPhysXRTConvexGeometry::MakeDesc(geometry.vertices, 0);
	return CResPhysXRTConvexGeometry::CookToFile(desc, outputPath);
}

HRESULT CPhysXCookingEditor::ValidateCookedFile(const _string& outputPath) const
{
	if (m_eCookType != COOK_TYPE::CONVEX_MESH)
		return CResPhysXTriMeshGeometry::CreateAndLoad(outputPath) ? S_OK : E_FAIL;

	return CResPhysXConvexGeometry::CreateAndLoad(outputPath) ? S_OK : E_FAIL;
}

void CPhysXCookingEditor::QueueResult(std::string message, _bool success)
{
	m_ResultMessage = std::move(message);
	m_bResultSuccess = success;
	m_bOpenResultPopup = true;
}

UPtr<CPhysXCookingEditor> CPhysXCookingEditor::Create()
{
	return ToUPtr(new CPhysXCookingEditor{});
}

