#include "pch.h"
#include "NavMeshGUI.h"
#include "GameInstance.h"
#include "Terrain.h"
#include "MapNaviPosPick.h"
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

	_bool PickTerrainTriangle(const E::CTerrain& terrain, uint32_t& outTriangleIndex, E::_float3& outHitPos)
	{
		_float3 vRayOrigin{};
		_float3 vRayDirection{};

		if (!MakeMouseRay(vRayOrigin, vRayDirection))
			return false;

		const auto& vertices = terrain.GetVertices();
		const auto& indices = terrain.GetIndices();

		const uint32_t iTriangleCount = static_cast<uint32_t>(indices.size() / 3);

		if (vertices.empty() || iTriangleCount == 0)
			return false;

		_matrix TerrainWorld = terrain.GetTransform().GetLoadedCombinedWorldMatrix();
		_matrix TerrainWorldInverse = XMMatrixInverse(nullptr, TerrainWorld);
		_vector vLocalOrigin = XMVector3TransformCoord(XMLoadFloat3(&vRayOrigin), TerrainWorldInverse);

		_vector vLocalDirection = XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3(&vRayDirection),
			TerrainWorldInverse));

		_float fNearestDistance = FLT_MAX;
		_bool bFound = false;

		for (uint32_t iTriangle = 0; iTriangle < iTriangleCount; ++iTriangle)
		{
			uint32_t iIndex0 = indices[iTriangle * 3 + 0];
			uint32_t iIndex1 = indices[iTriangle * 3 + 1];
			uint32_t iIndex2 = indices[iTriangle * 3 + 2];

			if (iIndex0 >= vertices.size() || iIndex1 >= vertices.size() ||
				iIndex2 >= vertices.size())
				continue;

			float fDistance = 0.f;

			if (IntersectRayTriangle(vLocalOrigin, vLocalDirection,
				XMLoadFloat3(&vertices[iIndex0].pos),
				XMLoadFloat3(&vertices[iIndex1].pos),
				XMLoadFloat3(&vertices[iIndex2].pos), fDistance) && fDistance < fNearestDistance)
			{
				fNearestDistance = fDistance;
				outTriangleIndex = iTriangle;
				bFound = true;
			}
		}

		if (!bFound)
			return false;

		_vector vLocalHitPosition = vLocalOrigin + vLocalDirection * fNearestDistance;
		XMStoreFloat3(&outHitPos, XMVector3TransformCoord(vLocalHitPosition, TerrainWorld));
		return true;
	}
	_bool PickManualSurfacePoint(
		const E::CTerrain& Terrain,
		CMapNaviPosPickPass* pMapPickPass,
		E::_float3& vOutPosition)
	{
		const E::_float2 vMousePosition =
			E::CGameInstance::Get().GetMousePos();

		if (pMapPickPass &&
			vMousePosition.x >= 0.f &&
			vMousePosition.y >= 0.f)
		{
			const auto MapMeshHit = pMapPickPass->Pick(
				static_cast<uint32_t>(vMousePosition.x),
				static_cast<uint32_t>(vMousePosition.y));

			if (MapMeshHit)
			{
				vOutPosition = MapMeshHit.value();
				return true;
			}
		}

		uint32_t iTriangleIndex{};
		return PickTerrainTriangle(
			Terrain,
			iTriangleIndex,
			vOutPosition);
	}

	_bool PickManualTriangle(
		const std::vector<E::NAVMESH_MANUAL_TRIANGLE>& Triangles,
		uint32_t& iOutTriangleIndex)
	{
		E::_float3 vRayOrigin{};
		E::_float3 vRayDirection{};

		if (!MakeMouseRay(vRayOrigin, vRayDirection))
			return false;

		const E::_vector vOrigin = XMLoadFloat3(&vRayOrigin);
		const E::_vector vDirection = XMLoadFloat3(&vRayDirection);
		_float fNearestDistance = FLT_MAX;
		_bool bFound = false;

		for (uint32_t i = 0; i < Triangles.size(); ++i)
		{
			const E::NAVMESH_MANUAL_TRIANGLE& Triangle = Triangles[i];
			_float fDistance{};

			if (IntersectRayTriangle(
				vOrigin,
				vDirection,
				XMLoadFloat3(&Triangle.vPoints[0]),
				XMLoadFloat3(&Triangle.vPoints[1]),
				XMLoadFloat3(&Triangle.vPoints[2]),
				fDistance) &&
				fDistance < fNearestDistance)
			{
				fNearestDistance = fDistance;
				iOutTriangleIndex = i;
				bFound = true;
			}
		}

		return bFound;
	}

	_bool IsValidManualTriangle(
		const E::NAVMESH_MANUAL_TRIANGLE& Triangle)
	{
		const E::_vector vPoint0 =
			XMLoadFloat3(&Triangle.vPoints[0]);
		const E::_vector vPoint1 =
			XMLoadFloat3(&Triangle.vPoints[1]);
		const E::_vector vPoint2 =
			XMLoadFloat3(&Triangle.vPoints[2]);

		const E::_vector vNormal = XMVector3Cross(
			vPoint1 - vPoint0,
			vPoint2 - vPoint0);

		return XMVectorGetX(
			XMVector3LengthSq(vNormal)) > FLT_EPSILON;
	}

	E::_float3 GetTriangleCenter(
		const E::CTerrain& terrain,
		uint32_t triangleIndex)
	{
		const auto& vertices = terrain.GetVertices();
		const auto& indices = terrain.GetIndices();

		const uint32_t i0 = indices[triangleIndex * 3 + 0];
		const uint32_t i1 = indices[triangleIndex * 3 + 1];
		const uint32_t i2 = indices[triangleIndex * 3 + 2];

		const _vector vLocalCenter =
			(XMLoadFloat3(&vertices[i0].pos) +
			 XMLoadFloat3(&vertices[i1].pos) +
			 XMLoadFloat3(&vertices[i2].pos)) / 3.f;

		const _matrix TerrainWorld =
			terrain.GetTransform().GetLoadedCombinedWorldMatrix();

		_float3 vWorldCenter{};
		XMStoreFloat3(
			&vWorldCenter,
			XMVector3TransformCoord(vLocalCenter, TerrainWorld));

		return vWorldCenter;
	}

	void PaintTerrainTriangles(
		E::CTerrain& terrain,
		E::CNavMeshManager& navMeshManager,
		const E::_float3& hitPos,
		float radius,
		E::ENavAreaType areaType)
	{
		const auto& indices = terrain.GetIndices();
		const uint32_t triangleCount =
			static_cast<uint32_t>(indices.size() / 3);
		const float radiusSq = radius * radius;

		for (uint32_t tri = 0; tri < triangleCount; ++tri)
		{
			const E::_float3 center =
				GetTriangleCenter(terrain, tri);
			const float dx = center.x - hitPos.x;
			const float dz = center.z - hitPos.z;

			if ((dx * dx + dz * dz) <= radiusSq)
				navMeshManager.SetTriangleArea(tri, areaType);
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
		navMeshManager->Save(
			(std::filesystem::path(mapPath) / "navmesh.json").generic_string(),
			&m_NavDesc);
	}
}

void CNavMeshGUI::LoadNavMesh(const std::string& mapPath)
{
	auto* navMeshManager = E::CGameInstance::Get().GetNavMeshManager();
	if (!navMeshManager)
	{
		return;
	}

	if (FAILED(navMeshManager->Load(
		(std::filesystem::path(mapPath) / "navmesh.json").generic_string(),
		&m_NavDesc)))
	{
		return;
	}
	m_iManualPickCount = 0;
	if (auto* terrain = FindFirstTerrain())
	{
		m_bBuildTried = true;

		if (!navMeshManager->GetManualTriangles().empty())
		{
			m_bBuildSucceeded =
				BuildManualNavMesh(*navMeshManager);
		}
		else
		{
			m_bBuildSucceeded =
				BuildNavMeshFromTerrain(*terrain, *navMeshManager);
		}
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
	ImGui::SliderFloat("Agent Radius", &m_NavDesc.agentRadius, 0.f, 2.0f); // 0.1f ~ 2.0f -> 0.f ~ 2.0f
	ImGui::SliderFloat("Agent Climb", &m_NavDesc.agentMaxClimb, 0.1f, 2.0f);
	ImGui::SliderFloat("Agent Slope", &m_NavDesc.agentMaxSlope, 0.0f, 60.0f);
	ImGui::SliderFloat("Cell Size", &m_NavDesc.cellSize, 0.05f, 4.0f);//0.05f ~ 1.0f -> 0.05f ~ 4.f
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

	if (ImGui::Button("Build Terrain NavMesh", ImVec2(180.f, 0.f)))
	{
		m_bBuildTried = true;
		m_bBuildSucceeded =
			BuildNavMeshFromTerrain(*pTerrain, *navMeshManager);
	}

	ImGui::SameLine();
	if (ImGui::Button("Build Manual NavMesh", ImVec2(180.f, 0.f)))
	{
		m_bBuildTried = true;
		m_bBuildSucceeded = BuildManualNavMesh(*navMeshManager);
	}

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

	const uint32_t triangleCount =
		static_cast<uint32_t>(pTerrain->GetIndices().size() / 3);

	ImGui::Separator();
	ImGui::Text(
		"Blocked Triangles: %u",
		navMeshManager->GetBlockedTriangleCount());
	ImGui::Text("Terrain Triangles: %u", triangleCount);

	ImGui::Checkbox("Mouse Paint", &m_bPaintWithMouse);
	ImGui::SameLine();
	ImGui::RadioButton("Blocked", &m_iPaintMode, 0);
	ImGui::SameLine();
	ImGui::RadioButton("Walkable", &m_iPaintMode, 1);
	ImGui::SliderFloat(
		"Brush Radius",
		&m_fBrushRadius,
		0.1f,
		20.0f);

	if (m_bPaintWithMouse &&
		!m_bPathPickWithMouse &&
		!m_bManualTrianglePickWithMouse)
	{
		ImGui::TextDisabled(
			"Paint: hold LMB on terrain. Rebuild NavMesh after painting.");

		const E::ENavAreaType paintAreaType =
			(m_iPaintMode == 0) ?
			E::ENavAreaType::Blocked :
			E::ENavAreaType::Walkable;

		const ImGuiIO& io = ImGui::GetIO();
		if (!io.WantCaptureMouse &&
			ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			E::_float3 hitPos{};
			if (PickTerrainTriangle(
				*pTerrain,
				m_iPickedTriangleIndex,
				hitPos))
			{
				m_bPickSucceeded = true;
				m_iEditTriangleIndex =
					static_cast<int>(m_iPickedTriangleIndex);

				PaintTerrainTriangles(
					*pTerrain,
					*navMeshManager,
					hitPos,
					m_fBrushRadius,
					paintAreaType);
			}
		}

		if (m_bPickSucceeded)
			ImGui::Text("Picked Triangle: %u", m_iPickedTriangleIndex);
		else
			ImGui::Text("Picked Triangle: none");
	}

	ImGui::InputInt("Triangle Index", &m_iEditTriangleIndex);
	if (m_iEditTriangleIndex < 0)
		m_iEditTriangleIndex = 0;

	if (triangleCount > 0 &&
		static_cast<uint32_t>(m_iEditTriangleIndex) >= triangleCount)
	{
		m_iEditTriangleIndex = static_cast<int>(triangleCount - 1);
	}

	const uint32_t triangleIndex =
		static_cast<uint32_t>(m_iEditTriangleIndex);
	const bool isBlocked =
		navMeshManager->IsTriangleBlocked(triangleIndex);

	ImGui::Text(
		"Selected: %s",
		isBlocked ? "Blocked" : "Walkable");

	if (ImGui::Button("Set Blocked", ImVec2(110.f, 0.f)))
		navMeshManager->SetTriangleBlocked(triangleIndex, true);

	ImGui::SameLine();
	if (ImGui::Button("Set Walkable", ImVec2(110.f, 0.f)))
		navMeshManager->SetTriangleBlocked(triangleIndex, false);

	if (ImGui::Button(
		"Clear Blocked Triangles",
		ImVec2(180.f, 0.f)))
	{
		navMeshManager->ClearBlockedTriangles();
	}

	//수동 네비용
	{

		auto pDbgLineRender = CGameInstance::Get().GetDbgLineRender();

		if (pDbgLineRender)
		{
			_float4 vPreviousColor = pDbgLineRender->GetColor();
			DBG_LINE_DEPTH_MODE ePreviousDepthMode = pDbgLineRender->GetDepthMode();
			pDbgLineRender->SetDepthTest(true);
			pDbgLineRender->SetColor({ 1.f, 0.5f, 0.f, 1.f });

			for (const E::NAVMESH_MANUAL_TRIANGLE& Triangle : navMeshManager->GetManualTriangles())
			{
				_float3 vPoint0 = Triangle.vPoints[0];
				_float3 vPoint1 = Triangle.vPoints[1];
				_float3 vPoint2 = Triangle.vPoints[2];

				vPoint0.y += 0.25f;
				vPoint1.y += 0.25f;
				vPoint2.y += 0.25f;

				pDbgLineRender->AddLine(vPoint0, vPoint1);
				pDbgLineRender->AddLine(vPoint1, vPoint2);
				pDbgLineRender->AddLine(vPoint2, vPoint0);
			}

			if (m_iManualPickCount > 0)
			{
				pDbgLineRender->SetColor({ 1.f, 1.f, 0.f, 1.f });

				for (uint32_t i = 0; i < m_iManualPickCount; ++i)
				{
					_float3 vPickPoint = m_vManualPickPoints[i];
					vPickPoint.y += 0.3f;
					pDbgLineRender->AddSphere(
						0.25f,
						XMMatrixTranslation(
							vPickPoint.x,
							vPickPoint.y,
							vPickPoint.z));
				}

				if (m_iManualPickCount >= 2)
				{
					_float3 vPoint0 = m_vManualPickPoints[0];
					_float3 vPoint1 = m_vManualPickPoints[1];
					vPoint0.y += 0.3f;
					vPoint1.y += 0.3f;
					pDbgLineRender->AddLine(vPoint0, vPoint1);
				}
			}
			pDbgLineRender->SetColor(vPreviousColor);
			pDbgLineRender->SetDepthMode(ePreviousDepthMode);
		}

		ImGui::Separator();
		ImGui::TextDisabled("Manual Triangle");

		if (ImGui::Checkbox("Manual Triangle Pick", &m_bManualTrianglePickWithMouse))
		{
			if (m_bManualTrianglePickWithMouse)
			{
				// 다른 마우스 편집 기능과 동시에 실행하지 않는다.
				m_bPathPickWithMouse = false;
				m_bPaintWithMouse = false;
				m_bManualTriangleDeleteWithMouse = false;
			}
		}

		ImGui::SameLine();
		if (ImGui::Checkbox("Delete Manual Triangle", &m_bManualTriangleDeleteWithMouse))
		{
			if (m_bManualTriangleDeleteWithMouse)
			{
				m_bPathPickWithMouse = false;
				m_bPaintWithMouse = false;
				m_bManualTrianglePickWithMouse = false;
				m_iManualPickCount = 0;
			}
		}

		ImGui::Text("Manual Triangle Count: %u", static_cast<uint32_t>(navMeshManager->GetManualTriangles().size()));
		ImGui::DragFloat(
			"Vertex Snap Distance",
			&m_fManualVertexSnapDistance,
			0.1f,
			0.1f,
			50.f);

		if (m_iManualPickCount == 0)
			ImGui::TextDisabled("Pick 1/3: choose the first point.");
		else if (m_iManualPickCount == 1)
			ImGui::TextDisabled("Pick 2/3: choose the second point.");
		else
			ImGui::TextDisabled("Pick 3/3: choose the final point.");

		if (m_bManualTrianglePickWithMouse)
		{
			const ImGuiIO& io = ImGui::GetIO();

			if (!io.WantCaptureMouse &&
				!ImGuizmo::IsOver() &&
				!ImGuizmo::IsUsing() &&
				ImGui::IsMouseClicked(
					ImGuiMouseButton_Left))
			{
				E::_float3 vPickedPosition{};
				if (PickManualSurfacePoint(
					*pTerrain,
					m_pMapNaviPosPickPass.get(),
					vPickedPosition))
				{
					E::_float3 vResolvedPoint = vPickedPosition;
					navMeshManager->FindNearestManualVertex(
						vPickedPosition,
						m_fManualVertexSnapDistance,
						vResolvedPoint);

					_bool bDuplicatePoint = false;
					for (uint32_t i = 0; i < m_iManualPickCount; ++i)
					{
						const _float fDistanceSq = XMVectorGetX(
							XMVector3LengthSq(
								XMLoadFloat3(&vResolvedPoint) -
								XMLoadFloat3(&m_vManualPickPoints[i])));

						if (fDistanceSq <= FLT_EPSILON)
						{
							bDuplicatePoint = true;
							break;
						}
					}

					if (!bDuplicatePoint && m_iManualPickCount < 2)
					{
						m_vManualPickPoints[m_iManualPickCount] = vResolvedPoint;
						++m_iManualPickCount;
					}
					else if (!bDuplicatePoint && m_iManualPickCount == 2)
					{
						E::NAVMESH_MANUAL_TRIANGLE Triangle{};
						Triangle.vPoints[0] = m_vManualPickPoints[0];
						Triangle.vPoints[1] = m_vManualPickPoints[1];
						Triangle.vPoints[2] = vResolvedPoint;

						if (IsValidManualTriangle(Triangle))
						{
							navMeshManager->AddManualTriangle(Triangle);
							m_iManualPickCount = 0;
						}
					}
				}
			}
		}

		if (m_bManualTriangleDeleteWithMouse)
		{
			ImGui::TextDisabled("Delete: click a manual triangle. Rebuild after deleting.");

			const ImGuiIO& io = ImGui::GetIO();
			if (!io.WantCaptureMouse &&
				!ImGuizmo::IsOver() &&
				!ImGuizmo::IsUsing() &&
				ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				uint32_t iPickedManualTriangle{};
				if (PickManualTriangle(
					navMeshManager->GetManualTriangles(),
					iPickedManualTriangle))
				{
					navMeshManager->RemoveManualTriangle(
						iPickedManualTriangle);
				}
			}
		}

		if (ImGui::Button("Reset Manual Picks", ImVec2(160.f, 0.f)))
		{
			m_iManualPickCount = 0;
		}

		ImGui::SameLine();

		if (ImGui::Button("Undo Triangle", ImVec2(120.f, 0.f)))
		{
			if (!navMeshManager->GetManualTriangles().empty())
			{
				navMeshManager->RemoveLastManualTriangle();
				m_iManualPickCount = 0;
			}
		}

		if (ImGui::Button("Clear Manual Triangles", ImVec2(180.f, 0.f)))
		{
			if (!navMeshManager->GetManualTriangles().empty())
			{
				navMeshManager->ClearManualTriangles();
				m_iManualPickCount = 0;
			}
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
}

bool CNavMeshGUI::BuildManualNavMesh(E::CNavMeshManager& navMeshManager)
{
	return navMeshManager.BuildManual(m_NavDesc);
}

bool CNavMeshGUI::BuildNavMeshFromTerrain(
	E::CTerrain& terrain,
	E::CNavMeshManager& navMeshManager)
{
	const auto& srcVertices = terrain.GetVertices();
	const auto& srcIndices = terrain.GetIndices();

	std::vector<E::_float3> navVertices{};
	navVertices.reserve(srcVertices.size());

	const _matrix TerrainWorld =
		terrain.GetTransform().GetLoadedCombinedWorldMatrix();

	for (const auto& vertex : srcVertices)
	{
		_float3 vWorldPosition{};
		XMStoreFloat3(
			&vWorldPosition,
			XMVector3TransformCoord(
				XMLoadFloat3(&vertex.pos),
				TerrainWorld));
		navVertices.push_back(vWorldPosition);
	}

	return navMeshManager.Build(
		navVertices,
		srcIndices,
		m_NavDesc);
}

E::CTerrain* CNavMeshGUI::FindFirstTerrain()
{
	const auto& layers =
		E::CGameInstance::Get().GetGameObjectLayers();

	for (const auto& [layerName, layer] : layers)
	{
		for (const auto& handle : layer)
		{
			if (auto* terrain =
				E::CGameInstance::Get().GetGameObjectByHandleT<E::CTerrain>(handle))
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

	pInstance->m_pMapNaviPosPickPass = CMapNaviPosPickPass::Create();
	if (!pInstance->m_pMapNaviPosPickPass)
	{
		MSG_BOX("Failed to Created : CMapNaviPosPickPass");
		return nullptr;
	}
	return pInstance;
}
