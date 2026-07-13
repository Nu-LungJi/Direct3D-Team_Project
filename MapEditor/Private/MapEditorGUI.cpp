#include "pch.h"
#include "MapEditorGUI.h"
#include "GameInstance.h"
#include "MapMeshObject.h"
#include "ResStaticModel.h"
#include "ResStaticModelMesh.h"
NS_USING(Client)

namespace
{
	bool PickMapMeshSubMeshBounds(E::CMapMeshObject& object, E::_fvector worldRayOrigin,
		E::_fvector worldRayDirection, float& outWorldDistance)
	{
		auto model = E::CGameInstance::Get().GetResourceFirst<E::CResStaticModel>(
			object.GetModelResourceGroup(), object.GetModelResourceTag());
		if (!model)
			return false;

		object.GetTransform().Update();
		const E::_matrix world = object.GetTransform().GetLoadedCombinedWorldMatrix();
		E::_vector determinant{};
		const E::_matrix inverseWorld = XMMatrixInverse(&determinant, world);
		const E::_vector localOrigin = XMVector3TransformCoord(worldRayOrigin, inverseWorld);
		const E::_vector localDirection = XMVector3Normalize(
			XMVector3TransformNormal(worldRayDirection, inverseWorld));

		bool picked = false;
		outWorldDistance = FLT_MAX;
		for (const auto& mesh : model->GetMeshes())
		{
			if (!mesh)
				continue;

			DirectX::BoundingBox localBounds{};
			DirectX::BoundingBox::CreateFromPoints(localBounds,
				XMLoadFloat3(&mesh->GetMinPos()), XMLoadFloat3(&mesh->GetMaxPos()));

			float localDistance = 0.f;
			if (!localBounds.Intersects(localOrigin, localDirection, localDistance))
				continue;

			const E::_vector worldHit = XMVector3TransformCoord(
				localOrigin + localDirection * localDistance, world);
			const float worldDistance = XMVectorGetX(XMVector3Length(worldHit - worldRayOrigin));
			if (worldDistance < outWorldDistance)
			{
				outWorldDistance = worldDistance;
				picked = true;
			}
		}

		return picked;
	}

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
		const std::string mapPath = MakeMapPath(m_MapName);
		CGameInstance::Get().SaveMap(mapPath);
		if (m_pNavMeshGUI)
		{
			m_pNavMeshGUI->SaveNavMesh(mapPath);
		}
		ImGui::OpenPopup("SaveCheck");
	}
	ImGui::SameLine();
	if (ImGui::Button("Level Load", ImVec2(112.f, 0.f)))
	{
		const std::string mapPath = MakeMapPath(m_MapName);
		CGameInstance::Get().LoadMap(mapPath, true);
		if (m_pNavMeshGUI)
		{
			m_pNavMeshGUI->LoadNavMesh(mapPath);
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

	bool bMapMeshDebugBounds = E::CMapMeshObject::IsDebugBoundsEnabled();
	if (ImGui::Checkbox("MapMesh BoundingBox", &bMapMeshDebugBounds))
	{
		E::CMapMeshObject::SetDebugBoundsEnabled(bMapMeshDebugBounds);
	}
	const auto& instancingStats = E::CMapMeshObject::GetInstancingStats();
	ImGui::Text("Mode: %s", instancingStats.bEnabled ? "Instanced" : "Normal");
	ImGui::Text("Objects: %u", instancingStats.iObjects);
	ImGui::Text("Batches: %u", instancingStats.iBatches);
	ImGui::Text("Instances: %u", instancingStats.iInstances);
	ImGui::Text("DrawCalls: %u", instancingStats.iDrawCalls);

	ImGui::Text("----------------------------Occlusion-----------------------------------");
	ImGui::Text("Visible: %u (cpu readback)", instancingStats.iVisibleInstances);
	ImGui::Text("Culled: %u (cpu readback)", instancingStats.iCulledInstances);
	ImGui::Text("----------------------------Occlusion-----------------------------------");

	m_pNavMeshGUI->UpdateGUI(fTimeDelta);

	ImGui::Separator();
	m_pHierarchy->UpdateGUI(fTimeDelta);

	ImGui::Separator();
	m_pInspector->UpdateGUI(fTimeDelta);

	ImGui::PopStyleVar(2);
	ImGui::End();

	m_pResourceGUI->UpdateGUI(fTimeDelta);
	m_pMapChunkGUI->UpdateGUI(fTimeDelta);
	RenderGizmo();
	PickMapMeshObject();
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

	pInstance->m_pNavMeshGUI = CNavMeshGUI::Create(pSelectedObject);
	if (pInstance->m_pNavMeshGUI == nullptr)
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

	const bool isUsingGizmo = ImGuizmo::IsUsing();
	if (m_bWasUsingGizmo && !isUsingGizmo)
	{
		E::CGameInstance::Get().RebuildMapChunks();
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

	auto* camera = E::CGameInstance::Get().GetActiveCamera();
	auto* selectedHandle = GetSelectedHandle();
	if (camera == nullptr || selectedHandle == nullptr)
		return;

	const E::_float2 mouse = E::CGameInstance::Get().GetMousePos();
	const E::_float2 clientSize = E::CGameInstance::Get().GetClientScreenSize();
	if (clientSize.x <= 0.f || clientSize.y <= 0.f)
		return;

	const E::_matrix identity = XMMatrixIdentity();
	const E::_vector nearPoint = XMVector3Unproject(
		XMVectorSet(mouse.x, mouse.y, 0.f, 1.f),
		0.f, 0.f, clientSize.x, clientSize.y, 0.f, 1.f,
		camera->GetProj(), camera->GetView(), identity);
	const E::_vector farPoint = XMVector3Unproject(
		XMVectorSet(mouse.x, mouse.y, 1.f, 1.f),
		0.f, 0.f, clientSize.x, clientSize.y, 0.f, 1.f,
		camera->GetProj(), camera->GetView(), identity);
	const E::_vector rayDirection = XMVector3Normalize(farPoint - nearPoint);

	const std::vector<E::CHandle> candidates =
		E::CGameInstance::Get().CollectMapMeshPickCandidates(nearPoint, rayDirection);
	float nearestDistance = FLT_MAX;
	std::optional<E::CHandle> pickedHandle;
	for (const E::CHandle& handle : candidates)
	{
		auto* object = E::CGameInstance::Get().GetGameObjectByHandleT<E::CMapMeshObject>(handle);
		float distance = 0.f;
		if (object && PickMapMeshSubMeshBounds(*object, nearPoint, rayDirection, distance) &&
			distance < nearestDistance)
		{
			nearestDistance = distance;
			pickedHandle = handle;
		}
	}

	if (pickedHandle)
		*selectedHandle = *pickedHandle;
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
