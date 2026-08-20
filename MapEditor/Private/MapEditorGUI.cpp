#include "pch.h"
#include "MapEditorGUI.h"
#include "GameInstance.h"
#include "MapPickingPass.h"
#include "MapMeshObject.h"
#include "EditorCommandManager.h"
#include "DeleteMapMeshCommand.h"
#include "MapMeshCommandCommon.h"
#include "EditorSelection.h"
#include "Terrain.h"
#include <nlohmann/json.hpp>
NS_USING(Client)

namespace
{
	constexpr int MAP_EDITOR_GIZMO_ID = 0x4D4150;

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

	E::CTerrain* FindTerrain()
	{
		for (const auto& [layer, handles] : E::CGameInstance::Get().GetGameObjectLayers())
		{
			for (const auto& handle : handles)
			{
				if (auto* terrain = E::CGameInstance::Get().GetGameObjectByHandleT<E::CTerrain>(handle))
					return terrain;
			}
		}
		return nullptr;
	}

	HRESULT SaveTerrainForMap(const std::string& mapPath)
	{
		auto* terrain = FindTerrain();
		if (!terrain) return S_FALSE;
		const std::filesystem::path mapDirectory = mapPath;
		const std::filesystem::path terrainRelativePath = std::filesystem::path("terrain") / "terrain.json";
		if (FAILED(terrain->SaveTerrain((mapDirectory / terrainRelativePath).generic_string())))
			return E_FAIL;
		const std::filesystem::path mapJsonPath = mapDirectory / "map.json";
		std::ifstream input(mapJsonPath);
		if (!input) return E_FAIL;
		nlohmann::ordered_json mapJson{};
		input >> mapJson;
		input.close();
		mapJson["terrain"] = { { "file", terrainRelativePath.generic_string() } };
		std::ofstream output(mapJsonPath, std::ios::trunc);
		if (!output) return E_FAIL;
		output << mapJson.dump(4);
		return output ? S_OK : E_FAIL;
	}

	HRESULT LoadTerrainForMap(const std::string& mapPath)
	{
		const std::filesystem::path mapDirectory = mapPath;
		std::ifstream input(mapDirectory / "map.json");
		if (!input) return E_FAIL;
		nlohmann::ordered_json mapJson{};
		input >> mapJson;
		if (!mapJson.contains("terrain")) return S_FALSE;
		auto* terrain = FindTerrain();
		if (!terrain) return E_FAIL;
		const std::filesystem::path terrainPath = mapDirectory /
			mapJson["terrain"].at("file").get<std::string>();
		return terrain->LoadTerrain(terrainPath.generic_string());
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
	if (m_pCommandManager)
		m_pCommandManager->ProcessOne();
	if (m_pSelection)
	{
		m_pSelection->SyncFromPrimary();
		m_pSelection->PruneInvalid();
	}

	ImGui::SetNextWindowSize(ImVec2(360.f, 520.f), ImGuiCond_FirstUseEver);
	ImGui::Begin("MapEditorGUI");
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, 7.f));

	ImGui::SetNextItemWidth(236.f);
	ImGui::InputText("Map", m_MapName, sizeof(m_MapName));

	if (ImGui::Button("Level Save", ImVec2(112.f, 0.f)))
	{
		ImGui::OpenPopup("Confirm Level Save");
	}
	ImGui::SameLine();
	if (ImGui::Button("Level Load", ImVec2(112.f, 0.f)))
	{
		ImGui::OpenPopup("Confirm Level Load");
	}

	if (m_bOpenSaveComplete)
	{
		ImGui::OpenPopup("SaveCheck");
		m_bOpenSaveComplete = false;
	}
	if (m_bOpenLoadComplete)
	{
		ImGui::OpenPopup("LoadCheck");
		m_bOpenLoadComplete = false;
	}

	if (ImGui::BeginPopupModal("Confirm Level Save", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Save level '%s'?", m_MapName);
		ImGui::Separator();
		if (ImGui::Button("Yes", ImVec2(100.f, 0.f)))
		{
			const std::string mapPath = MakeMapPath(m_MapName);
			const bool mapSaved = SUCCEEDED(CGameInstance::Get().SaveMap(mapPath));
			const bool terrainSaved = mapSaved && SUCCEEDED(SaveTerrainForMap(mapPath));
			if (terrainSaved && m_pNavMeshGUI)
				m_pNavMeshGUI->SaveNavMesh(mapPath);
			m_bOpenSaveComplete = terrainSaved;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("No", ImVec2(100.f, 0.f)))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Confirm Level Load", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Load level '%s'?", m_MapName);
		ImGui::TextDisabled("Unsaved changes will be lost.");
		ImGui::Separator();
		if (ImGui::Button("Yes", ImVec2(100.f, 0.f)))
		{
			const std::string mapPath = MakeMapPath(m_MapName);
			const bool resourcesLoaded = SUCCEEDED(
				CGameInstance::Get().LoadMapResources(mapPath));
			const bool mapLoaded = resourcesLoaded &&
				SUCCEEDED(CGameInstance::Get().LoadMap(mapPath, true));
			const bool terrainLoaded = mapLoaded && SUCCEEDED(LoadTerrainForMap(mapPath));
			if (m_pCommandManager)
				m_pCommandManager->Clear();
			if (m_pSelection)
				m_pSelection->Clear();
			if (terrainLoaded && m_pNavMeshGUI)
				m_pNavMeshGUI->LoadNavMesh(mapPath);
			m_bOpenLoadComplete = terrainLoaded;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("No", ImVec2(100.f, 0.f)))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
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
	if (ImGui::Button("Undo", ImVec2(72.f, 0.f)) && m_pCommandManager)
		m_pCommandManager->RequestUndo();
	ImGui::SameLine();
	if (ImGui::Button("Redo", ImVec2(72.f, 0.f)) && m_pCommandManager)
		m_pCommandManager->RequestRedo();
	ImGui::SameLine();
	ImGui::TextDisabled("Ctrl+Z / Ctrl+Y");

	const ImGuiIO& io = ImGui::GetIO();
	if (m_pCommandManager && io.KeyCtrl && !io.WantTextInput)
	{
		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z), false))
		{
			if (io.KeyShift)
				m_pCommandManager->RequestRedo();
			else
				m_pCommandManager->RequestUndo();
		}
		else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y), false))
		{
			m_pCommandManager->RequestRedo();
		}
	}

	ImGui::Separator();
	if (m_pSelection)
		ImGui::Text("Selected Objects: %zu", m_pSelection->GetCount());
	DrawGizmoToolbar();

	ImGui::Separator();
	bool bMapMeshInstancing = E::CGameInstance::Get().IsInstancingEnabled();
	if (ImGui::Checkbox("MapMesh Instancing", &bMapMeshInstancing))
	{
		E::CGameInstance::Get().SetInstancingEnabled(bMapMeshInstancing);
	}

	bool bMapMeshDebugBounds = E::CGameInstance::Get().IsDebugBoundsEnabled();
	if (ImGui::Checkbox("MapMesh BoundingBox", &bMapMeshDebugBounds))
	{
		E::CGameInstance::Get().SetDebugBoundsEnabled(bMapMeshDebugBounds);
	}
	const auto& instancingStats = E::CGameInstance::Get().GetInstancingStats();
	ImGui::Text("Mode: %s", instancingStats.bEnabled ? "Instanced" : "Normal");
	ImGui::Text("Objects: %u", instancingStats.iObjects);
	ImGui::Text("Batches: %u", instancingStats.iBatches);
	ImGui::Text("Instances: %u", instancingStats.iInstances);
	ImGui::Text("DrawCalls: %u", instancingStats.iDrawCalls);

	ImGui::Text("----------------------------Occlusion-----------------------------------");
	ImGui::Text("Visible: %u (cpu readback)", instancingStats.iVisibleInstances);
	ImGui::Text("Culled: %u (cpu readback)", instancingStats.iCulledInstances);
	ImGui::Text("----------------------------Occlusion-----------------------------------");

	m_pTerrainGUI->UpdateGUI(fTimeDelta);
	if (!m_pTerrainGUI->IsSculptEnabled())
		m_pNavMeshGUI->UpdateGUI(fTimeDelta);

	ImGui::Separator();
	m_pHierarchy->UpdateGUI(fTimeDelta);

	ImGui::Separator();
	m_pInspector->UpdateGUI(fTimeDelta);
	DrawMapMeshContextMenu();

	ImGui::PopStyleVar(2);
	ImGui::End();

	m_pResourceGUI->UpdateGUI(fTimeDelta);
	m_pMapChunkGUI->UpdateGUI(fTimeDelta);
	if (!m_pTerrainGUI->IsSculptEnabled())
	{
		RenderGizmo();
		PickMapMeshObject();
	}
}

E::UPtr<CMapEditorGUI> CMapEditorGUI::Create(E::CHandle* pSelectedObject)
{
	auto pInstance = E::UPtr<CMapEditorGUI>(new CMapEditorGUI{});
	if (FAILED(pInstance->Initialize(pSelectedObject)))
	{
		MSG_BOX("Failed to Created : CMapEditorGUI");
		return nullptr;
	}
	pInstance->m_pCommandManager = CEditorCommandManager::Create();
	if (pInstance->m_pCommandManager == nullptr)
		return nullptr;
	pInstance->m_pSelection = std::make_unique<CEditorSelection>(pSelectedObject);

	pInstance->m_pHierarchy = CHierarchy::Create(
		pSelectedObject, pInstance->m_pCommandManager.get(), pInstance->m_pSelection.get());
	if (pInstance->m_pHierarchy == nullptr)
	{
		return nullptr;
	}

	pInstance->m_pInspector = CInspector::Create(pSelectedObject);
	if (pInstance->m_pInspector == nullptr)
	{
		return nullptr;
	}

	pInstance->m_pResourceGUI = CResourceGUI::Create(
		pSelectedObject, pInstance->m_pCommandManager.get());
	if (pInstance->m_pResourceGUI == nullptr)
	{
		return nullptr;
	}

	pInstance->m_pMapChunkGUI = CMapChunkGUI::Create(pSelectedObject);
	if (pInstance->m_pMapChunkGUI == nullptr)
	{
		return nullptr;
	}

	pInstance->m_pNavMeshGUI = CNavMeshGUI::Create(pSelectedObject);
	if (pInstance->m_pNavMeshGUI == nullptr)
	{
		return nullptr;
	}

	pInstance->m_pTerrainGUI = CTerrainGUI::Create(pSelectedObject, pInstance->m_pCommandManager.get());
	if (pInstance->m_pTerrainGUI == nullptr)
	{
		return nullptr;
	}

	pInstance->m_pMapPickingPass = CMapPickingPass::Create();
	if (pInstance->m_pMapPickingPass == nullptr)
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
	//네비매시에서 쓰는동안은 막기
	if (m_pNavMeshGUI && m_pNavMeshGUI->IsMouseEditing())
		return;

	auto pActiveCamera = E::CGameInstance::Get().GetActiveCamera();
	if (pActiveCamera == nullptr)
	{
		return;
	}

	if (m_pSelection == nullptr || m_pSelection->GetCount() == 0)
	{
		return;
	}

	E::_float4x4 view{};
	E::_float4x4 proj{};
	XMStoreFloat4x4(&view, pActiveCamera->GetView());
	XMStoreFloat4x4(&proj, pActiveCamera->GetProj());

	const bool multiSelection = m_pSelection->GetCount() > 1;
	E::CGameObject* pSelectedObject = GetSelectedObject();
	E::_float3 pivotPosition{};
	E::_float4x4 gizmoMatrix{};

	if (multiSelection)
	{
		size_t validCount = 0;
		for (const auto& handle : m_pSelection->GetHandles())
		{
			if (auto* object = E::CGameInstance::Get().GetGameObjectByHandle(handle))
			{
				const auto& position = object->GetTransform().GetPosition();
				pivotPosition.x += position.x;
				pivotPosition.y += position.y;
				pivotPosition.z += position.z;
				++validCount;
			}
		}
		if (validCount == 0)
			return;

		const float inverseCount = 1.f / static_cast<float>(validCount);
		pivotPosition.x *= inverseCount;
		pivotPosition.y *= inverseCount;
		pivotPosition.z *= inverseCount;

		if (!m_bWasUsingGizmo)
		{
			XMStoreFloat4x4(&m_MultiGizmoStartMatrix,
				XMMatrixTranslation(pivotPosition.x, pivotPosition.y, pivotPosition.z));
			m_MultiGizmoCurrentMatrix = m_MultiGizmoStartMatrix;
			m_MultiGizmoStartTransforms.clear();

			for (const auto& handle : m_pSelection->GetHandles())
			{
				if (auto* object = E::CGameInstance::Get().GetGameObjectByHandle(handle))
				{
					object->GetTransform().Update();
					m_MultiGizmoStartTransforms.emplace_back(
						handle, *object->GetTransform().GetWorldMatrix());
				}
			}
		}

		gizmoMatrix = m_MultiGizmoCurrentMatrix;
	}
	else
	{
		if (pSelectedObject == nullptr)
			return;
		auto& selectedTransform = pSelectedObject->GetTransform();
		selectedTransform.Update();
		gizmoMatrix = *selectedTransform.GetWorldMatrix();
	}

	ImGuiViewport* pViewport = ImGui::GetMainViewport();
	const ImVec2 viewportPos = pViewport->Pos;
	const ImVec2 viewportSize = pViewport->Size;

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList(pViewport));
	ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);
	ImGuizmo::SetID(MAP_EDITOR_GIZMO_ID);
	const auto operation = m_GizmoOperation;
	const auto mode = multiSelection ? ImGuizmo::WORLD : m_GizmoMode;
	if (ImGuizmo::Manipulate(&view._11, &proj._11, operation, mode, &gizmoMatrix._11))
	{
		if (multiSelection)
		{
			m_MultiGizmoCurrentMatrix = gizmoMatrix;

			const XMMATRIX startPivot = XMLoadFloat4x4(&m_MultiGizmoStartMatrix);
			const XMMATRIX manipulatedPivot = XMLoadFloat4x4(&m_MultiGizmoCurrentMatrix);
			const XMMATRIX groupDelta = XMMatrixInverse(nullptr, startPivot) * manipulatedPivot;
			for (const auto& [handle, startWorld] : m_MultiGizmoStartTransforms)
			{
				if (auto* object = E::CGameInstance::Get().GetGameObjectByHandle(handle))
				{
					E::_float4x4 transformed{};
					XMStoreFloat4x4(&transformed, XMLoadFloat4x4(&startWorld) * groupDelta);
					ApplyMatrixToTransform(object->GetTransform(), transformed);
				}
			}
		}
		else
		{
			ApplyMatrixToTransform(pSelectedObject->GetTransform(), gizmoMatrix);
		}
	}

	const bool isUsingGizmo = ImGuizmo::IsUsing();
	if (m_bWasUsingGizmo && !isUsingGizmo)
	{
		E::CGameInstance::Get().RebuildMapChunks();
		m_MultiGizmoStartTransforms.clear();
		m_MultiGizmoStartMatrix = {};
		m_MultiGizmoCurrentMatrix = {};
	}
	m_bWasUsingGizmo = isUsingGizmo;
}

void CMapEditorGUI::PickMapMeshObject()
{
	const ImGuiIO& io = ImGui::GetIO();
	if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left) || io.WantCaptureMouse ||
		ImGuizmo::IsOver() || ImGuizmo::IsUsing())
	{
		return;
	}
	//네비매시 피킹중에는 잠시 막기
	if (m_pNavMeshGUI && m_pNavMeshGUI->IsMouseEditing())
		return;

	auto* selectedHandle = GetSelectedHandle();
	if (selectedHandle == nullptr || m_pMapPickingPass == nullptr)
		return;

	const E::_float2 mouse = E::CGameInstance::Get().GetMousePos();
	const E::_float2 clientSize = E::CGameInstance::Get().GetClientScreenSize();
	if (clientSize.x <= 0.f || clientSize.y <= 0.f ||
		mouse.x < 0.f || mouse.y < 0.f || mouse.x >= clientSize.x || mouse.y >= clientSize.y)
		return;

	if (const auto picked = m_pMapPickingPass->Pick(
		static_cast<uint32_t>(mouse.x), static_cast<uint32_t>(mouse.y)))
	{
		if (m_pSelection)
		{
			if (io.KeyCtrl)
				m_pSelection->Toggle(*picked);
			else
				m_pSelection->SelectSingle(*picked);
		}
		else
		{
			*selectedHandle = *picked;
		}
	}
}

void CMapEditorGUI::DrawMapMeshContextMenu()
{
	const E::_float2 mouse = E::CGameInstance::Get().GetMousePos();
	const E::_float2 clientSize = E::CGameInstance::Get().GetClientScreenSize();
	const bool mouseInClient = mouse.x >= 0.f && mouse.y >= 0.f &&
		mouse.x < clientSize.x && mouse.y < clientSize.y;

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && mouseInClient &&
		!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) &&
		!ImGuizmo::IsOver() && !ImGuizmo::IsUsing() && m_pMapPickingPass)
	{
		m_ContextMapMeshHandle = m_pMapPickingPass->Pick(
			static_cast<uint32_t>(mouse.x), static_cast<uint32_t>(mouse.y));
		if (m_ContextMapMeshHandle)
		{
			if (m_pSelection)
				m_pSelection->SelectSingle(*m_ContextMapMeshHandle);
			else if (auto* selectedHandle = GetSelectedHandle())
				*selectedHandle = *m_ContextMapMeshHandle;
			ImGui::OpenPopup("MapMesh Scene Context");
		}
	}

	if (ImGui::BeginPopup("MapMesh Scene Context"))
	{
		auto* object = m_ContextMapMeshHandle
			? E::CGameInstance::Get().GetGameObjectByHandleT<E::CMapMeshObject>(*m_ContextMapMeshHandle)
			: nullptr;

		if (object != nullptr)
		{
			ImGui::TextUnformatted(object->GetObjectTag().data());
			ImGui::Separator();
			if (ImGui::MenuItem("Delete Object"))
			{
				if (auto snapshot = MakeMapMeshObjectSnapshot(*m_ContextMapMeshHandle);
					snapshot && m_pCommandManager)
				{
					m_pCommandManager->Submit(std::make_unique<CDeleteMapMeshCommand>(
						*m_ContextMapMeshHandle, std::move(*snapshot), GetSelectedHandle()));
				}
				m_ContextMapMeshHandle.reset();
			}
		}
		else
		{
			ImGui::TextDisabled("Object no longer exists.");
		}

		ImGui::EndPopup();
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
