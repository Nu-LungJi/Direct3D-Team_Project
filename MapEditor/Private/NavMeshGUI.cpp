#include "pch.h"
#include "NavMeshGUI.h"
#include "GameInstance.h"
#include "Terrain.h"

#include <cfloat>
#include <filesystem>

NS_USING(Client)

namespace
{
	bool IntersectRayTriangle(
		E::_fvector rayOrigin,
		E::_fvector rayDir,
		E::_fvector v0,
		E::_fvector v1,
		E::_fvector v2,
		float& distance)
	{
		constexpr float epsilon = 1e-6f;

		const E::_vector edge1 = v1 - v0;
		const E::_vector edge2 = v2 - v0;
		const E::_vector pvec = XMVector3Cross(rayDir, edge2);
		const float det = XMVectorGetX(XMVector3Dot(edge1, pvec));

		if (fabsf(det) < epsilon)
		{
			return false;
		}

		const float invDet = 1.0f / det;
		const E::_vector tvec = rayOrigin - v0;
		const float u = XMVectorGetX(XMVector3Dot(tvec, pvec)) * invDet;

		if (u < 0.0f || u > 1.0f)
		{
			return false;
		}

		const E::_vector qvec = XMVector3Cross(tvec, edge1);
		const float v = XMVectorGetX(XMVector3Dot(rayDir, qvec)) * invDet;

		if (v < 0.0f || u + v > 1.0f)
		{
			return false;
		}

		distance = XMVectorGetX(XMVector3Dot(edge2, qvec)) * invDet;
		return distance >= 0.0f;
	}

	bool MakeMouseRay(E::_float3& outOrigin, E::_float3& outDir)
	{
		auto* camera = E::CGameInstance::Get().GetActiveCamera();
		if (!camera)
		{
			return false;
		}

		const E::_float2 mouse = E::CGameInstance::Get().GetMousePos();
		const E::_float2 clientSize = E::CGameInstance::Get().GetClientScreenSize();
		if (clientSize.x <= 0.0f || clientSize.y <= 0.0f)
		{
			return false;
		}

		const E::_matrix view = camera->GetView();
		const E::_matrix proj = camera->GetProj();
		const E::_matrix world = XMMatrixIdentity();

		E::_vector nearPoint = XMVector3Unproject(
			XMVectorSet(mouse.x, mouse.y, 0.0f, 1.0f),
			0.0f, 0.0f,
			clientSize.x, clientSize.y,
			0.0f, 1.0f,
			proj, view, world);

		E::_vector farPoint = XMVector3Unproject(
			XMVectorSet(mouse.x, mouse.y, 1.0f, 1.0f),
			0.0f, 0.0f,
			clientSize.x, clientSize.y,
			0.0f, 1.0f,
			proj, view, world);

		E::_vector dir = XMVector3Normalize(farPoint - nearPoint);

		XMStoreFloat3(&outOrigin, nearPoint);
		XMStoreFloat3(&outDir, dir);
		return true;
	}

	bool PickTerrainTriangle(const E::CTerrain& terrain, uint32_t& outTriangleIndex, E::_float3& outHitPos)
	{
		E::_float3 rayOrigin{};
		E::_float3 rayDir{};
		if (!MakeMouseRay(rayOrigin, rayDir))
		{
			return false;
		}

		const auto& vertices = terrain.GetVertices();
		const auto& indices = terrain.GetIndices();
		const uint32_t triangleCount = static_cast<uint32_t>(indices.size() / 3);
		if (vertices.empty() || triangleCount == 0)
		{
			return false;
		}

		float nearestDistance = FLT_MAX;
		bool found = false;

		const E::_vector origin = XMLoadFloat3(&rayOrigin);
		const E::_vector dir = XMLoadFloat3(&rayDir);

		for (uint32_t tri = 0; tri < triangleCount; ++tri)
		{
			const uint32_t i0 = indices[tri * 3 + 0];
			const uint32_t i1 = indices[tri * 3 + 1];
			const uint32_t i2 = indices[tri * 3 + 2];
			if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size())
			{
				continue;
			}

			float distance = 0.0f;
			if (IntersectRayTriangle(
				origin,
				dir,
				XMLoadFloat3(&vertices[i0].pos),
				XMLoadFloat3(&vertices[i1].pos),
				XMLoadFloat3(&vertices[i2].pos),
				distance) &&
				distance < nearestDistance)
			{
				nearestDistance = distance;
				outTriangleIndex = tri;
				found = true;
			}
		}

		if (!found)
		{
			return false;
		}

		XMStoreFloat3(&outHitPos, origin + dir * nearestDistance);
		return true;
	}

	E::_float3 GetTriangleCenter(const E::CTerrain& terrain, uint32_t triangleIndex)
	{
		const auto& vertices = terrain.GetVertices();
		const auto& indices = terrain.GetIndices();

		const uint32_t i0 = indices[triangleIndex * 3 + 0];
		const uint32_t i1 = indices[triangleIndex * 3 + 1];
		const uint32_t i2 = indices[triangleIndex * 3 + 2];

		E::_float3 center{};
		XMStoreFloat3(
			&center,
			(XMLoadFloat3(&vertices[i0].pos) +
				XMLoadFloat3(&vertices[i1].pos) +
				XMLoadFloat3(&vertices[i2].pos)) / 3.0f);
		return center;
	}

	void PaintTerrainTriangles(E::CTerrain& terrain, E::CNavMeshManager& navMeshManager, const E::_float3& hitPos, float radius, E::ENavAreaType areaType)
	{
		const auto& indices = terrain.GetIndices();
		const uint32_t triangleCount = static_cast<uint32_t>(indices.size() / 3);
		const float radiusSq = radius * radius;

		for (uint32_t tri = 0; tri < triangleCount; ++tri)
		{
			const E::_float3 center = GetTriangleCenter(terrain, tri);
			const float dx = center.x - hitPos.x;
			const float dz = center.z - hitPos.z;

			if ((dx * dx + dz * dz) <= radiusSq)
			{
				navMeshManager.SetTriangleArea(tri, areaType);
			}
		}
	}
}

CNavMeshGUI::CNavMeshGUI()
{
}

CNavMeshGUI::~CNavMeshGUI()
{
}

void CNavMeshGUI::SaveNavMesh(const std::string& mapPath)
{
	if (auto* navMeshManager = E::CGameInstance::Get().GetNavMeshManager())
	{
		navMeshManager->Save((std::filesystem::path(mapPath) / "navmesh.json").generic_string());
	}
}

void CNavMeshGUI::LoadNavMesh(const std::string& mapPath)
{
	auto* navMeshManager = E::CGameInstance::Get().GetNavMeshManager();
	if (!navMeshManager)
	{
		return;
	}

	if (FAILED(navMeshManager->Load((std::filesystem::path(mapPath) / "navmesh.json").generic_string())))
	{
		return;
	}

	if (auto* terrain = FindFirstTerrain())
	{
		m_bBuildTried = true;
		m_bBuildSucceeded = BuildNavMeshFromTerrain(*terrain, *navMeshManager);
	}
}

void CNavMeshGUI::UpdateGUI(E::_float fTimeDelta)
{
	CHandle* phandle = GetSelectedHandle();
	if (phandle == nullptr)
	{
		return;
	}

	E::CTerrain* pTerrain = E::CGameInstance::Get().GetGameObjectByHandleT<E::CTerrain>(*phandle);
	if (pTerrain == nullptr)
	{
		return;
	}

	ImGui::Separator();
	ImGui::TextDisabled("NavMesh");

	ImGui::SliderFloat("Agent Height", &m_NavDesc.agentHeight, 0.5f, 5.0f);
	ImGui::SliderFloat("Agent Radius", &m_NavDesc.agentRadius, 0.1f, 2.0f);
	ImGui::SliderFloat("Agent Climb", &m_NavDesc.agentMaxClimb, 0.1f, 2.0f);
	ImGui::SliderFloat("Agent Slope", &m_NavDesc.agentMaxSlope, 0.0f, 60.0f);
	ImGui::SliderFloat("Cell Size", &m_NavDesc.cellSize, 0.05f, 1.0f);
	ImGui::SliderFloat("Cell Height", &m_NavDesc.cellHeight, 0.05f, 1.0f);
	ImGui::SliderFloat("Max Edge Len", &m_NavDesc.maxEdgeLen, 1.0f, 64.0f);
	ImGui::SliderFloat("Max Edge Error", &m_NavDesc.maxSimplificationError, 0.1f, 5.0f);
	ImGui::SliderInt("Min Region Area", &m_NavDesc.minRegionArea, 0, 128);
	ImGui::SliderInt("Merge Region Area", &m_NavDesc.mergeRegionArea, 0, 256);
	ImGui::SliderInt("Verts Per Poly", &m_NavDesc.maxVertsPerPoly, 3, 12);
	ImGui::SliderFloat("Detail Sample Dist", &m_NavDesc.detailSampleDist, 0.0f, 16.0f);
	ImGui::SliderFloat("Detail Max Error", &m_NavDesc.detailSampleMaxError, 0.0f, 8.0f);

	auto* navMeshManager = E::CGameInstance::Get().GetNavMeshManager();
	if (navMeshManager == nullptr)
	{
		return;
	}

	if (ImGui::Checkbox("Debug Draw", &m_bDebugDrawNavMesh))
	{
		navMeshManager->SetDebugDraw(m_bDebugDrawNavMesh);
	}

	if (ImGui::Button("Build NavMesh", ImVec2(140.f, 0.f)))
	{
		m_bBuildTried = true;
		m_bBuildSucceeded = BuildNavMeshFromTerrain(*pTerrain, *navMeshManager);
	}

	ImGui::SameLine();
	if (ImGui::Button("Clear NavMesh", ImVec2(120.f, 0.f)))
	{
		navMeshManager->Clear();
		m_bBuildTried = false;
		m_bBuildSucceeded = false;
	}

	if (m_bBuildTried)
	{
		ImGui::Text("Build: %s", m_bBuildSucceeded ? "Success" : "Failed");
	}
	ImGui::Text("Built: %s", navMeshManager->IsBuilt() ? "Yes" : "No");

	const uint32_t triangleCount = static_cast<uint32_t>(pTerrain->GetIndices().size() / 3);
	ImGui::Separator();
	ImGui::Text("Blocked Triangles: %u", navMeshManager->GetBlockedTriangleCount());
	ImGui::Text("Terrain Triangles: %u", triangleCount);

	ImGui::Checkbox("Mouse Paint", &m_bPaintWithMouse);
	ImGui::SameLine();
	ImGui::RadioButton("Blocked", &m_iPaintMode, 0);
	ImGui::SameLine();
	ImGui::RadioButton("Walkable", &m_iPaintMode, 1);
	ImGui::SliderFloat("Brush Radius", &m_fBrushRadius, 0.1f, 20.0f);

	if (m_bPaintWithMouse && !m_bPathPickWithMouse)
	{
		ImGui::TextDisabled("Paint: hold LMB on terrain. Rebuild NavMesh after painting.");
		const E::ENavAreaType paintAreaType = (m_iPaintMode == 0) ? E::ENavAreaType::Blocked : E::ENavAreaType::Walkable;

		const ImGuiIO& io = ImGui::GetIO();
		if (!io.WantCaptureMouse && ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			E::_float3 hitPos{};
			if (PickTerrainTriangle(*pTerrain, m_iPickedTriangleIndex, hitPos))
			{
				m_bPickSucceeded = true;
				m_iEditTriangleIndex = static_cast<int>(m_iPickedTriangleIndex);
				PaintTerrainTriangles(*pTerrain, *navMeshManager, hitPos, m_fBrushRadius, paintAreaType);
			}
		}

		if (m_bPickSucceeded)
		{
			ImGui::Text("Picked Triangle: %u", m_iPickedTriangleIndex);
		}
		else
		{
			ImGui::Text("Picked Triangle: none");
		}
	}

	ImGui::InputInt("Triangle Index", &m_iEditTriangleIndex);
	if (m_iEditTriangleIndex < 0)
	{
		m_iEditTriangleIndex = 0;
	}
	if (triangleCount > 0 && static_cast<uint32_t>(m_iEditTriangleIndex) >= triangleCount)
	{
		m_iEditTriangleIndex = static_cast<int>(triangleCount - 1);
	}

	const uint32_t triangleIndex = static_cast<uint32_t>(m_iEditTriangleIndex);
	const bool isBlocked = navMeshManager->IsTriangleBlocked(triangleIndex);
	ImGui::Text("Selected: %s", isBlocked ? "Blocked" : "Walkable");

	if (ImGui::Button("Set Blocked", ImVec2(110.f, 0.f)))
	{
		navMeshManager->SetTriangleBlocked(triangleIndex, true);
	}
	ImGui::SameLine();
	if (ImGui::Button("Set Walkable", ImVec2(110.f, 0.f)))
	{
		navMeshManager->SetTriangleBlocked(triangleIndex, false);
	}

	if (ImGui::Button("Clear Blocked Triangles", ImVec2(180.f, 0.f)))
	{
		navMeshManager->ClearBlockedTriangles();
	}

	ImGui::Separator();
	ImGui::TextDisabled("Path Test");
	ImGui::Checkbox("Path Pick", &m_bPathPickWithMouse);
	ImGui::SameLine();
	ImGui::RadioButton("Start", &m_iPathPickTarget, 0);
	ImGui::SameLine();
	ImGui::RadioButton("End", &m_iPathPickTarget, 1);

	if (m_bPathPickWithMouse)
	{
		ImGui::TextDisabled("Path: hold LMB on terrain to set point.");

		const ImGuiIO& io = ImGui::GetIO();
		if (!io.WantCaptureMouse && ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			E::_float3 hitPos{};
			uint32_t hitTriangleIndex = 0;
			if (PickTerrainTriangle(*pTerrain, hitTriangleIndex, hitPos))
			{
				if (m_iPathPickTarget == 0)
				{
					navMeshManager->SetPathTestStart(hitPos);
				}
				else
				{
					navMeshManager->SetPathTestEnd(hitPos);
				}

				m_bPathFindTried = false;
				m_bPathFindSucceeded = false;
			}
		}
	}

	ImGui::Text("Start: %s", navMeshManager->HasPathTestStart() ? "Set" : "None");
	ImGui::SameLine();
	ImGui::Text("End: %s", navMeshManager->HasPathTestEnd() ? "Set" : "None");

	if (ImGui::Button("Find Path", ImVec2(110.f, 0.f)))
	{
		m_bPathFindTried = true;
		m_bPathFindSucceeded = navMeshManager->BuildPathTest();
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear Path", ImVec2(110.f, 0.f)))
	{
		navMeshManager->ClearPathTest();
		m_bPathFindTried = false;
		m_bPathFindSucceeded = false;
	}

	if (m_bPathFindTried)
	{
		ImGui::Text("Path: %s (%u points)", m_bPathFindSucceeded ? "Success" : "Failed", navMeshManager->GetPathTestPointCount());
	}
}

bool CNavMeshGUI::BuildNavMeshFromTerrain(E::CTerrain& terrain, E::CNavMeshManager& navMeshManager)
{
	const auto& srcVertices = terrain.GetVertices();
	const auto& srcIndices = terrain.GetIndices();

	std::vector<E::_float3> navVertices{};
	navVertices.reserve(srcVertices.size());

	for (const auto& vertex : srcVertices)
	{
		navVertices.push_back(vertex.pos);
	}

	return navMeshManager.Build(navVertices, srcIndices, m_NavDesc);
}

E::CTerrain* CNavMeshGUI::FindFirstTerrain()
{
	const auto& layers = E::CGameInstance::Get().GetGameObjectLayers();
	for (const auto& [layerName, layer] : layers)
	{
		for (const auto& handle : layer)
		{
			if (auto* terrain = E::CGameInstance::Get().GetGameObjectByHandleT<E::CTerrain>(handle))
			{
				return terrain;
			}
		}
	}

	return nullptr;
}

E::UPtr<CNavMeshGUI> CNavMeshGUI::Create(E::CHandle* pSelectedObject)
{
	auto pInstance = E::UPtr<CNavMeshGUI>(new CNavMeshGUI{});
	if (FAILED(pInstance->Initialize(pSelectedObject)))
	{
		MSG_BOX("Failed to Created : CNavMeshGUI");
		return nullptr;
	}

	return pInstance;
}
