#include "pch.h"
#include <filesystem>
#include "NpcPlacementManager.h"
#include "GameInstance.h"
#include "ResModel.h"
#include "CameraObject.h"
#include "PhysXManager.h"

NS_USING(Engine)

CNpcPlacementManager::CNpcPlacementManager() = default;
CNpcPlacementManager::~CNpcPlacementManager() = default;

void CNpcPlacementManager::DrawStringInput(const char* pLabel, _string& Value)
{
	_char Buffer[256]{};
	strncpy_s(Buffer, Value.c_str(), _TRUNCATE);
	if (ImGui::InputText(pLabel, Buffer, IM_ARRAYSIZE(Buffer)))
		Value = Buffer;
}

_bool CNpcPlacementManager::DrawStringSelection(
	const char* pLabel, _string& Value, const std::vector<_string>& Options)
{
	_bool bChanged = false;
	const char* pPreview = Value.empty() ? "<Select>" : Value.c_str();
	if (ImGui::BeginCombo(pLabel, pPreview))
	{
		for (const auto& Option : Options)
		{
			const _bool bSelected = Value == Option;
			if (ImGui::Selectable(Option.c_str(), bSelected))
			{
				Value = Option;
				bChanged = true;
			}
			if (bSelected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	return bChanged;
}

void CNpcPlacementManager::SortAndUnique(std::vector<_string>& Options)
{
	std::ranges::sort(Options);
	const auto Iter = std::ranges::unique(Options).begin();
	Options.erase(Iter, Options.end());
}

void CNpcPlacementManager::UpdateGUI()
{
	ImGui::SetNextWindowSize(ImVec2{ 920.f, 680.f }, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints(ImVec2{ 680.f, 480.f }, ImVec2{ 1200.f, 900.f });
	if (!ImGui::Begin("NPC Placement Manager"))
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Placements: %zu", m_Placements.size());
	ImGui::SameLine();
	ImGui::TextColored(
		HasSpawnCallback() ? ImVec4{ 0.3f, 1.f, 0.3f, 1.f } : ImVec4{ 1.f, 0.75f, 0.2f, 1.f },
		HasSpawnCallback() ? "Client adapter connected" : "Client adapter not connected");

	ImGui::SetNextItemWidth(420.f);
	DrawStringInput("Placement File", m_sFilePath);
	if (ImGui::Button("Save JSON"))
	{
		if (std::filesystem::exists(std::filesystem::path{ m_sFilePath }))
		{
			m_eFileConfirmAction = FILE_CONFIRM_ACTION::SAVE;
			ImGui::OpenPopup("Confirm NPC Placement File");
		}
		else
			m_sFileStatus = SUCCEEDED(Save(m_sFilePath)) ? "Save succeeded." : "Save failed. Check placement data or path.";
	}
	ImGui::SameLine();
	if (ImGui::Button("Load JSON"))
	{
		if (!m_Placements.empty())
		{
			m_eFileConfirmAction = FILE_CONFIRM_ACTION::LOAD;
			ImGui::OpenPopup("Confirm NPC Placement File");
		}
		else
			m_sFileStatus = SUCCEEDED(Load(m_sFilePath)) ? "Load succeeded." : "Load failed. Check JSON data or version.";
	}
	DrawFileConfirmPopup();
	if (!m_sFileStatus.empty()) ImGui::TextUnformatted(m_sFileStatus.c_str());

	if (ImGui::Button("Add Placement"))
	{
		AddPlacement();
		m_iSelectedIndex = static_cast<int32_t>(m_Placements.size()) - 1;
	}
	ImGui::SameLine();
	if (ImGui::Button(m_bPlacementPicking ? "Pick Placement: ON" : "Pick Placement: OFF"))
		m_bPlacementPicking = !m_bPlacementPicking;
	ImGui::SameLine();
	ImGui::Checkbox("Spawn On Pick", &m_bSpawnOnPick);
	ImGui::SameLine();
	if (ImGui::Button("Spawn All") && HasSpawnCallback()) SpawnAll();
	ImGui::SameLine();
	if (ImGui::Button("Clear Results")) m_LastResults.clear();

	ImGui::Separator();
	ImGui::BeginChild("NpcPlacementList", ImVec2{ 210.f, 0.f }, true);
	for (size_t i = 0; i < m_Placements.size(); ++i)
	{
		const auto& Desc = m_Placements[i];
		_char Label[128]{};
		sprintf_s(Label, "#%llu  %s", static_cast<unsigned long long>(Desc.iPlacementId),
			Desc.sPrototypeTag.empty() ? "<Prototype>" : Desc.sPrototypeTag.c_str());
		if (ImGui::Selectable(Label, m_iSelectedIndex == static_cast<int32_t>(i)))
			m_iSelectedIndex = static_cast<int32_t>(i);
	}
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginGroup();
	if (m_iSelectedIndex >= 0 && m_iSelectedIndex < static_cast<int32_t>(m_Placements.size()))
		DrawPlacementEditor(m_Placements[m_iSelectedIndex], static_cast<size_t>(m_iSelectedIndex));
	else
		ImGui::TextDisabled("Select or add a placement.");
	ImGui::EndGroup();

	if (!m_LastResults.empty() && ImGui::CollapsingHeader("Last Spawn Results", ImGuiTreeNodeFlags_DefaultOpen))
	{
		for (const auto& Result : m_LastResults)
		{
			ImGui::TextColored(
				Result.bSucceeded ? ImVec4{ 0.3f, 1.f, 0.3f, 1.f } : ImVec4{ 1.f, 0.3f, 0.3f, 1.f },
				"#%llu %s - %s", static_cast<unsigned long long>(Result.iPlacementId),
				Result.bSucceeded ? "Success" : "Failed", Result.sMessage.c_str());
		}
	}

	UpdatePlacementPicking();

	ImGui::End();
}

void CNpcPlacementManager::DrawFileConfirmPopup()
{
	if (!ImGui::BeginPopupModal("Confirm NPC Placement File", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return;

	if (m_eFileConfirmAction == FILE_CONFIRM_ACTION::SAVE)
	{
		ImGui::TextUnformatted("The JSON file already exists.");
		ImGui::TextUnformatted("Overwrite the existing file?");
	}
	else if (m_eFileConfirmAction == FILE_CONFIRM_ACTION::LOAD)
	{
		ImGui::TextUnformatted("Loading will replace the current placement list.");
		ImGui::TextUnformatted("Discard current edits and continue?");
	}

	ImGui::Separator();
	if (ImGui::Button("Confirm", ImVec2{ 120.f, 0.f }))
	{
		if (m_eFileConfirmAction == FILE_CONFIRM_ACTION::SAVE)
			m_sFileStatus = SUCCEEDED(Save(m_sFilePath)) ? "Save succeeded." : "Save failed. Check placement data or path.";
		else if (m_eFileConfirmAction == FILE_CONFIRM_ACTION::LOAD)
			m_sFileStatus = SUCCEEDED(Load(m_sFilePath)) ? "Load succeeded." : "Load failed. Check JSON data or version.";

		m_eFileConfirmAction = FILE_CONFIRM_ACTION::NONE;
		ImGui::CloseCurrentPopup();
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel", ImVec2{ 120.f, 0.f }))
	{
		m_eFileConfirmAction = FILE_CONFIRM_ACTION::NONE;
		m_sFileStatus = "Operation cancelled.";
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}

void CNpcPlacementManager::UpdatePlacementPicking()
{
	if (!m_bPlacementPicking ||
		ImGui::GetIO().WantCaptureMouse ||
		ImGui::IsAnyItemHovered() ||
		ImGui::IsAnyItemActive() ||
		!CGameInstance::Get().MouseDown(MOUSEKEYSTATE::LB))
		return;

	auto* pCamera = CGameInstance::Get().GetActiveCamera();
	auto* pPhysXManager = CGameInstance::Get().GetPhysXManager();
	if (!pCamera || !pPhysXManager) return;

	const _float2 vMousePosition = CGameInstance::Get().GetMousePos();
	const _float2 vViewportSize = CGameInstance::Get().GetClientScreenSize();
	const auto [vOrigin, vDirection] = pCamera->GetRayFromScreenPixel(vMousePosition, vViewportSize);

	PX_RAYCAST_RESULT RayResult{};
	if (!pPhysXManager->RayCast(
		{
			.vOrigin = vOrigin,
			.vDirection = vDirection,
			.fMaxDistance = 1000.f,
			.tFilter = { .iQueryMask = m_iPickingQueryMask }
		},
		RayResult))
		return;

	NPC_PLACEMENT_DESC Desc{};
	if (m_iSelectedIndex >= 0 && m_iSelectedIndex < static_cast<int32_t>(m_Placements.size()))
		Desc = m_Placements[m_iSelectedIndex];

	Desc.iPlacementId = 0;
	Desc.vPosition = RayResult.vHitpos;
	Desc.vPatrolStartPosition = RayResult.vHitpos;
	Desc.vPatrolEndPosition = RayResult.vHitpos;

	const uint64_t iPlacementId = AddPlacement(Desc);
	m_iSelectedIndex = static_cast<int32_t>(m_Placements.size()) - 1;
	if (m_bSpawnOnPick && HasSpawnCallback()) Spawn(iPlacementId);
}

uint64_t CNpcPlacementManager::AddPlacement(const NPC_PLACEMENT_DESC& Desc)
{
	NPC_PLACEMENT_DESC NewDesc = Desc;
	if (NewDesc.iPlacementId == 0 || std::any_of(m_Placements.begin(), m_Placements.end(),
		[&](const NPC_PLACEMENT_DESC& Existing) { return Existing.iPlacementId == NewDesc.iPlacementId; }))
		NewDesc.iPlacementId = AllocatePlacementId();
	else
		m_iNextPlacementId = std::max(m_iNextPlacementId, NewDesc.iPlacementId + 1);

	m_Placements.push_back(std::move(NewDesc));
	return m_Placements.back().iPlacementId;
}

_bool CNpcPlacementManager::RemovePlacement(uint64_t iPlacementId)
{
	const auto Iter = std::find_if(m_Placements.begin(), m_Placements.end(),
		[&](const NPC_PLACEMENT_DESC& Desc) { return Desc.iPlacementId == iPlacementId; });
	if (Iter == m_Placements.end()) return false;

	const size_t iRemovedIndex = static_cast<size_t>(std::distance(m_Placements.begin(), Iter));
	m_Placements.erase(Iter);
	if (m_Placements.empty()) m_iSelectedIndex = -1;
	else if (m_iSelectedIndex >= static_cast<int32_t>(m_Placements.size()))
		m_iSelectedIndex = static_cast<int32_t>(m_Placements.size()) - 1;
	else if (m_iSelectedIndex > static_cast<int32_t>(iRemovedIndex)) --m_iSelectedIndex;
	return true;
}

void CNpcPlacementManager::ClearPlacements()
{
	m_Placements.clear();
	m_LastResults.clear();
	m_iSelectedIndex = -1;
}

void CNpcPlacementManager::RegisterNpcOption(const _string& sDisplayName, const NPC_PLACEMENT_DESC& Desc)
{
	if (sDisplayName.empty())
		return;

	const auto Iter = std::ranges::find_if(m_NpcOptions,
		[&](const auto& Option) { return Option.first == sDisplayName; });
	if (Iter != m_NpcOptions.end())
		Iter->second = Desc;
	else
		m_NpcOptions.emplace_back(sDisplayName, Desc);
}

void CNpcPlacementManager::RegisterNpcSkeletonOption(
	const _string& sPrototypeTag, const _string& sDisplayName,
	const _string& sModelGroupTag, const _string& sModelResourceTag)
{
	if (sPrototypeTag.empty() || sDisplayName.empty() || sModelGroupTag.empty() || sModelResourceTag.empty())
		return;

	const auto Iter = std::ranges::find_if(m_NpcSkeletonOptions, [&](const NPC_SKELETON_OPTION& Option)
	{
		return Option.sPrototypeTag == sPrototypeTag && Option.sDisplayName == sDisplayName;
	});
	NPC_SKELETON_OPTION Option{ sPrototypeTag, sDisplayName, sModelGroupTag, sModelResourceTag };
	if (Iter != m_NpcSkeletonOptions.end())
		*Iter = std::move(Option);
	else
		m_NpcSkeletonOptions.emplace_back(std::move(Option));
}

void CNpcPlacementManager::RegisterBehaviorOption(
	const _string& sDisplayName,
	const _string& sBehaviorMajorTag,
	const _string& sBehaviorMinorTag)
{
	if (sDisplayName.empty() || sBehaviorMajorTag.empty() || sBehaviorMinorTag.empty())
		return;

	const auto Iter = std::ranges::find_if(m_BehaviorOptions, [&](const BEHAVIOR_OPTION& Option)
	{
		return Option.sDisplayName == sDisplayName;
	});
	BEHAVIOR_OPTION Option{ sDisplayName, sBehaviorMajorTag, sBehaviorMinorTag };
	if (Iter != m_BehaviorOptions.end())
		*Iter = std::move(Option);
	else
		m_BehaviorOptions.emplace_back(std::move(Option));
}

void CNpcPlacementManager::ClearNpcOptions()
{
	m_NpcOptions.clear();
	m_NpcSkeletonOptions.clear();
	m_BehaviorOptions.clear();
	ClearSpawnCallback();
}

NPC_PLACEMENT_RESULT CNpcPlacementManager::Spawn(uint64_t iPlacementId)
{
	const auto Iter = std::find_if(m_Placements.begin(), m_Placements.end(),
		[&](const NPC_PLACEMENT_DESC& Desc) { return Desc.iPlacementId == iPlacementId; });
	NPC_PLACEMENT_RESULT Result = Iter == m_Placements.end()
		? NPC_PLACEMENT_RESULT{ iPlacementId, false, {}, "Placement not found." }
		: SpawnPlacement(*Iter);
	m_LastResults = { Result };
	return Result;
}

const std::vector<NPC_PLACEMENT_RESULT>& CNpcPlacementManager::SpawnAll()
{
	m_LastResults.clear();
	m_LastResults.reserve(m_Placements.size());
	for (const auto& Desc : m_Placements) m_LastResults.push_back(SpawnPlacement(Desc));
	return m_LastResults;
}

HRESULT CNpcPlacementManager::Save(const _string& sFilePath) const
{
	if (sFilePath.empty()) return E_INVALIDARG;

	for (const auto& Desc : m_Placements)
		if (!ValidatePlacement(Desc).empty()) return E_FAIL;

	const std::filesystem::path FilePath{ sFilePath };
	if (const auto ParentPath = FilePath.parent_path(); !ParentPath.empty())
	{
		std::error_code ErrorCode{};
		std::filesystem::create_directories(ParentPath, ErrorCode);
		if (ErrorCode) return E_FAIL;
	}

	NPC_PLACEMENT_FILE File{};
	File.Placements = m_Placements;
	return CGameInstance::Get().JsonSerialize(sFilePath, File, "NpcPlacements");
}

HRESULT CNpcPlacementManager::Load(const _string& sFilePath)
{
	if (sFilePath.empty()) return E_INVALIDARG;

	NPC_PLACEMENT_FILE File{};
	if (FAILED(CGameInstance::Get().JsonDeSerialize(sFilePath, File, "NpcPlacements"))) return E_FAIL;
	if (File.iVersion != 1) return E_FAIL;

	std::unordered_set<uint64_t> PlacementIds{};
	uint64_t iNextPlacementId = 1;
	for (const auto& Desc : File.Placements)
	{
		if (Desc.iPlacementId == 0 || !PlacementIds.emplace(Desc.iPlacementId).second) return E_FAIL;
		if (!ValidatePlacement(Desc).empty()) return E_FAIL;
		iNextPlacementId = std::max(iNextPlacementId, Desc.iPlacementId + 1);
	}

	m_Placements = std::move(File.Placements);
	m_LastResults.clear();
	m_iNextPlacementId = iNextPlacementId;
	m_iSelectedIndex = m_Placements.empty() ? -1 : 0;
	return S_OK;
}

void CNpcPlacementManager::DrawPlacementEditor(NPC_PLACEMENT_DESC& Desc, size_t iIndex)
{
	auto* pCam = CGameInstance::Get().GetActiveCamera();
	if (nullptr == pCam) return;
	_float3 vCamPos = pCam->GetTransform().GetPosition();
	ImGui::PushID(static_cast<int32_t>(Desc.iPlacementId));
	ImGui::PushItemWidth(420.f);
	ImGui::Text("Placement ID: %llu", static_cast<unsigned long long>(Desc.iPlacementId));

	_string sSelectedNpc{};
	for (const auto& [Name, Option] : m_NpcOptions)
	{
		if (Desc.sPrototypeGroupTag == Option.sPrototypeGroupTag &&
			Desc.sPrototypeTag == Option.sPrototypeTag)
		{
			sSelectedNpc = Name;
			break;
		}
	}

	std::vector<_string> NpcNames{};
	NpcNames.reserve(m_NpcOptions.size());
	for (const auto& [Name, Option] : m_NpcOptions)
		NpcNames.emplace_back(Name);
	if (DrawStringSelection("NPC", sSelectedNpc, NpcNames))
	{
		const auto Iter = std::ranges::find_if(m_NpcOptions,
			[&](const auto& Option) { return Option.first == sSelectedNpc; });
		if (Iter != m_NpcOptions.end())
		{
			const uint64_t iPlacementId = Desc.iPlacementId;
			const _float3 vPosition = Desc.vPosition;
			const _float3 vRotation = Desc.vRotation;
			const _float3 vScale = Desc.vScale;
			const _float3 vPatrolStart = Desc.vPatrolStartPosition;
			const _float3 vPatrolEnd = Desc.vPatrolEndPosition;
			Desc = Iter->second;
			Desc.iPlacementId = iPlacementId;
			Desc.vPosition = vPosition;
			Desc.vRotation = vRotation;
			Desc.vScale = vScale;
			Desc.vPatrolStartPosition = vPatrolStart;
			Desc.vPatrolEndPosition = vPatrolEnd;
		}
	}

	ImGui::Text("Prototype: %s", Desc.sPrototypeTag.empty() ? "<Select NPC>" : Desc.sPrototypeTag.c_str());
	ImGui::Text("Layer: %s", Desc.sLayerTag.empty() ? "-" : Desc.sLayerTag.c_str());

	_string sSelectedSkeleton{};
	std::vector<_string> SkeletonNames{};
	for (const auto& Option : m_NpcSkeletonOptions)
	{
		if (Option.sPrototypeTag != Desc.sPrototypeTag)
			continue;
		SkeletonNames.emplace_back(Option.sDisplayName);
		if (Option.sModelGroupTag == Desc.sModelGroupTag &&
			Option.sModelResourceTag == Desc.sModelResourceTag)
			sSelectedSkeleton = Option.sDisplayName;
	}
	if (DrawStringSelection("Skeleton", sSelectedSkeleton, SkeletonNames))
	{
		const auto Iter = std::ranges::find_if(m_NpcSkeletonOptions, [&](const NPC_SKELETON_OPTION& Option)
		{
			return Option.sPrototypeTag == Desc.sPrototypeTag && Option.sDisplayName == sSelectedSkeleton;
		});
		if (Iter != m_NpcSkeletonOptions.end())
		{
			Desc.sModelGroupTag = Iter->sModelGroupTag;
			Desc.sModelResourceTag = Iter->sModelResourceTag;
		}
	}
	ImGui::Text("Model Resource: %s", Desc.sModelResourceTag.empty() ? "-" : Desc.sModelResourceTag.c_str());

	_string sSelectedBehavior{};
	std::vector<_string> BehaviorNames{};
	BehaviorNames.reserve(m_BehaviorOptions.size());
	for (const auto& Option : m_BehaviorOptions)
	{
		BehaviorNames.emplace_back(Option.sDisplayName);
		if (Option.sBehaviorMajorTag == Desc.sBehaviorMajorTag &&
			Option.sBehaviorMinorTag == Desc.sBehaviorMinorTag)
			sSelectedBehavior = Option.sDisplayName;
	}
	if (DrawStringSelection("Behavior Tree", sSelectedBehavior, BehaviorNames))
	{
		const auto Iter = std::ranges::find_if(m_BehaviorOptions, [&](const BEHAVIOR_OPTION& Option)
		{
			return Option.sDisplayName == sSelectedBehavior;
		});
		if (Iter != m_BehaviorOptions.end())
		{
			Desc.sBehaviorMajorTag = Iter->sBehaviorMajorTag;
			Desc.sBehaviorMinorTag = Iter->sBehaviorMinorTag;
		}
	}
	DrawStringInput("Behavior Group", Desc.sBehaviorMajorTag);
	DrawStringInput("Behavior Name", Desc.sBehaviorMinorTag);

	int32_t iRuntimeType = static_cast<int32_t>(Desc.eRuntimeType);
	if (ImGui::Combo(
		"Runtime Type",
		&iRuntimeType,
		RUNTIME_TYPE_NAMES.data(),
		static_cast<int32_t>(RUNTIME_TYPE_NAMES.size())))
		Desc.eRuntimeType = static_cast<NPC_RUNTIME_TYPE>(iRuntimeType);
	ImGui::DragFloat3("Position", &Desc.vPosition.x, 0.1f); ImGui::SameLine();
	if (ImGui::Button("CamPosTo Pos"))
		Desc.vPosition = vCamPos;

	ImGui::DragFloat3("Rotation", &Desc.vRotation.x, 0.5f);
	ImGui::DragFloat3("Scale", &Desc.vScale.x, 0.01f, 0.001f, 100.f);

	ImGui::DragFloat3("Patrol Start", &Desc.vPatrolStartPosition.x, 0.1f); ImGui::SameLine();
	if (ImGui::Button("CamPosTo Patrol Start"))
		Desc.vPatrolStartPosition = vCamPos;

	ImGui::DragFloat3("Patrol End", &Desc.vPatrolEndPosition.x, 0.1f); ImGui::SameLine();
	if (ImGui::Button("CamPosTo Patrol End")) 
		Desc.vPatrolEndPosition = vCamPos;
	ImGui::DragFloat("Speed", &Desc.fSpeed, 0.1f); ImGui::SameLine();

	ImGui::Checkbox("Cast Shadow", &Desc.bCastShadow);
	ImGui::DragFloat("Visible Distance", &Desc.fVisibleDistance, 1.f, 0.f, 100000.f);
	ImGui::DragFloat("Animation Update Distance", &Desc.fAnimationUpdateDistance, 1.f, 0.f, 100000.f);
	ImGui::DragFloat("AI Update Distance", &Desc.fAIUpdateDistance, 1.f, 0.f, 100000.f);

	if (Desc.eRuntimeType == NPC_RUNTIME_TYPE::GPU_CROWD_AMBIENT)
	{
		ImGui::DragScalar("Crowd Count", ImGuiDataType_U32, &Desc.iCrowdCount, 1.f);
		Desc.iCrowdCount = std::max(1u, Desc.iCrowdCount);
		ImGui::DragFloat("Crowd Radius", &Desc.fCrowdRadius, 0.1f, 0.f, 100000.f);
		ImGui::DragScalar("Random Seed", ImGuiDataType_U32, &Desc.iRandomSeed, 1.f);
		Desc.bCastShadow = false;
	}

	if (ImGui::Button("Spawn Selected") && HasSpawnCallback()) Spawn(Desc.iPlacementId);
	ImGui::SameLine();
	if (ImGui::Button("Duplicate"))
	{
		NPC_PLACEMENT_DESC Copy = Desc;
		Copy.iPlacementId = 0;
		AddPlacement(Copy);
		m_iSelectedIndex = static_cast<int32_t>(m_Placements.size()) - 1;
		ImGui::PopItemWidth();
		ImGui::PopID();
		return;
	}
	ImGui::SameLine();
	if (ImGui::Button("Remove"))
	{
		const uint64_t iPlacementId = Desc.iPlacementId;
		ImGui::PopItemWidth();
		ImGui::PopID();
		RemovePlacement(iPlacementId);
		return;
	}
	ImGui::TextDisabled("Editor index: %zu", iIndex);
	ImGui::PopItemWidth();
	ImGui::PopID();
}

NPC_PLACEMENT_RESULT CNpcPlacementManager::SpawnPlacement(const NPC_PLACEMENT_DESC& Desc) const
{
	if (!m_SpawnCallback) return { Desc.iPlacementId, false, {}, "Client spawn adapter is not connected." };
	if (const _string Error = ValidatePlacement(Desc); !Error.empty())
		return { Desc.iPlacementId, false, {}, Error };

	NPC_PLACEMENT_RESULT Result = m_SpawnCallback(Desc);
	Result.iPlacementId = Desc.iPlacementId;
	if (Result.bSucceeded && Result.hObject == CHandle{})
	{
		Result.bSucceeded = false;
		Result.sMessage = "Spawn callback returned success with an invalid handle.";
	}
	return Result;
}

_string CNpcPlacementManager::ValidatePlacement(const NPC_PLACEMENT_DESC& Desc) const
{
	if (Desc.sPrototypeGroupTag.empty()) return "Prototype group tag is empty.";
	if (Desc.sPrototypeTag.empty()) return "Prototype tag is empty.";
	if (Desc.sLayerTag.empty()) return "Layer tag is empty.";
	if (Desc.vScale.x <= 0.f || Desc.vScale.y <= 0.f || Desc.vScale.z <= 0.f)
		return "Scale must be greater than zero.";
	if (Desc.fVisibleDistance < 0.f || Desc.fAnimationUpdateDistance < 0.f || Desc.fAIUpdateDistance < 0.f)
		return "Update distances cannot be negative.";
	if (Desc.eRuntimeType == NPC_RUNTIME_TYPE::GPU_CROWD_AMBIENT && Desc.iCrowdCount == 0)
		return "GPU crowd count must be greater than zero.";
	return {};
}

uint64_t CNpcPlacementManager::AllocatePlacementId()
{
	while (std::any_of(m_Placements.begin(), m_Placements.end(),
		[&](const NPC_PLACEMENT_DESC& Desc) { return Desc.iPlacementId == m_iNextPlacementId; }))
		++m_iNextPlacementId;
	return m_iNextPlacementId++;
}

UPtr<CNpcPlacementManager> CNpcPlacementManager::Create()
{
	return UPtr<CNpcPlacementManager>(new CNpcPlacementManager{});
}
