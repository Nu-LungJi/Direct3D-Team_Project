#include "pch.h"
#include "PhysXCookingEditor.h"

#include "Engine_PhysxDefines.h"
#include "ResPhysXConvexGeometry.h"
#include "ResPhysXRTConvexGeometry.h"

#include <filesystem>

NS_USING(Engine)

namespace
{
	constexpr int32_t MIN_CYLINDER_SEGMENTS = 3;
	constexpr int32_t MAX_CYLINDER_SEGMENTS = 64;
	const char* SOURCE_TYPE_NAMES[] = {
		"Procedural Primitive",
		"Model Resource (Next Phase)"
	};

	const char* PRIMITIVE_TYPE_NAMES[] = {
		"Unit Cylinder",
		"Unit Wedge"
	};

	const char* COOK_TYPE_NAMES[] = {
		"Convex Mesh",
		"Triangle Mesh (Next Phase)"
	};
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

	int sourceType = static_cast<int>(m_eSourceType);
	if (ImGui::Combo("Source Type", &sourceType, SOURCE_TYPE_NAMES,
		static_cast<int>(std::size(SOURCE_TYPE_NAMES))))
	{
		m_eSourceType = static_cast<SOURCE_TYPE>(sourceType);
	}

	if (m_eSourceType == SOURCE_TYPE::PROCEDURAL_PRIMITIVE)
	{
		int primitiveType = static_cast<int>(m_ePrimitiveType);
		if (ImGui::Combo("Primitive", &primitiveType, PRIMITIVE_TYPE_NAMES,
			static_cast<int>(std::size(PRIMITIVE_TYPE_NAMES))))
		{
			m_ePrimitiveType = static_cast<PRIMITIVE_TYPE>(primitiveType);
			const char* defaultPath = m_ePrimitiveType == PRIMITIVE_TYPE::CYLINDER
				? PX_UNIT_CYLINDER_CONVEX_PATH
				: PX_UNIT_WEDGE_CONVEX_PATH;
			strncpy_s(m_OutputPath, defaultPath, _TRUNCATE);
		}

		if (m_ePrimitiveType == PRIMITIVE_TYPE::CYLINDER)
		{
			ImGui::SliderInt("Cylinder Segments", &m_iCylinderSegments,
				MIN_CYLINDER_SEGMENTS, MAX_CYLINDER_SEGMENTS);
			ImGui::TextDisabled("Unit cylinder: radius 0.5, height 1.0, Y axis");
			ImGui::Text("Expected vertices: %d", m_iCylinderSegments * 2);
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
		ImGui::TextDisabled("Model Resource input will be added in the next phase.");
	}

	ImGui::Separator();
	int cookType = static_cast<int>(m_eCookType);
	if (ImGui::Combo("Cook Type", &cookType, COOK_TYPE_NAMES,
		static_cast<int>(std::size(COOK_TYPE_NAMES))))
	{
		m_eCookType = static_cast<COOK_TYPE>(cookType);
	}

	ImGui::InputText("Output Path", m_OutputPath, std::size(m_OutputPath));
	ImGui::Checkbox("Overwrite Existing File", &m_bOverwrite);

	const _bool supported =
		m_eSourceType == SOURCE_TYPE::PROCEDURAL_PRIMITIVE &&
		m_eCookType == COOK_TYPE::CONVEX_MESH;

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
		ImGui::TextDisabled("This source/cook combination is reserved for the model-input phase.");

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

HRESULT CPhysXCookingEditor::BuildSourceGeometry(SOURCE_GEOMETRY& outGeometry) const
{
	outGeometry = {};
	if (m_eSourceType != SOURCE_TYPE::PROCEDURAL_PRIMITIVE)
		return E_NOTIMPL;

	switch (m_ePrimitiveType)
	{
	case PRIMITIVE_TYPE::CYLINDER:
		return BuildUnitCylinder(outGeometry);
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
	outGeometry.vertices.clear();
	outGeometry.indices.clear();
	outGeometry.vertices.reserve(segmentCount * 2);

	for (uint32_t i = 0; i < segmentCount; ++i)
	{
		const _float angle = XM_2PI * static_cast<_float>(i) /
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

HRESULT CPhysXCookingEditor::CookGeometry(
	const SOURCE_GEOMETRY& geometry, const _string& outputPath) const
{
	if (m_eCookType != COOK_TYPE::CONVEX_MESH)
		return E_NOTIMPL;

	const auto desc = CResPhysXRTConvexGeometry::MakeDesc(geometry.vertices, 0);
	return CResPhysXRTConvexGeometry::CookToFile(desc, outputPath);
}

HRESULT CPhysXCookingEditor::ValidateCookedFile(const _string& outputPath) const
{
	if (m_eCookType != COOK_TYPE::CONVEX_MESH)
		return E_NOTIMPL;

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

