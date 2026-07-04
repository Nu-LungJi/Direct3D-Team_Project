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

	//if (ImGui::Button("Add Map Mesh", ImVec2(120.f, 0.f)))
	//{
	//	AddDefaultMapMeshObject(GetSelectedHandle());
	//}
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

			// --- 우클릭 오브젝트 추가 로직 ---
			// BeginPopupContextItem은 바로 직전에 호출된 위젯(TreeNode)을 대상으로 우클릭을 감지
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Create MapMeshObject"))
				{
					AddDefaultMapMeshObject(GetSelectedHandle(), layerName);
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

