#include "pch.h"
#include <filesystem>
#include "AnimatedObjectPlacementManager.h"
#include "CameraObject.h"
#include "GameInstance.h"
#include "PhysXManager.h"

NS_USING(Engine)

namespace
{
	void DrawStringInput(const char* label, _string& value)
	{
		_char buffer[256]{};
		strncpy_s(buffer, value.c_str(), _TRUNCATE);
		if (ImGui::InputText(label, buffer, IM_ARRAYSIZE(buffer))) value = buffer;
	}
}

void CAnimatedObjectPlacementManager::UpdateGUI()
{
	ImGui::SetNextWindowSize(ImVec2{ 900.f, 680.f }, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Animated Object Placement Manager")) { ImGui::End(); return; }

	ImGui::Text("Placements: %zu", m_Placements.size());
	ImGui::SameLine();
	ImGui::TextColored(m_SpawnCallback ? ImVec4{ .3f, 1.f, .3f, 1.f } : ImVec4{ 1.f, .75f, .2f, 1.f },
		m_SpawnCallback ? "Client adapter connected" : "Client adapter not connected");
	ImGui::SetNextItemWidth(430.f);
	DrawStringInput("Placement File", m_sFilePath);
	if (ImGui::Button("Save Animated JSON"))
		m_sFileStatus = SUCCEEDED(Save(m_sFilePath)) ? "Save succeeded." : "Save failed.";
	ImGui::SameLine();
	if (ImGui::Button("Load Animated JSON"))
		m_sFileStatus = SUCCEEDED(Load(m_sFilePath)) ? "Load succeeded." : "Load failed.";
	if (!m_sFileStatus.empty()) ImGui::TextUnformatted(m_sFileStatus.c_str());

	if (ImGui::Button("Add Placement")) { AddPlacement(); m_iSelectedIndex = static_cast<int32_t>(m_Placements.size()) - 1; }
	ImGui::SameLine();
	if (ImGui::Button(m_bPlacementPicking ? "Pick Placement: ON" : "Pick Placement: OFF")) m_bPlacementPicking = !m_bPlacementPicking;
	ImGui::SameLine(); ImGui::Checkbox("Spawn On Pick", &m_bSpawnOnPick);
	ImGui::SameLine(); if (ImGui::Button("Spawn All") && m_SpawnCallback) SpawnAll();

	ImGui::Separator();
	ImGui::BeginChild("AnimatedPlacementList", ImVec2{ 220.f, 0.f }, true);
	for (size_t i = 0; i < m_Placements.size(); ++i)
	{
		const auto& desc = m_Placements[i];
		_char label[160]{};
		sprintf_s(label, "#%llu %s", static_cast<unsigned long long>(desc.iPlacementId),
			desc.sPrototypeTag.empty() ? "<Prototype>" : desc.sPrototypeTag.c_str());
		if (ImGui::Selectable(label, m_iSelectedIndex == static_cast<int32_t>(i))) m_iSelectedIndex = static_cast<int32_t>(i);
	}
	ImGui::EndChild();
	ImGui::SameLine();
	if (m_iSelectedIndex >= 0 && m_iSelectedIndex < static_cast<int32_t>(m_Placements.size()))
		DrawEditor(m_Placements[m_iSelectedIndex]);
	else ImGui::TextDisabled("Select or add a placement.");

	if (!m_LastResults.empty() && ImGui::CollapsingHeader("Last Spawn Results"))
		for (const auto& result : m_LastResults)
			ImGui::TextColored(result.bSucceeded ? ImVec4{ .3f, 1.f, .3f, 1.f } : ImVec4{ 1.f, .3f, .3f, 1.f },
				"#%llu %s", static_cast<unsigned long long>(result.iPlacementId), result.sMessage.c_str());

	UpdatePicking();
	ImGui::End();
}

void CAnimatedObjectPlacementManager::DrawEditor(ANIMATED_OBJECT_PLACEMENT_DESC& desc)
{
	ImGui::BeginGroup(); ImGui::PushID(static_cast<int32_t>(desc.iPlacementId)); ImGui::PushItemWidth(420.f);
	ImGui::Text("Placement ID: %llu", static_cast<unsigned long long>(desc.iPlacementId));
	_string selected{};
	for (const auto& option : m_Options)
		if (option.Desc.sPrototypeGroupTag == desc.sPrototypeGroupTag && option.Desc.sPrototypeTag == desc.sPrototypeTag) selected = option.sDisplayName;
	if (ImGui::BeginCombo("Object", selected.empty() ? "<Select>" : selected.c_str()))
	{
		for (const auto& option : m_Options)
			if (ImGui::Selectable(option.sDisplayName.c_str(), selected == option.sDisplayName))
			{
				const auto id = desc.iPlacementId; const auto pos = desc.vPosition; const auto rot = desc.vRotation; const auto scale = desc.vScale;
				desc = option.Desc; desc.iPlacementId = id; desc.vPosition = pos; desc.vRotation = rot; desc.vScale = scale;
			}
		ImGui::EndCombo();
	}
	const auto optionIt = std::ranges::find_if(m_Options, [&](const OPTION& option)
		{ return option.Desc.sPrototypeGroupTag == desc.sPrototypeGroupTag && option.Desc.sPrototypeTag == desc.sPrototypeTag; });
	if (optionIt != m_Options.end() && ImGui::BeginCombo("Animation", desc.sAnimationName.empty() ? "<Default>" : desc.sAnimationName.c_str()))
	{
		for (const auto& name : optionIt->AnimationNames)
			if (ImGui::Selectable(name.c_str(), name == desc.sAnimationName)) desc.sAnimationName = name;
		ImGui::EndCombo();
	}
	DrawStringInput("Prototype Group", desc.sPrototypeGroupTag); DrawStringInput("Prototype", desc.sPrototypeTag);
	DrawStringInput("Layer", desc.sLayerTag); DrawStringInput("Model Group", desc.sModelGroupTag);
	DrawStringInput("Model Resource", desc.sModelResourceTag); DrawStringInput("Animation Name", desc.sAnimationName);
	ImGui::DragFloat3("Position", &desc.vPosition.x, .1f); ImGui::DragFloat3("Rotation", &desc.vRotation.x, .5f);
	ImGui::DragFloat3("Scale", &desc.vScale.x, .01f, .001f, 100.f);
	ImGui::Checkbox("Auto Play", &desc.bAutoPlay); ImGui::SameLine(); ImGui::Checkbox("Loop", &desc.bLoop);
	ImGui::Checkbox("Cast Shadow", &desc.bCastShadow);
	ImGui::DragFloat("Animation Speed", &desc.fAnimationSpeed, .05f, 0.f, 10.f);
	ImGui::SliderFloat("Start Ratio", &desc.fStartRatio, 0.f, 1.f);
	ImGui::DragFloat("Visible Distance", &desc.fVisibleDistance, 1.f, 0.f, 100000.f);
	if (ImGui::Button("Spawn Selected") && m_SpawnCallback) Spawn(desc.iPlacementId);
	ImGui::SameLine();
	if (ImGui::Button("Duplicate")) { auto copy = desc; copy.iPlacementId = 0; AddPlacement(copy); m_iSelectedIndex = static_cast<int32_t>(m_Placements.size()) - 1; }
	ImGui::SameLine();
	if (ImGui::Button("Remove")) { const auto id = desc.iPlacementId; ImGui::PopItemWidth(); ImGui::PopID(); ImGui::EndGroup(); RemovePlacement(id); return; }
	ImGui::PopItemWidth(); ImGui::PopID(); ImGui::EndGroup();
}

void CAnimatedObjectPlacementManager::UpdatePicking()
{
	if (!m_bPlacementPicking || ImGui::GetIO().WantCaptureMouse || ImGui::IsAnyItemHovered() || ImGui::IsAnyItemActive() ||
		!CGameInstance::Get().MouseDown(MOUSEKEYSTATE::LB)) return;
	auto* camera = CGameInstance::Get().GetActiveCamera(); auto* physx = CGameInstance::Get().GetPhysXManager();
	if (!camera || !physx) return;
	const auto [origin, direction] = camera->GetRayFromScreenPixel(CGameInstance::Get().GetMousePos(), CGameInstance::Get().GetClientScreenSize());
	PX_RAYCAST_RESULT hit{};
	if (!physx->RayCast({ .vOrigin = origin, .vDirection = direction, .fMaxDistance = 1000.f, .tFilter = { .iQueryMask = m_iPickingQueryMask } }, hit)) return;
	ANIMATED_OBJECT_PLACEMENT_DESC desc{};
	if (m_iSelectedIndex >= 0 && m_iSelectedIndex < static_cast<int32_t>(m_Placements.size())) desc = m_Placements[m_iSelectedIndex];
	desc.iPlacementId = 0; desc.vPosition = hit.vHitpos;
	const auto id = AddPlacement(desc); m_iSelectedIndex = static_cast<int32_t>(m_Placements.size()) - 1;
	if (m_bSpawnOnPick && m_SpawnCallback) Spawn(id);
}

uint64_t CAnimatedObjectPlacementManager::AddPlacement(const ANIMATED_OBJECT_PLACEMENT_DESC& source)
{
	auto desc = source;
	if (desc.iPlacementId == 0 || std::ranges::any_of(m_Placements, [&](const auto& value) { return value.iPlacementId == desc.iPlacementId; })) desc.iPlacementId = AllocateId();
	else m_iNextPlacementId = std::max(m_iNextPlacementId, desc.iPlacementId + 1);
	m_Placements.emplace_back(std::move(desc)); return m_Placements.back().iPlacementId;
}

_bool CAnimatedObjectPlacementManager::RemovePlacement(uint64_t id)
{
	const auto it = std::ranges::find_if(m_Placements, [&](const auto& value) { return value.iPlacementId == id; });
	if (it == m_Placements.end()) return false; m_Placements.erase(it);
	m_iSelectedIndex = m_Placements.empty() ? -1 : std::min(m_iSelectedIndex, static_cast<int32_t>(m_Placements.size()) - 1); return true;
}

void CAnimatedObjectPlacementManager::ClearPlacements() { m_Placements.clear(); m_LastResults.clear(); m_iSelectedIndex = -1; }

void CAnimatedObjectPlacementManager::RegisterOption(const _string& name, const ANIMATED_OBJECT_PLACEMENT_DESC& desc, const std::vector<_string>& animations)
{
	if (name.empty()) return;
	OPTION value{ name, desc, animations };
	const auto it = std::ranges::find_if(m_Options, [&](const OPTION& option) { return option.sDisplayName == name; });
	if (it == m_Options.end()) m_Options.emplace_back(std::move(value)); else *it = std::move(value);
}

void CAnimatedObjectPlacementManager::ClearOptions() { m_Options.clear(); ClearSpawnCallback(); }

ANIMATED_OBJECT_PLACEMENT_RESULT CAnimatedObjectPlacementManager::Spawn(uint64_t id)
{
	const auto it = std::ranges::find_if(m_Placements, [&](const auto& value) { return value.iPlacementId == id; });
	auto result = it == m_Placements.end() ? ANIMATED_OBJECT_PLACEMENT_RESULT{ id, false, {}, "Placement not found." } :
		(!m_SpawnCallback ? ANIMATED_OBJECT_PLACEMENT_RESULT{ id, false, {}, "Client spawn adapter is not connected." } : m_SpawnCallback(*it));
	result.iPlacementId = id;
	if (result.bSucceeded && result.hObject == CHandle{}) { result.bSucceeded = false; result.sMessage = "Spawn callback returned an invalid handle."; }
	m_LastResults = { result }; return result;
}

const std::vector<ANIMATED_OBJECT_PLACEMENT_RESULT>& CAnimatedObjectPlacementManager::SpawnAll()
{
	m_LastResults.clear();
	for (const auto& desc : m_Placements)
	{
		auto result = m_SpawnCallback ? m_SpawnCallback(desc) : ANIMATED_OBJECT_PLACEMENT_RESULT{ desc.iPlacementId, false, {}, "Client spawn adapter is not connected." };
		result.iPlacementId = desc.iPlacementId;
		if (result.bSucceeded && result.hObject == CHandle{}) { result.bSucceeded = false; result.sMessage = "Spawn callback returned an invalid handle."; }
		m_LastResults.emplace_back(std::move(result));
	}
	return m_LastResults;
}

HRESULT CAnimatedObjectPlacementManager::Save(const _string& path) const
{
	if (path.empty()) return E_INVALIDARG; for (const auto& desc : m_Placements) if (!Validate(desc).empty()) return E_FAIL;
	const std::filesystem::path filePath{ path }; std::error_code error{}; if (!filePath.parent_path().empty()) std::filesystem::create_directories(filePath.parent_path(), error); if (error) return E_FAIL;
	ANIMATED_OBJECT_PLACEMENT_FILE file{}; file.Placements = m_Placements;
	return CGameInstance::Get().JsonSerialize(path, file, "AnimatedObjectPlacements");
}

HRESULT CAnimatedObjectPlacementManager::Load(const _string& path)
{
	if (path.empty()) return E_INVALIDARG; ANIMATED_OBJECT_PLACEMENT_FILE file{};
	if (FAILED(CGameInstance::Get().JsonDeSerialize(path, file, "AnimatedObjectPlacements")) || file.iVersion != 1) return E_FAIL;
	std::unordered_set<uint64_t> ids{}; uint64_t nextId = 1;
	for (const auto& desc : file.Placements) { if (desc.iPlacementId == 0 || !ids.emplace(desc.iPlacementId).second || !Validate(desc).empty()) return E_FAIL; nextId = std::max(nextId, desc.iPlacementId + 1); }
	m_Placements = std::move(file.Placements); m_iNextPlacementId = nextId; m_iSelectedIndex = m_Placements.empty() ? -1 : 0; m_LastResults.clear(); return S_OK;
}

_string CAnimatedObjectPlacementManager::Validate(const ANIMATED_OBJECT_PLACEMENT_DESC& desc) const
{
	if (desc.sPrototypeGroupTag.empty()) return "Prototype group tag is empty."; if (desc.sPrototypeTag.empty()) return "Prototype tag is empty."; if (desc.sLayerTag.empty()) return "Layer tag is empty.";
	if (desc.vScale.x <= 0.f || desc.vScale.y <= 0.f || desc.vScale.z <= 0.f) return "Scale must be greater than zero.";
	if (desc.fAnimationSpeed < 0.f || desc.fStartRatio < 0.f || desc.fStartRatio > 1.f || desc.fVisibleDistance < 0.f) return "Animation values are out of range."; return {};
}

uint64_t CAnimatedObjectPlacementManager::AllocateId()
{
	while (std::ranges::any_of(m_Placements, [&](const auto& value) { return value.iPlacementId == m_iNextPlacementId; })) ++m_iNextPlacementId; return m_iNextPlacementId++;
}

UPtr<CAnimatedObjectPlacementManager> CAnimatedObjectPlacementManager::Create() { return UPtr<CAnimatedObjectPlacementManager>(new CAnimatedObjectPlacementManager{}); }
