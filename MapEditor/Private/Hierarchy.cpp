#include "pch.h"
#include "Hierarchy.h"
#include "GameInstance.h"
#include "MapMeshObject.h"

NS_USING(Client)

namespace
{
	E::CHandle g_RenameTarget{};
	char g_RenameBuffer[128]{};
	bool g_bOpenRenamePopup = false;

	struct MapMeshObjectClipboard
	{
		_bool bValid = false;
		std::string objectTag{};
		std::string modelGroupTag{};
		std::string modelResTag{};
		std::string protoGroupTag = "PERMANENT";
		std::string prototypeTag = "Prototype_GameObject_MapMeshObject";
		E::_float3 position{};
		E::_float4 rotation{ 0.f, 0.f, 0.f, 1.f };
		E::_float3 scale{ 1.f, 1.f, 1.f };
	};

	struct ModelResourceDragPayload
	{
		char groupName[128]{};
		char resourceName[128]{};
	};

	constexpr const char* PAYLOAD_MODEL_RESOURCE = "MAPEDITOR_MODEL_RESOURCE";

	MapMeshObjectClipboard g_MapMeshClipboard{};

	void AddDefaultMapMeshObject(E::CHandle* pSelectedObject, const std::string& strLayerTag)
	{
		if (pSelectedObject == nullptr)
		{
			return;
		}

		static uint32_t s_iObjectIndex = 1;

		E::CMapMeshObject::MAP_MESH_OBJECT_DESC Desc{};
		Desc.sObjectTag = "MapMesh_" + std::to_string(s_iObjectIndex++);
		Desc.modelGroupTag = "TEST";
		Desc.modelResTag = "Model_Resource";
		Desc.protoGroupTag = "PERMANENT";
		Desc.prototypeTag = "Prototype_GameObject_MapMeshObject";

		if (auto hObject = E::CGameInstance::Get().AddGameObjectToLayer(
			Desc.protoGroupTag,
			Desc.prototypeTag,
			strLayerTag,
			&Desc))
		{
			*pSelectedObject = hObject.value();
		}
	}

	void AddMapMeshObjectFromModelResource(E::CHandle* pSelectedObject, const std::string& strLayerTag, const char* modelGroupTag, const char* modelResTag)
	{
		if (pSelectedObject == nullptr || modelGroupTag == nullptr || modelResTag == nullptr)
		{
			return;
		}

		static uint32_t s_iDroppedObjectIndex = 1;

		E::CMapMeshObject::MAP_MESH_OBJECT_DESC Desc{};
		Desc.sObjectTag = std::string("MapMesh_") + modelResTag + "_" + std::to_string(s_iDroppedObjectIndex++);
		Desc.modelGroupTag = modelGroupTag;
		Desc.modelResTag = modelResTag;
		Desc.protoGroupTag = "PERMANENT";
		Desc.prototypeTag = "Prototype_GameObject_MapMeshObject";

		if (auto hObject = E::CGameInstance::Get().AddGameObjectToLayer(
			Desc.protoGroupTag,
			Desc.prototypeTag,
			strLayerTag,
			&Desc))
		{
			*pSelectedObject = hObject.value();
		}
	}

	_bool CopyMapMeshObject(const E::CHandle& handle)
	{
		auto* pMapMeshObject = E::CGameInstance::Get().GetGameObjectByHandleT<E::CMapMeshObject>(handle);
		if (pMapMeshObject == nullptr)
		{
			return false;
		}

		const auto& transform = pMapMeshObject->GetTransform();
		g_MapMeshClipboard.bValid = true;
		g_MapMeshClipboard.objectTag = pMapMeshObject->GetObjectTag();
		g_MapMeshClipboard.modelGroupTag = pMapMeshObject->GetModelResourceGroup();
		g_MapMeshClipboard.modelResTag = pMapMeshObject->GetModelResourceTag();
		g_MapMeshClipboard.position = transform.GetPosition();
		g_MapMeshClipboard.rotation = transform.GetQuaternion();
		g_MapMeshClipboard.scale = transform.GetScale();

		return true;
	}

	void PasteMapMeshObject(E::CHandle* pSelectedObject, const std::string& strLayerTag)
	{
		if (pSelectedObject == nullptr || !g_MapMeshClipboard.bValid)
		{
			return;
		}

		static uint32_t s_iPasteIndex = 1;

		E::CMapMeshObject::MAP_MESH_OBJECT_DESC Desc{};
		Desc.sObjectTag = g_MapMeshClipboard.objectTag + "_Copy" + std::to_string(s_iPasteIndex++);
		Desc.modelGroupTag = g_MapMeshClipboard.modelGroupTag;
		Desc.modelResTag = g_MapMeshClipboard.modelResTag;
		Desc.protoGroupTag = g_MapMeshClipboard.protoGroupTag;
		Desc.prototypeTag = g_MapMeshClipboard.prototypeTag;

		if (auto hObject = E::CGameInstance::Get().AddGameObjectToLayer(
			Desc.protoGroupTag,
			Desc.prototypeTag,
			strLayerTag,
			&Desc))
		{
			auto* pPastedObject = E::CGameInstance::Get().GetGameObjectByHandle(hObject.value());
			if (pPastedObject != nullptr)
			{
				auto& transform = pPastedObject->GetTransform();
				transform.SetPosition(g_MapMeshClipboard.position);
				transform.SetQuaternion(g_MapMeshClipboard.rotation);
				transform.SetScale(g_MapMeshClipboard.scale);
			}

			*pSelectedObject = hObject.value();
		}
	}

	std::optional<std::string> FindLayerNameByHandle(const E::CHandle& handle)
	{
		const auto& layers = E::CGameInstance::Get().GetGameObjectLayers();
		for (const auto& [layerName, handles] : layers)
		{
			for (const auto& layerHandle : handles)
			{
				if (layerHandle == handle)
				{
					return layerName;
				}
			}
		}

		return std::nullopt;
	}

	std::optional<std::string> FindDefaultPasteLayer(const E::CHandle& selectedObject)
	{
		if (auto selectedLayerName = FindLayerNameByHandle(selectedObject))
		{
			return selectedLayerName;
		}

		const auto& layers = E::CGameInstance::Get().GetGameObjectLayers();
		if (!layers.empty())
		{
			return layers.front().first;
		}

		return std::nullopt;
	}

	void HandleHierarchyShortcuts(E::CHandle* pSelectedObject)
	{
		if (pSelectedObject == nullptr)
		{
			return;
		}

		const ImGuiIO& io = ImGui::GetIO();
		if (!io.KeyCtrl || io.WantTextInput)
		{
			return;
		}

		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_C), false))
		{
			CopyMapMeshObject(*pSelectedObject);
		}

		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_V), false))
		{
			if (auto pasteLayerName = FindDefaultPasteLayer(*pSelectedObject))
			{
				PasteMapMeshObject(pSelectedObject, pasteLayerName.value());
			}
		}
	}

	void OpenRenamePopup(const E::CHandle& handle, std::string_view objectTag)
	{
		g_RenameTarget = handle;
		g_bOpenRenamePopup = true;

		const size_t copyLength = std::min(objectTag.size(), sizeof(g_RenameBuffer) - 1);
		std::memset(g_RenameBuffer, 0, sizeof(g_RenameBuffer));
		std::memcpy(g_RenameBuffer, objectTag.data(), copyLength);
	}

	void DrawRenamePopup()
	{
		if (g_bOpenRenamePopup)
		{
			ImGui::OpenPopup("Rename Object");
			g_bOpenRenamePopup = false;
		}

		if (ImGui::BeginPopupModal("Rename Object", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::InputText("Name", g_RenameBuffer, sizeof(g_RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::Separator();

			if (ImGui::Button("OK", ImVec2(90.f, 0.f)))
			{
				if (auto* pObject = E::CGameInstance::Get().GetGameObjectByHandle(g_RenameTarget))
				{
					pObject->SetObjectTag(g_RenameBuffer);
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(90.f, 0.f)))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}
}

CHierarchy::CHierarchy()
{
}

CHierarchy::~CHierarchy()
{
}

void CHierarchy::UpdateGUI(E::_float fTimeDelta)
{
	ImGui::TextDisabled("Hierarchy");

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Add TEST / Model_Resource to 00_OBJECTS");
	}

	ImGui::BeginChild("##ObjectList", ImVec2(0.f, 156.f), true);

	if (auto* pSelectedObject = GetSelectedHandle())
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

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PAYLOAD_MODEL_RESOURCE))
				{
					const auto* modelPayload = static_cast<const ModelResourceDragPayload*>(payload->Data);
					if (modelPayload != nullptr)
					{
						AddMapMeshObjectFromModelResource(GetSelectedHandle(), layerName, modelPayload->groupName, modelPayload->resourceName);
					}
				}
				ImGui::EndDragDropTarget();
			}

			// --- 우클릭 오브젝트 추가 로직 ---
			// BeginPopupContextItem은 바로 직전에 호출된 위젯(TreeNode)을 대상으로 우클릭을 감지
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Create MapMeshObject"))
				{
					AddDefaultMapMeshObject(GetSelectedHandle(), layerName);
				}
				if (ImGui::MenuItem("Paste MapMeshObject", nullptr, false, g_MapMeshClipboard.bValid))
				{
					PasteMapMeshObject(GetSelectedHandle(), layerName);
				}
				ImGui::EndPopup();
			}

			if (bOpen)
			{
				for (const auto& handle : handles)
				{
					auto* pObject = E::CGameInstance::Get().GetGameObjectByHandle(handle);
					if (pObject == nullptr)
					{
						continue;
					}

					const bool bSelected = (handle == *pSelectedObject);
					ImGui::PushID(static_cast<int>(handle.GetIndex()));
					if (ImGui::Selectable(pObject->GetObjectTag().data(), bSelected))
					{
						*pSelectedObject = handle;
					}
					ImGui::PopID();

					// --- 우클릭 삭제 로직 ---
					// BeginPopupContextItem은 바로 직전에 호출된 위젯(TreeNode)을 대상으로 우클릭을 감지
					if (ImGui::BeginPopupContextItem())
					{
						const bool bCanCopyMapMesh = E::CGameInstance::Get().GetGameObjectByHandleT<E::CMapMeshObject>(handle) != nullptr;
						if (ImGui::MenuItem("Copy Object", nullptr, false, bCanCopyMapMesh))
						{
							CopyMapMeshObject(handle);
						}
						if (ImGui::MenuItem("Paste MapMeshObject", nullptr, false, g_MapMeshClipboard.bValid))
						{
							PasteMapMeshObject(GetSelectedHandle(), layerName);
						}
						ImGui::Separator();
						if (ImGui::MenuItem("Rename Object"))
						{
							OpenRenamePopup(handle, pObject->GetObjectTag());
						}
						if (ImGui::MenuItem("Delete Object"))
						{
							pObject->SetPendingDestroyCascade(true);
						}
						ImGui::EndPopup();
					}
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}

	bool bHierarchyFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	if (bHierarchyFocused)
	{
		HandleHierarchyShortcuts(GetSelectedHandle());
	}

	ImGui::EndChild();
	DrawRenamePopup();
}

E::UPtr<CHierarchy> CHierarchy::Create(E::CHandle* pSelectedObject)
{
	auto pInstance = E::UPtr<CHierarchy>(new CHierarchy{});
	if (FAILED(pInstance->Initialize(pSelectedObject)))
	{
		MSG_BOX("Failed to Created : CHierarchy");
		return nullptr;
	}

	return pInstance;
}

