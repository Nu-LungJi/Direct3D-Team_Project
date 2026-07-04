#include "pch.h"
#include "Hierarchy.h"
#include "GameInstance.h"

NS_USING(Client)

CHierarchy::CHierarchy()
{
}

CHierarchy::~CHierarchy()
{
}

void CHierarchy::UpdateGUI(E::_float fTimeDelta)
{
	ImGui::TextDisabled("Hierarchy");
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
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}

	ImGui::EndChild();
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
