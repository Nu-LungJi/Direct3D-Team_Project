#include "pch.h"
#include "Inspector.h"
#include "ComTransform.h"

NS_USING(Client)

namespace
{
	bool DrawVec3Control(const char* label, E::_float3& value, const E::_float3& resetValue, float speed)
	{
		bool changed = false;

		ImGui::PushID(label);
		ImGui::TextUnformatted(label);
		ImGui::SameLine(82.f);
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 34.f);
		changed = ImGui::DragFloat3("##Value", reinterpret_cast<float*>(&value), speed, 0.f, 0.f, "%.3f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		if (ImGui::Button("R", ImVec2(26.f, 0.f)))
		{
			value = resetValue;
			changed = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Reset %s", label);
		}
		ImGui::PopID();

		return changed;
	}

	void DrawSelectedTransform(E::CComTransform& transform)
	{
		E::_float3 position = transform.GetPosition();
		E::_float3 rotation = transform.GetRotationEuler();
		E::_float3 scale = transform.GetScale();

		if (DrawVec3Control("Position", position, E::_float3{ 0.f, 0.f, 0.f }, 0.1f))
		{
			transform.SetPosition(position);
		}
		if (DrawVec3Control("Rotation", rotation, E::_float3{ 0.f, 0.f, 0.f }, 0.5f))
		{
			transform.SetRotationEuler(rotation);
		}
		if (DrawVec3Control("Scale", scale, E::_float3{ 1.f, 1.f, 1.f }, 0.1f))
		{
			transform.SetScale(scale);
		}

		transform.Update();
	}
}

CInspector::CInspector()
{
}

CInspector::~CInspector()
{
}

void CInspector::UpdateGUI(E::_float fTimeDelta)
{
	ImGui::TextDisabled("Inspector");

	auto* pSelectedObject = GetSelectedObject();
	auto* pSelectedHandle = GetSelectedHandle();
	if (pSelectedObject == nullptr || pSelectedHandle == nullptr)
	{
		ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, 0.55f), "Select an object to view details.");
		return;
	}

	ImGui::BeginChild("##Inspector", ImVec2(0.f, 0.f), true);
	ImGui::TextUnformatted("Name");
	ImGui::SameLine(82.f);
	ImGui::TextUnformatted(pSelectedObject->GetObjectTag().data());

	ImGui::TextUnformatted("Handle");
	ImGui::SameLine(82.f);
	ImGui::Text("%zu : %u", pSelectedHandle->GetIndex(), pSelectedHandle->GetGeneration());

	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		DrawSelectedTransform(pSelectedObject->GetTransform());
	}
	ImGui::EndChild();
}

E::UPtr<CInspector> CInspector::Create(E::CHandle* pSelectedObject)
{
	auto pInstance = E::UPtr<CInspector>(new CInspector{});
	if (FAILED(pInstance->Initialize(pSelectedObject)))
	{
		MSG_BOX("Failed to Created : CInspector");
		return nullptr;
	}

	return pInstance;
}
