#include "pch.h"
#include "Hierarchy.h"
#include "GameInstance.h"
#include "MapMeshObject.h"
#include "EditorCommandManager.h"
#include "CreateMapMeshCommand.h"
#include "DeleteMapMeshCommand.h"
#include "MapMeshCommandCommon.h"
#include "EditorSelection.h"

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

	void AddDefaultMapMeshObject(E::CHandle* pSelectedObject,
		CEditorCommandManager* pCommandManager, const std::string& strLayerTag)
	{
		if (pSelectedObject == nullptr || pCommandManager == nullptr)
		{
			return;
		}

		static uint32_t s_iObjectIndex = 1;

		MAPMESH_OBJECT_SNAPSHOT snapshot{};
		snapshot.objectTag = "MapMesh_" + std::to_string(s_iObjectIndex++);
		snapshot.modelGroupTag = E::TAG_RES_GRP_MAPEDITOR_STATIC_MODEL;
		snapshot.modelResTag = E::TAG_RES_MAPEDITOR_DEFAULT_STATIC_MODEL;
		snapshot.layerTag = strLayerTag;
		pCommandManager->Submit(
			std::make_unique<CCreateMapMeshCommand>(std::move(snapshot), pSelectedObject));
	}

	void AddMapMeshObjectFromModelResource(E::CHandle* pSelectedObject,
		CEditorCommandManager* pCommandManager, const std::string& strLayerTag,
		const char* modelGroupTag, const char* modelResTag)
	{
		if (pSelectedObject == nullptr || pCommandManager == nullptr ||
			modelGroupTag == nullptr || modelResTag == nullptr)
		{
			return;
		}

		static uint32_t s_iDroppedObjectIndex = 1;

		MAPMESH_OBJECT_SNAPSHOT snapshot{};
		snapshot.objectTag = std::string("MapMesh_") + modelResTag + "_" +
			std::to_string(s_iDroppedObjectIndex++);
		snapshot.modelGroupTag = modelGroupTag;
		snapshot.modelResTag = modelResTag;
		snapshot.layerTag = strLayerTag;
		pCommandManager->Submit(
			std::make_unique<CCreateMapMeshCommand>(std::move(snapshot), pSelectedObject));
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

	void PasteMapMeshObject(E::CHandle* pSelectedObject,
		CEditorCommandManager* pCommandManager, const std::string& strLayerTag)
	{
		if (pSelectedObject == nullptr || pCommandManager == nullptr || !g_MapMeshClipboard.bValid)
		{
			return;
		}

		static uint32_t s_iPasteIndex = 1;

		MAPMESH_OBJECT_SNAPSHOT snapshot{};
		snapshot.objectTag = g_MapMeshClipboard.objectTag + "_Copy" + std::to_string(s_iPasteIndex++);
		snapshot.modelGroupTag = g_MapMeshClipboard.modelGroupTag;
		snapshot.modelResTag = g_MapMeshClipboard.modelResTag;
		snapshot.protoGroupTag = g_MapMeshClipboard.protoGroupTag;
		snapshot.prototypeTag = g_MapMeshClipboard.prototypeTag;
		snapshot.layerTag = strLayerTag;
		snapshot.position = g_MapMeshClipboard.position;
		snapshot.rotation = g_MapMeshClipboard.rotation;
		snapshot.scale = g_MapMeshClipboard.scale;
		pCommandManager->Submit(
			std::make_unique<CCreateMapMeshCommand>(std::move(snapshot), pSelectedObject));
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

	void HandleHierarchyShortcuts(E::CHandle* pSelectedObject,
		CEditorCommandManager* pCommandManager)
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
				PasteMapMeshObject(pSelectedObject, pCommandManager, pasteLayerName.value());
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
						AddMapMeshObjectFromModelResource(GetSelectedHandle(), m_pCommandManager,
							layerName, modelPayload->groupName, modelPayload->resourceName);
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
					AddDefaultMapMeshObject(GetSelectedHandle(), m_pCommandManager, layerName);
				}
				if (ImGui::MenuItem("Paste MapMeshObject", nullptr, false, g_MapMeshClipboard.bValid))
				{
					PasteMapMeshObject(GetSelectedHandle(), m_pCommandManager, layerName);
				}
				ImGui::EndPopup();
			}

			if (bOpen)
			{
				for (size_t handleIndex = 0; handleIndex < handles.size(); ++handleIndex)
				{
					const auto& handle = handles[handleIndex];
					auto* pObject = E::CGameInstance::Get().GetGameObjectByHandle(handle);
					if (pObject == nullptr)
					{
						continue;
					}

					const bool bSelected = m_pSelection
						? m_pSelection->IsSelected(handle)
						: (handle == *pSelectedObject);
					ImGui::PushID(static_cast<int>(handle.GetIndex()));
					if (ImGui::Selectable(pObject->GetObjectTag().data(), bSelected))
					{
						if (m_pSelection)
						{
							const ImGuiIO& io = ImGui::GetIO();
							if (io.KeyShift && m_RangeAnchor.has_value())
							{
								const auto anchorIter = std::find(handles.begin(), handles.end(), *m_RangeAnchor);
								if (anchorIter != handles.end())
								{
									const size_t anchorIndex = static_cast<size_t>(std::distance(handles.begin(), anchorIter));
									m_pSelection->SelectRange(handles, anchorIndex, handleIndex, io.KeyCtrl);
								}
								else
								{
									m_pSelection->SelectSingle(handle);
									m_RangeAnchor = handle;
								}
							}
							else if (io.KeyCtrl)
							{
								m_pSelection->Toggle(handle);
								m_RangeAnchor = handle;
							}
							else
							{
								m_pSelection->SelectSingle(handle);
								m_RangeAnchor = handle;
							}
						}
						else
						{
							*pSelectedObject = handle;
						}
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
							PasteMapMeshObject(GetSelectedHandle(), m_pCommandManager, layerName);
						}
						ImGui::Separator();
						if (ImGui::MenuItem("Rename Object"))
						{
							OpenRenamePopup(handle, pObject->GetObjectTag());
						}
						if (ImGui::MenuItem("Delete Object", nullptr, false, bCanCopyMapMesh))
						{
							if (auto snapshot = MakeMapMeshObjectSnapshot(handle); snapshot && m_pCommandManager)
							{
								m_pCommandManager->Submit(std::make_unique<CDeleteMapMeshCommand>(
									handle, std::move(*snapshot), GetSelectedHandle()));
							}
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
		HandleHierarchyShortcuts(GetSelectedHandle(), m_pCommandManager);
	}

	ImGui::EndChild();
	DrawRenamePopup();
}

E::UPtr<CHierarchy> CHierarchy::Create(E::CHandle* pSelectedObject,
	CEditorCommandManager* pCommandManager, CEditorSelection* pSelection)
{
	auto pInstance = E::UPtr<CHierarchy>(new CHierarchy{});
	if (FAILED(pInstance->Initialize(pSelectedObject)))
	{
		MSG_BOX("Failed to Created : CHierarchy");
		return nullptr;
	}
	pInstance->m_pCommandManager = pCommandManager;
	pInstance->m_pSelection = pSelection;

	return pInstance;
}

