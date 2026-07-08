#include "pch.h"
#include "MapEditorGUI.h"
#include "GameInstance.h"
#include "MapMeshObject.h"
#include "MapEditorTerrain.h"
#include "ResMapEditorTerrainVIBuffer.h"
#include <cfloat>
#include <filesystem>
NS_USING(Client)

namespace
{
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

		return std::string(E::MAP_SAVE_ROOT) + cleanName + "/";
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
			0.0f,
			0.0f,
			clientSize.x,
			clientSize.y,
			0.0f,
			1.0f,
			proj,
			view,
			world);

		E::_vector farPoint = XMVector3Unproject(
			XMVectorSet(mouse.x, mouse.y, 1.0f, 1.0f),
			0.0f,
			0.0f,
			clientSize.x,
			clientSize.y,
			0.0f,
			1.0f,
			proj,
			view,
			world);

		E::_vector dir = XMVector3Normalize(farPoint - nearPoint);

		XMStoreFloat3(&outOrigin, nearPoint);
		XMStoreFloat3(&outDir, dir);
		return true;
	}

	bool PickTerrainTriangle(const CMapEditorTerrain& terrain, uint32_t& outTriangleIndex, E::_float3& outHitPos)
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

	E::_float3 GetTriangleCenter(const CMapEditorTerrain& terrain, uint32_t triangleIndex)
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

	void PaintTerrainTriangles(CMapEditorTerrain& terrain, E::CNavMeshManager& navMeshManager, const E::_float3& hitPos, float radius, E::ENavAreaType areaType)
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

	CMapEditorTerrain* FindFirstMapEditorTerrain()
	{
		const auto& layers = E::CGameInstance::Get().GetGameObjectLayers();
		for (const auto& [layerName, layer] : layers)
		{
			for (const auto& handle : layer)
			{
				if (auto* terrain = E::CGameInstance::Get().GetGameObjectByHandleT<CMapEditorTerrain>(handle))
				{
					return terrain;
				}
			}
		}

		return nullptr;
	}

	bool BuildNavMeshFromTerrain(CMapEditorTerrain& terrain, E::CNavMeshManager& navMeshManager, const E::NAVMESH_BUILD_DESC& navDesc)
	{
		const auto& srcVertices = terrain.GetVertices();
		const auto& srcIndices = terrain.GetIndices();

		std::vector<E::_float3> navVertices{};
		navVertices.reserve(srcVertices.size());

		for (const auto& vertex : srcVertices)
		{
			navVertices.push_back(vertex.pos);
		}

		return navMeshManager.Build(navVertices, srcIndices, navDesc);
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

	static E::NAVMESH_BUILD_DESC navDesc{};
	static bool buildTried = false;
	static bool buildSucceeded = false;

	if (ImGui::Button("Level Save", ImVec2(112.f, 0.f)))
	{
		const std::string mapPath = MakeMapPath(m_MapName);
		CGameInstance::Get().SaveMap(mapPath);
		if (auto* navMeshManager = CGameInstance::Get().GetNavMeshManager())
		{
			navMeshManager->Save((std::filesystem::path(mapPath) / "navmesh.json").generic_string());
		}
		ImGui::OpenPopup("SaveCheck");
	}
	ImGui::SameLine();
	if (ImGui::Button("Level Load", ImVec2(112.f, 0.f)))
	{
		const std::string mapPath = MakeMapPath(m_MapName);
		CGameInstance::Get().LoadMap(mapPath, true);
		if (auto* navMeshManager = CGameInstance::Get().GetNavMeshManager())
		{
			if (SUCCEEDED(navMeshManager->Load((std::filesystem::path(mapPath) / "navmesh.json").generic_string())))
			{
				if (auto* terrain = FindFirstMapEditorTerrain())
				{
					buildTried = true;
					buildSucceeded = BuildNavMeshFromTerrain(*terrain, *navMeshManager, navDesc);
				}
			}
		}
		//AddDefaultCameraLight();
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
	bool bMapMeshInstancing = E::CMapMeshObject::IsInstancingEnabled();
	if (ImGui::Checkbox("MapMesh Instancing", &bMapMeshInstancing))
	{
		E::CMapMeshObject::SetInstancingEnabled(bMapMeshInstancing);
	}
	const auto& instancingStats = E::CMapMeshObject::GetInstancingStats();
	ImGui::Text("Mode: %s", instancingStats.bEnabled ? "Instanced" : "Normal");
	ImGui::Text("Objects: %u", instancingStats.iObjects);
	ImGui::Text("Batches: %u", instancingStats.iBatches);
	ImGui::Text("Instances: %u", instancingStats.iInstances);
	ImGui::Text("DrawCalls: %u", instancingStats.iDrawCalls);


	// ---------------------------------------NavMeshBuild--------------------------------------
	CHandle* phandle = GetSelectedHandle();
	if (phandle != nullptr)
	{
		CMapEditorTerrain* pTerrain = CGameInstance::Get().GetGameObjectByHandleT<CMapEditorTerrain>(*phandle);
		if (pTerrain != nullptr)
		{
			ImGui::Separator();
			ImGui::TextDisabled("NavMesh");

			static bool debugDrawNavMesh = true;
			static int editTriangleIndex = 0;
			static int paintMode = 0;
			static bool paintWithMouse = false;
			static float brushRadius = 1.0f;
			static bool pickSucceeded = false;
			static uint32_t pickedTriangleIndex = 0;

			ImGui::SliderFloat("Agent Height", &navDesc.agentHeight, 0.5f, 5.0f);
			ImGui::SliderFloat("Agent Radius", &navDesc.agentRadius, 0.1f, 2.0f);
			ImGui::SliderFloat("Agent Climb", &navDesc.agentMaxClimb, 0.1f, 2.0f);
			ImGui::SliderFloat("Agent Slope", &navDesc.agentMaxSlope, 0.0f, 60.0f);
			ImGui::SliderFloat("Cell Size", &navDesc.cellSize, 0.05f, 1.0f);
			ImGui::SliderFloat("Cell Height", &navDesc.cellHeight, 0.05f, 1.0f);
			ImGui::SliderFloat("Max Edge Len", &navDesc.maxEdgeLen, 1.0f, 64.0f);
			ImGui::SliderFloat("Max Edge Error", &navDesc.maxSimplificationError, 0.1f, 5.0f);
			ImGui::SliderInt("Min Region Area", &navDesc.minRegionArea, 0, 128);
			ImGui::SliderInt("Merge Region Area", &navDesc.mergeRegionArea, 0, 256);
			ImGui::SliderInt("Verts Per Poly", &navDesc.maxVertsPerPoly, 3, 12);
			ImGui::SliderFloat("Detail Sample Dist", &navDesc.detailSampleDist, 0.0f, 16.0f);
			ImGui::SliderFloat("Detail Max Error", &navDesc.detailSampleMaxError, 0.0f, 8.0f);

			auto* navMeshManager = CGameInstance::Get().GetNavMeshManager();
			if (navMeshManager != nullptr)
			{
				if (ImGui::Checkbox("Debug Draw", &debugDrawNavMesh))
				{
					navMeshManager->SetDebugDraw(debugDrawNavMesh);
				}

				if (ImGui::Button("Build NavMesh", ImVec2(140.f, 0.f)))
				{
					buildTried = true;
					buildSucceeded = BuildNavMeshFromTerrain(*pTerrain, *navMeshManager, navDesc);
				}

				ImGui::SameLine();
				if (ImGui::Button("Clear NavMesh", ImVec2(120.f, 0.f)))
				{
					navMeshManager->Clear();
					buildTried = false;
					buildSucceeded = false;
				}

				if (buildTried)
				{
					ImGui::Text("Build: %s", buildSucceeded ? "Success" : "Failed");
				}
				ImGui::Text("Built: %s", navMeshManager->IsBuilt() ? "Yes" : "No");

				const uint32_t triangleCount = static_cast<uint32_t>(pTerrain->GetIndices().size() / 3);
				ImGui::Separator();
				ImGui::Text("Blocked Triangles: %u", navMeshManager->GetBlockedTriangleCount());
				ImGui::Text("Terrain Triangles: %u", triangleCount);

				ImGui::Checkbox("Mouse Paint", &paintWithMouse);
				ImGui::SameLine();
				ImGui::RadioButton("Blocked", &paintMode, 0);
				ImGui::SameLine();
				ImGui::RadioButton("Walkable", &paintMode, 1);
				ImGui::SliderFloat("Brush Radius", &brushRadius, 0.1f, 20.0f);

				if (paintWithMouse)
				{
					ImGui::TextDisabled("Paint: hold LMB on terrain. Rebuild NavMesh after painting.");
					const E::ENavAreaType paintAreaType = (paintMode == 0) ? E::ENavAreaType::Blocked : E::ENavAreaType::Walkable;

					const ImGuiIO& io = ImGui::GetIO();
					if (!io.WantCaptureMouse && ImGui::IsMouseDown(ImGuiMouseButton_Left))
					{
						E::_float3 hitPos{};
						if (PickTerrainTriangle(*pTerrain, pickedTriangleIndex, hitPos))
						{
							pickSucceeded = true;
							editTriangleIndex = static_cast<int>(pickedTriangleIndex);
							PaintTerrainTriangles(*pTerrain, *navMeshManager, hitPos, brushRadius, paintAreaType);
						}
					}

					if (pickSucceeded)
					{
						ImGui::Text("Picked Triangle: %u", pickedTriangleIndex);
					}
					else
					{
						ImGui::Text("Picked Triangle: none");
					}
				}

				ImGui::InputInt("Triangle Index", &editTriangleIndex);
				if (editTriangleIndex < 0)
				{
					editTriangleIndex = 0;
				}
				if (triangleCount > 0 && static_cast<uint32_t>(editTriangleIndex) >= triangleCount)
				{
					editTriangleIndex = static_cast<int>(triangleCount - 1);
				}

				const uint32_t triangleIndex = static_cast<uint32_t>(editTriangleIndex);
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
			}
		}
	}
	// ---------------------------------------NavMeshBuild--------------------------------------

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

//void CMapEditorGUI::AddDefaultCameraLight()
//{
//	{
//		E::CCameraObject::CAMERA_DESC Desc{};
//		Desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
//		Desc.vAt = { 0.f, 0.f, 0.f };
//		Desc.vEye = { 0.f, 0.f, -5.f };
//		Desc.fAspect = { g_iWinSizeX / (E::_float)g_iWinSizeY };
//		Desc.fFovY = 75.f;
//		Desc.fNear = 0.1f;
//		Desc.fFar = 100.f;
//		Desc.sObjectTag = "FlyCam";
//
//		if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_FlyCamera",
//			"99_CAMERA", &Desc))
//		{
//			if (FAILED(E::CGameInstance::Get().RegistCamera("FLY", flyCam.value())))
//			{
//				MSG_BOX("MSG_BOX_123");
//			}
//			E::CGameInstance::Get().SetActiveCamera("FLY");
//		}
//	}
//
//	{
//		E::CCameraObject::CAMERA_DESC Desc{};
//		Desc.eProj = E::CCameraObject::PROJ::ORTHOGRAPHIC;
//		Desc.fNear = 0.f;
//		Desc.fFar = 1.f;
//		Desc.fWidth = g_iWinSizeX;
//		Desc.fHeight = g_iWinSizeY;
//		Desc.sObjectTag = "UICam";
//		Desc.vEye = { 0.f, 0.f, -0.1f };
//
//		if (auto uiCam = E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_UICamera",
//			"99_CAMERA", &Desc))
//		{
//			if (FAILED(E::CGameInstance::Get().RegistCamera("UI", uiCam.value())))
//			{
//				MSG_BOX("MSG_BOX_123_");
//			}
//			//E::CGameInstance::Get().SetActiveUICamera("UI");
//		}
//	}
//
//	if (FAILED(E::CGameInstance::Get().AddPrototype("LIGHT", "Prototype_GameObject_Light", CLight::Create())))
//	{
//		MSG_BOX("MSG_BOX_123_");		// 월드에 전역조명 추가
//	}
//	CGameInstance::Get().Add_DirectionalLight({ 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, 10.f);
//}
