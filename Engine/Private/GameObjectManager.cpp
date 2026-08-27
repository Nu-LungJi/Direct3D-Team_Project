#include "pch.h"
#include "GameObjectManager.h"
#include "GameInstance.h"
#include "GameObject.h"
//#include "Helper.h"

NS_USING(Engine)

CGameObjectManager::CGameObjectManager()
{
}
CGameObjectManager::~CGameObjectManager()
{
}

void CGameObjectManager::UpdateGUI()
{
	if (!ImGui::Begin("GameObject_Manager"))
	{
		ImGui::End();
		return;
	}

	ImGui::Checkbox(
		"Tracy Detailed Object Profiling",
		&m_bTracyDetailedObjectProfiling);

	if (ImGui::TreeNodeEx(
		"Managed Update Loop Counts",
		ImGuiTreeNodeFlags_DefaultOpen))
	{
		// 마지막 update view 재구축 때 마스크로 분류된 후보 수다.
		// PendingDestroy/ManagedUpdateEnabled는 dispatch 직전에 거르므로 실제 호출 횟수와는 다를 수 있다.
		const size_t iRegisteredObjectCount = std::count_if(
			m_Objects.begin(),
			m_Objects.end(),
			[](const CSlot<CGameObject>& Slot)
			{
				return Slot.IsOccupied();
			});
		ImGui::Text("Priority Update: %zu / %zu", m_PriorityUpdateObjects.size(), iRegisteredObjectCount);
		ImGui::Text("Fixed Update: %zu / %zu", m_FixedUpdateObjects.size(), iRegisteredObjectCount);
		ImGui::Text("Update: %zu / %zu", m_UpdateObjects.size(), iRegisteredObjectCount);
		ImGui::Text("Late Update: %zu / %zu", m_LateUpdateObjects.size(), iRegisteredObjectCount);
		ImGui::TextDisabled("Configured loop candidates / registered objects");
		ImGui::TreePop();
	}
	ImGui::Separator();

	std::vector<uint32_t> vecFreeSlotReferences(m_Objects.size(), 0);
	size_t iInvalidFreeSlotCount = 0;
	for (const size_t iFreeSlotIndex : m_FreeSlots)
	{
		if (iFreeSlotIndex >= m_Objects.size())
		{
			++iInvalidFreeSlotCount;
			continue;
		}

		++vecFreeSlotReferences[iFreeSlotIndex];
		if (m_Objects[iFreeSlotIndex].IsOccupied() ||
			vecFreeSlotReferences[iFreeSlotIndex] > 1)
		{
			++iInvalidFreeSlotCount;
		}
	}

	std::string sFreeSlotsLabel = "FreeSlots (" + std::to_string(m_FreeSlots.size()) +
		" Entries";
	if (iInvalidFreeSlotCount > 0)
	{
		sFreeSlotsLabel += ", " + std::to_string(iInvalidFreeSlotCount) + " Invalid";
	}
	sFreeSlotsLabel += ")###FreeSlots";

	if (ImGui::TreeNode(sFreeSlotsLabel.c_str()))
	{
		for (const size_t iFreeSlotIndex : m_FreeSlots)
		{
			if (iFreeSlotIndex >= m_Objects.size())
			{
				ImGui::TextColored(
					ImVec4{ 1.f, 0.35f, 0.35f, 1.f },
					"[Invalid] Index: %zu | Slot: Out Of Range",
					iFreeSlotIndex);
				continue;
			}

			const auto& slot = m_Objects[iFreeSlotIndex];
			const uint32_t iReferenceCount = vecFreeSlotReferences[iFreeSlotIndex];
			if (slot.IsOccupied() || iReferenceCount > 1)
			{
				ImGui::TextColored(
					ImVec4{ 1.f, 0.35f, 0.35f, 1.f },
					"[Invalid] Index: %zu | Gen: %u | Slot: %s | References: %u",
					iFreeSlotIndex,
					slot.GetGeneration(),
					slot.IsOccupied() ? "Occupied" : "Empty",
					iReferenceCount);
			}
			else
			{
				ImGui::Text(
					"Index: %zu | Gen: %u | Slot: Empty",
					iFreeSlotIndex,
					slot.GetGeneration());
			}
		}

		ImGui::TreePop();
	}

	size_t iOccupiedSlotCount = 0;
	size_t iEmptySlotCount = 0;
	size_t iUntrackedEmptySlotCount = 0;
	for (size_t i = 0; i < m_Objects.size(); ++i)
	{
		if (m_Objects[i].IsOccupied())
		{
			++iOccupiedSlotCount;
		}
		else
		{
			++iEmptySlotCount;
			if (vecFreeSlotReferences[i] == 0)
				++iUntrackedEmptySlotCount;
		}
	}

	std::string sSlotsLabel = "Slots (" + std::to_string(m_Objects.size()) + " Total, " +
		std::to_string(iOccupiedSlotCount) + " Occupied, " +
		std::to_string(iEmptySlotCount) + " Empty";
	if (iUntrackedEmptySlotCount > 0)
	{
		sSlotsLabel += ", " + std::to_string(iUntrackedEmptySlotCount) + " Untracked";
	}
	sSlotsLabel += ")###Slots";

	if (ImGui::TreeNode(sSlotsLabel.c_str()))
	{
		for (size_t i = 0; i < m_Objects.size(); ++i)
		{
			auto& slot = m_Objects[i];
			const bool bOccupied = slot.IsOccupied();
			const uint32_t iFreeReferenceCount = vecFreeSlotReferences[i];
			const bool bInvalidState =
				(bOccupied && iFreeReferenceCount > 0) ||
				(!bOccupied && iFreeReferenceCount != 1);
			const bool bManagedUpdateDisabled =
				bOccupied && !slot.Get()->IsManagedUpdateEnabled();

			std::string sSlotLabel = bOccupied ? "[Occupied] " : "[Empty] ";
			sSlotLabel += "Index: " + std::to_string(i) +
				" | Gen: " + std::to_string(slot.GetGeneration());
			if (bOccupied)
			{
				sSlotLabel += " | Type: " + std::string{ slot.Get()->GetTypeString() } +
					" | Tag: " + std::string{ slot.Get()->GetObjectTag() };
			}
			else
			{
				sSlotLabel += iFreeReferenceCount == 1 ?
					" | FreeList: Registered" :
					" | FreeList: Invalid";
			}
			sSlotLabel += "###Slot_" + std::to_string(i);

			if (bInvalidState)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1.f, 0.35f, 0.35f, 1.f });
			else if (bManagedUpdateDisabled)
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

			const bool bSlotOpened = ImGui::TreeNode(sSlotLabel.c_str());

			if (bInvalidState || bManagedUpdateDisabled)
				ImGui::PopStyleColor();

			if (bSlotOpened)
			{
				ImGui::Text("Slot Index: %zu", i);
				ImGui::Text("Slot Generation: %u", slot.GetGeneration());
				ImGui::Text("FreeList References: %u", iFreeReferenceCount);

				if (bOccupied)
				{
					CGameObject* pObj = slot.Get();
					const CHandle hObject = pObj->GetHandle();
					const std::string_view sObjectType = pObj->GetTypeString();
					const std::string_view sObjectTag = pObj->GetObjectTag();
					ImGui::Text(
						"Object Type: %.*s",
						static_cast<int>(sObjectType.size()),
						sObjectType.data());
					ImGui::Text(
						"Object Tag: %.*s",
						static_cast<int>(sObjectTag.size()),
						sObjectTag.data());
					ImGui::Text(
						"Object Handle: Index %zu | Gen %u",
						hObject.GetIndex(),
						hObject.GetGeneration());
					ImGui::Text("Pending Destroy: %s", pObj->GetPendingDestroy() ? "True" : "False");
					ImGui::Separator();
					pObj->UpdateGUI();
				}

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

	const bool bLayerFilterActive =
		m_bGUIEnableSearchInput && m_GUISearchFilter[0] != '\0';
	size_t iTotalValidLayerObjectCount = 0;
	size_t iTotalInvalidLayerHandleCount = 0;
	size_t iFilteredLayerItemCount = 0;
	for (const auto& [sLayerName, vecHandles] : m_Layers)
	{
		for (const auto& handle : vecHandles)
		{
			if (auto* pObj = GetGameObjectByHandle(handle))
			{
				++iTotalValidLayerObjectCount;
				if (MatchesLayerObjectFilter(sLayerName, pObj))
					++iFilteredLayerItemCount;
			}
			else
			{
				++iTotalInvalidLayerHandleCount;
				if (m_bGUIShowInvalidLayerHandles &&
					(MatchesGUIFilter(sLayerName) ||
						MatchesGUIFilter(GetInvalidLayerHandleDebugText(handle))))
				{
					++iFilteredLayerItemCount;
				}
			}
		}
	}

	std::string sLayersLabel = "Layers (";
	if (bLayerFilterActive)
	{
		sLayersLabel += std::to_string(iFilteredLayerItemCount) + " Matches, ";
	}
	sLayersLabel += std::to_string(iTotalValidLayerObjectCount) + " Objects, " +
		std::to_string(m_Layers.size()) + " Layers";
	if (m_bGUIShowInvalidLayerHandles)
	{
		sLayersLabel += ", " + std::to_string(iTotalInvalidLayerHandleCount) + " Invalid";
	}
	sLayersLabel += ")###Layers";

	if (ImGui::TreeNode(sLayersLabel.c_str()))
	{
		std::optional<std::string> sLayerToDelete{};

		ImGui::Checkbox("Enable Search Input##Layers", &m_bGUIEnableSearchInput);
		ImGui::SetNextItemWidth(-1);
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !m_bGUIEnableSearchInput);
		ImGui::PushStyleVar(
			ImGuiStyleVar_Alpha,
			m_bGUIEnableSearchInput ? ImGui::GetStyle().Alpha :
			ImGui::GetStyle().Alpha * 0.5f);
		ImGui::InputTextWithHint(
			"##layer_search",
			"Search layer or object...",
			m_GUISearchFilter,
			sizeof(m_GUISearchFilter));
		ImGui::PopStyleVar();
		ImGui::PopItemFlag();
		ImGui::Checkbox("Show Invalid Handles", &m_bGUIShowInvalidLayerHandles);
		ImGui::Separator();

		for (const auto& [sLayerName, vecHandles] : m_Layers)
		{
			const bool bLayerNameMatched = MatchesGUIFilter(sLayerName);
			size_t iValidObjectCount = 0;
			size_t iInvalidHandleCount = 0;
			size_t iMatchedItemCount = 0;

			for (const auto& handle : vecHandles)
			{
				if (auto* pObj = GetGameObjectByHandle(handle))
				{
					++iValidObjectCount;
					if (bLayerNameMatched || MatchesLayerObjectFilter(sLayerName, pObj))
						++iMatchedItemCount;
				}
				else
				{
					++iInvalidHandleCount;
					if (m_bGUIShowInvalidLayerHandles &&
						(bLayerNameMatched ||
							MatchesGUIFilter(GetInvalidLayerHandleDebugText(handle))))
					{
						++iMatchedItemCount;
					}
				}
			}

			if (bLayerFilterActive && iMatchedItemCount == 0 && !bLayerNameMatched)
				continue;

			std::string sLayerLabel = sLayerName + " (";
			if (bLayerFilterActive)
			{
				sLayerLabel += std::to_string(iMatchedItemCount) + " Matches, ";
			}
			sLayerLabel += std::to_string(iValidObjectCount) + " Objects";
			if (m_bGUIShowInvalidLayerHandles)
			{
				sLayerLabel += ", " + std::to_string(iInvalidHandleCount) + " Invalid";
			}
			sLayerLabel += ")###Layer_" + sLayerName;

			ImGui::PushID(sLayerName.c_str());
			if (ImGui::SmallButton("Delete Layer"))
			{
				sLayerToDelete = sLayerName;
			}
			ImGui::PopID();
			ImGui::SameLine();
			const bool bLayerOpened = ImGui::TreeNode(sLayerLabel.c_str());

			if (bLayerOpened)
			{
				for (const auto& handle : vecHandles)
				{
					if (auto* pObj = GetGameObjectByHandle(handle))
					{
						if (bLayerFilterActive && !bLayerNameMatched &&
							!MatchesLayerObjectFilter(sLayerName, pObj))
						{
							continue;
						}

						std::string sObjectLabel = GetGameObjectDebugLabel(pObj);
						sObjectLabel += "###LayerObject_" + std::to_string(handle.GetIndex()) + "_" +
							std::to_string(handle.GetGeneration());

						const bool bManagedUpdateDisabled = !pObj->IsManagedUpdateEnabled();
						if (bManagedUpdateDisabled)
							ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

						const bool bObjectOpened = ImGui::TreeNode(sObjectLabel.c_str());

						if (bManagedUpdateDisabled)
							ImGui::PopStyleColor();

						if (bObjectOpened)
						{
							pObj->UpdateGUI();
							ImGui::TreePop();
						}
					}
					else if (m_bGUIShowInvalidLayerHandles)
					{
						const std::string sInvalidHandleText =
							GetInvalidLayerHandleDebugText(handle);
						if (bLayerFilterActive && !bLayerNameMatched &&
							!MatchesGUIFilter(sInvalidHandleText))
						{
							continue;
						}

						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1.f, 0.35f, 0.35f, 1.f });
						ImGui::TextUnformatted(sInvalidHandleText.c_str());
						ImGui::PopStyleColor();
					}
				}

				ImGui::TreePop();
			}
		}

		// 레이어 컨테이너를 순회하는 동안에는 삭제하지 않고, GUI 그리기가 끝난 뒤 처리한다.
		if (sLayerToDelete)
		{
			DelLayer(*sLayerToDelete);
		}

		ImGui::TreePop();
	}

	ImGui::End();
}

bool CGameObjectManager::MatchesGUIFilter(std::string_view sText) const
{
	if (!m_bGUIEnableSearchInput || m_GUISearchFilter[0] == '\0')
		return true;

	const std::string_view sFilter{ m_GUISearchFilter };
	const auto iter = std::search(
		sText.begin(),
		sText.end(),
		sFilter.begin(),
		sFilter.end(),
		[](char chLeft, char chRight)
		{
			return std::tolower(static_cast<unsigned char>(chLeft)) ==
				std::tolower(static_cast<unsigned char>(chRight));
		});

	return iter != sText.end();
}

bool CGameObjectManager::MatchesLayerObjectFilter(
	std::string_view sLayerName,
	CGameObject* pObj) const
{
	if (MatchesGUIFilter(sLayerName))
		return true;

	return MatchesGUIFilter(GetGameObjectDebugLabel(pObj));
}

std::string CGameObjectManager::GetGameObjectDebugLabel(CGameObject* pObj) const
{
	const CHandle handle = pObj->GetHandle();
	return "Index: " + std::to_string(handle.GetIndex()) +
		" | Type: " + std::string{ pObj->GetTypeString() } +
		" | Tag: " + std::string{ pObj->GetObjectTag() };
}

std::string CGameObjectManager::GetInvalidLayerHandleDebugText(const CHandle& handle) const
{
	const size_t iIndex = handle.GetIndex();
	std::string sText = "[Invalid] Index: " + std::to_string(iIndex) +
		" | Stored Gen: " + std::to_string(handle.GetGeneration());

	if (iIndex >= m_Objects.size())
	{
		sText += " | Slot: Out Of Range";
		return sText;
	}

	const auto& slot = m_Objects[iIndex];
	sText += " | Current Gen: " + std::to_string(slot.GetGeneration());
	sText += slot.IsOccupied() ? " | Slot: Occupied" : " | Slot: Empty";
	return sText;
}

void CGameObjectManager::FrameStart()
{
	ZoneScopedN("CGameObjectManager_FrameStart");

	// 객체 추가·삭제로 등록 목록이 바뀐 경우에만 단계별 view를 갱신한다.
	if (!m_bUpdateViewsDirty)
	{
		return;
	}

	{
		ZoneScopedN("CGameObjectManager_RebuildUpdateViews");

		m_PriorityUpdateObjects.clear();
		m_FixedUpdateObjects.clear();
		m_UpdateObjects.clear();
		m_LateUpdateObjects.clear();

		constexpr uint8_t iPriorityMask =
			static_cast<uint8_t>(GAMEOBJECT_UPDATE_LOOP::PRIORITY);
		constexpr uint8_t iFixedMask =
			static_cast<uint8_t>(GAMEOBJECT_UPDATE_LOOP::FIXED);
		constexpr uint8_t iUpdateMask =
			static_cast<uint8_t>(GAMEOBJECT_UPDATE_LOOP::UPDATE);
		constexpr uint8_t iLateMask =
			static_cast<uint8_t>(GAMEOBJECT_UPDATE_LOOP::LATE);

		// 정렬된 레이어와 레이어 내부 Handle 순서를 유지하면서 각 단계의 비소유 포인터 view를 만든다.
		// 이후 hot loop는 전체 객체의 마스크를 매번 검사하거나 빈 가상 함수를 호출하지 않는다.
		for (const auto& [_, Layer] : m_Layers)
		{
			for (const CHandle& hObject : Layer)
			{
				CGameObject* pObject = GetGameObjectByHandle(hObject);
				if (pObject == nullptr)
					continue;

				const uint8_t iUpdateLoopMask =
					static_cast<uint8_t>(pObject->GetUpdateLoopMask());

				if ((iUpdateLoopMask & iPriorityMask) != 0)
					m_PriorityUpdateObjects.push_back(pObject);
				if ((iUpdateLoopMask & iFixedMask) != 0)
					m_FixedUpdateObjects.push_back(pObject);
				if ((iUpdateLoopMask & iUpdateMask) != 0)
					m_UpdateObjects.push_back(pObject);
				if ((iUpdateLoopMask & iLateMask) != 0)
					m_LateUpdateObjects.push_back(pObject);
			}
		}
	}

	m_bUpdateViewsDirty = false;
}

void CGameObjectManager::FrameEnd()
{
	ZoneScopedN("CGameObjectManager_FrameEnd");

	// 파괴/레이어 리셋 요청이 없는 일반 프레임은 컨테이너를 전혀 순회하지 않는다.
	if (!m_bBatchResetPending && m_PendingDestroyHandles.empty())
		return;

	const _bool bProcessBatchReset = m_bBatchResetPending;
	std::unordered_set<std::string> resetTargetLayers{};
	if (bProcessBatchReset)
	{
		// 이번 FrameEnd가 책임질 레이어 이름을 떼어낸다.
		// 파괴 중 새 reset 요청이 들어오면 새 집합에 쌓여 다음 FrameEnd에서 레이어 컨테이너까지 정리된다.
		m_bBatchResetPending = false;
		resetTargetLayers = std::move(m_PendingResetTargetLayers);
		m_PendingResetTargetLayers.clear();
	}

	// 실제 슬롯 파괴와 각 레이어의 Handle 제거는 모든 update 단계가 끝난 이 시점에만 수행한다.
	const _bool bAnyObjectDestroyed = DestroyQueuedPendingObjects();

	if (bAnyObjectDestroyed)
	{
		// 단계별 view에는 비소유 raw pointer가 있으므로 다음 FrameStart 전에 반드시 다시 만든다.
		m_bUpdateViewsDirty = true;
	}

	// 일반 개별 파괴는 빈 레이어 이름을 유지한다. 명시적 reset/del 대상 레이어만 비었을 때 제거한다.
	if (bProcessBatchReset &&
		RemoveEmptyResetTargetLayers(resetTargetLayers))
	{
		SortLayer();
	}
}

std::optional<CHandle> CGameObjectManager::AllocateHandleSlot()
{
	// slot index를 공유하는 병렬 메타데이터는 항상 m_Objects와 같은 길이여야 한다.
	assert(m_Objects.size() == m_ObjectLayerLocations.size());
	assert(m_Objects.size() == m_PendingLayerRemoveGenerationKeys.size());

	CHandle objectHandle{};
	if (m_FreeSlots.empty())
	{
		// 새 slot을 만들 때 위치 캐시와 삭제 표식도 같은 인덱스로 동시에 확장한다.
		size_t idx = m_Objects.size();
		uint32_t gen = 0;

		objectHandle = CHandle{ idx, gen };
		m_Objects.push_back({});
		m_ObjectLayerLocations.push_back({});
		m_PendingLayerRemoveGenerationKeys.push_back(0);
		//m_FreeSlots.push_back(idx);
	}
	else
	{
		// CSlot::Reset이 증가시킨 현재 generation으로 Handle을 만들고 이전 사용의 scratch를 지운다.
		size_t emptyIdx = m_FreeSlots.back();
		m_FreeSlots.pop_back();

		size_t idx = emptyIdx;
		uint32_t gen = m_Objects[emptyIdx].GetGeneration();

		m_ObjectLayerLocations[emptyIdx] = {};
		m_PendingLayerRemoveGenerationKeys[emptyIdx] = 0;
		objectHandle = CHandle{ idx, gen };
	}

	assert(m_Objects.size() == m_ObjectLayerLocations.size());
	assert(m_Objects.size() == m_PendingLayerRemoveGenerationKeys.size());
	return objectHandle;
}

std::optional<CHandle> CGameObjectManager::AddGameObjectToLayer(const StringID& siProtoGroupTag, const StringID& siPrototypeTag, std::string_view sLayerName, void* pArg)
{
	const _bool bTracyConnected =
		m_bTracyDetailedObjectProfiling && TracyIsConnected;
	ZoneNamedN(
		tObjectZone,
		"CGameObjectManager_AddGameObjectToLayer",
		bTracyConnected);

	auto pDesc = static_cast<CGameObject::GAMEOBJECT_DESC*>(pArg);
	auto allocHandle = AllocateHandleSlot();
	if (!allocHandle)
	{
		return std::nullopt;
	}
	pDesc->__handle = allocHandle.value();

	auto pProto = CGameInstance::Get().ClonePrototype(siProtoGroupTag, siPrototypeTag, pArg);
	auto pGameObject = static_uptr_cast<CGameObject>(std::move(pProto));
	if (!pGameObject)
	{
		m_Objects[allocHandle.value().GetIndex()].Reset();
		m_FreeSlots.push_back(allocHandle.value().GetIndex());
		return std::nullopt;
	}

	auto objHandle = pGameObject->GetHandle();
	if (bTracyConnected)
	{
		const std::string sDebugLabel = GetGameObjectDebugLabel(pGameObject.get());
		ZoneNameV(tObjectZone, sDebugLabel.data(), sDebugLabel.size());
	}

	CGameObject* pRegisteredObject = pGameObject.get();

	//auto iter = std::find(m_FreeSlots.begin(), m_FreeSlots.end(), objHandle.GetIndex());
	//if (iter != m_FreeSlots.end())
	//{
	//	m_FreeSlots.erase(iter);
	//}

	m_Objects[objHandle.GetIndex()].Set(std::move(pGameObject));

	auto lookupIter = m_LookupLayers.find(sLayerName);
	if (lookupIter == m_LookupLayers.end())
	{
		// 새 레이어 추가는 외부 vector 정렬을 일으키므로 모든 layer index 위치표를 다시 만든다.
		m_Layers.push_back({ std::string{sLayerName}, {objHandle} });
		SortLayer();
	}
	else
	{
		// 기존 레이어의 뒤에 추가되는 경우에는 새 객체 한 개의 위치만 O(1)로 기록한다.
		const size_t iLayerIndex = lookupIter->second;
		auto& LayerHandles = m_Layers[iLayerIndex].second;
		const size_t iHandleIndex = LayerHandles.size();
		LayerHandles.push_back(objHandle);
		SetObjectLayerLocation(objHandle, iLayerIndex, iHandleIndex);
	}

	// 복사 생성자/Initialize 중에도 삭제가 예약될 수 있다. 그 시점에는 아직 slot 등록 전이므로
	// 유효한 실제 Handle 요청이 큐에 없을 수 있어 등록 완료 후 정확한 Handle을 한 번 보장한다.
	if (pRegisteredObject->GetPendingDestroy() &&
		std::find(
			m_PendingDestroyHandles.begin(),
			m_PendingDestroyHandles.end(),
			objHandle) == m_PendingDestroyHandles.end())
	{
		QueuePendingDestroy(objHandle);
	}

	m_bUpdateViewsDirty = true;

	// 슬롯과 레이어 등록을 모두 마친 뒤 호출하여, 객체가 자기 Handle과 레이어를 안전하게 조회할 수 있게 한다.
	pRegisteredObject->OnRegisteredToManager();

	return objHandle;
}
void CGameObjectManager::SortLayer()
{
	// 레이어 이름 정렬은 바깥 vector의 index를 바꾸므로 lookup과 모든 slot 위치표를 함께 재구축한다.
	// 레이어 내부 Handle의 상대 순서는 유지하고, 남아 있을 수 있는 stale Handle만 방어적으로 제거한다.
	std::sort(m_Layers.begin(), m_Layers.end(),
		[](const std::pair<std::string, std::vector<CHandle>>& a,
			const std::pair<std::string, std::vector<CHandle>>& b) {
				return a.first < b.first;
		});

	m_LookupLayers.clear();
	m_LookupLayers.reserve(m_Layers.size());
	std::fill(
		m_ObjectLayerLocations.begin(),
		m_ObjectLayerLocations.end(),
		OBJECT_LAYER_LOCATION{});

	for (size_t iLayerIndex = 0;
		iLayerIndex < m_Layers.size();
		++iLayerIndex)
	{
		auto& [sLayerName, LayerHandles] = m_Layers[iLayerIndex];
		std::erase_if(
			LayerHandles,
			[this](const CHandle& hObject)
			{
				return GetGameObjectByHandle(hObject) == nullptr;
			});
		m_LookupLayers.emplace(sLayerName, iLayerIndex);

		for (size_t iHandleIndex = 0;
			iHandleIndex < LayerHandles.size();
			++iHandleIndex)
		{
			SetObjectLayerLocation(
				LayerHandles[iHandleIndex],
				iLayerIndex,
				iHandleIndex);
		}
	}
}

void CGameObjectManager::SetObjectLayerLocation(
	const CHandle& hObject,
	size_t iLayerIndex,
	size_t iHandleIndex)
{
	// slot index로 직접 접근하되 generation도 함께 저장하여 같은 slot의 이전 객체와 구분한다.
	if (hObject.GetIndex() >= m_ObjectLayerLocations.size())
		return;

	m_ObjectLayerLocations[hObject.GetIndex()] = {
		iLayerIndex,
		iHandleIndex,
		hObject.GetGeneration()
	};
}

_bool CGameObjectManager::FindObjectLayerIndex(
	const CHandle& hObject,
	size_t& iOutLayerIndex) const
{
	// 정상 경로는 slot index 한 번으로 위치를 얻고, 외부/내부 index와 실제 Handle까지 검증한다.
	if (hObject.GetIndex() < m_ObjectLayerLocations.size())
	{
		const auto& Location =
			m_ObjectLayerLocations[hObject.GetIndex()];
		if (Location.Matches(hObject) &&
			Location.iLayerIndex < m_Layers.size())
		{
			const auto& LayerHandles =
				m_Layers[Location.iLayerIndex].second;
			if (Location.iHandleIndex < LayerHandles.size() &&
				LayerHandles[Location.iHandleIndex] == hObject)
			{
				iOutLayerIndex = Location.iLayerIndex;
				return true;
			}
		}
	}

	// 위치표가 어긋난 경우에만 삭제를 놓치지 않도록 전체 레이어를 fallback 탐색한다.
	// 찾은 레이어는 이어지는 안정 압축 후 생존 Handle을 재색인하면서 위치표도 복구된다.
	for (size_t iLayerIndex = 0;
		iLayerIndex < m_Layers.size();
		++iLayerIndex)
	{
		const auto& LayerHandles = m_Layers[iLayerIndex].second;
		const auto Iter = std::find(
			LayerHandles.begin(),
			LayerHandles.end(),
			hObject);
		if (Iter == LayerHandles.end())
			continue;

		iOutLayerIndex = iLayerIndex;
		return true;
	}

	return false;
}

void CGameObjectManager::RemovePendingObjectsFromLayers()
{
	// 정상 삭제 비용을 모든 레이어 전체 스캔이 아니라
	// 삭제 요청 수 + 영향받은 레이어들의 Handle 수 합으로 제한하는 배치 정리 단계다.
	m_AffectedLayerIndices.clear();
	m_AffectedLayerIndices.reserve(m_ProcessingDestroyHandles.size());

	// 1) 삭제할 Handle의 정확한 generation을 slot marker에 기록하고 영향 레이어만 수집한다.
	for (const CHandle& hObject : m_ProcessingDestroyHandles)
	{
		size_t iLayerIndex{};
		if (!FindObjectLayerIndex(
			hObject,
			iLayerIndex))
			continue;

		m_PendingLayerRemoveGenerationKeys[hObject.GetIndex()] =
			static_cast<uint64_t>(hObject.GetGeneration()) + 1;
		m_AffectedLayerIndices.push_back(iLayerIndex);
	}

	// 2) 같은 레이어에서 여러 객체가 삭제돼도 해당 레이어를 이번 묶음에서 한 번만 순회한다.
	std::sort(
		m_AffectedLayerIndices.begin(),
		m_AffectedLayerIndices.end());
	m_AffectedLayerIndices.erase(
		std::unique(
			m_AffectedLayerIndices.begin(),
			m_AffectedLayerIndices.end()),
		m_AffectedLayerIndices.end());

	// 3) swap-pop을 쓰지 않고 안정 압축하여 front()/rbegin() 사용처가 보는 기존 삽입 순서를 보존한다.
	for (const size_t iLayerIndex : m_AffectedLayerIndices)
	{
		if (iLayerIndex >= m_Layers.size())
			continue;

		auto& LayerHandles = m_Layers[iLayerIndex].second;
		std::erase_if(
			LayerHandles,
			[this](const CHandle& hObject)
			{
				if (hObject.GetIndex() >=
					m_PendingLayerRemoveGenerationKeys.size() ||
					hObject.GetIndex() >= m_ObjectLayerLocations.size() ||
					m_PendingLayerRemoveGenerationKeys[hObject.GetIndex()] !=
						static_cast<uint64_t>(hObject.GetGeneration()) + 1)
				{
					return false;
				}

				m_ObjectLayerLocations[hObject.GetIndex()] = {};
				return true;
			});

		// 4) 압축으로 내부 index가 이동한 모든 생존 Handle의 위치 캐시를 다시 기록한다.
		for (size_t iHandleIndex = 0;
			iHandleIndex < LayerHandles.size();
			++iHandleIndex)
		{
			SetObjectLayerLocation(
				LayerHandles[iHandleIndex],
				iLayerIndex,
				iHandleIndex);
		}
	}

	// marker는 이번 묶음에만 유효하므로 처리한 generation만 0으로 되돌린다.
	for (const CHandle& hObject : m_ProcessingDestroyHandles)
	{
		if (hObject.GetIndex() >=
			m_PendingLayerRemoveGenerationKeys.size())
			continue;

		auto& iGenerationKey = m_PendingLayerRemoveGenerationKeys[
			hObject.GetIndex()];
		if (iGenerationKey ==
			static_cast<uint64_t>(hObject.GetGeneration()) + 1)
		{
			iGenerationKey = 0;
		}
	}

	m_AffectedLayerIndices.clear();
}

const std::vector<CHandle>* CGameObjectManager::GetLayer(std::string_view sLayerName) const
{
	auto iter = m_LookupLayers.find(sLayerName);
	if (iter == m_LookupLayers.end())
	{
		return nullptr;
	}

	return &m_Layers[iter->second].second;
}

const std::vector<CHandle>* CGameObjectManager::GetLayer(std::string_view sLayerName, const StringID& iPrototypeLevelIndex, const StringID& svPrototypeTag, void* pArg) 
{
	if (auto pLayer = GetLayer(sLayerName))
	{
		return pLayer;
	}
	else
	{
		AddGameObjectToLayer(iPrototypeLevelIndex, svPrototypeTag, sLayerName, pArg);
		return GetLayer(sLayerName);
	}
}

void CGameObjectManager::DelLayer(std::string_view sLayerName)
{
	const auto LayerIter = m_LookupLayers.find(sLayerName);
	if (LayerIter == m_LookupLayers.end())
		return;
	const size_t iLayerIndex = LayerIter->second;

	// 슬롯 파괴는 FrameEnd로 미루되 레이어에서는 즉시 분리한다.
	// MapManager처럼 DelLayer 직후 같은 호출 스택에서 새 데이터를 구성·저장하는 사용처가
	// PendingDestroy 상태인 이전 객체를 다시 읽지 않도록 기존 동기식 논리 삭제 의미를 유지한다.
	const std::array<std::string_view, 1> layerNames{ sLayerName };
	RequestResetByLayers(layerNames, RESET_LAYER_MODE::INCLUDE_ONLY);

	auto& LayerHandles = m_Layers[iLayerIndex].second;
	for (const CHandle& hObject : LayerHandles)
	{
		if (CGameObject* pObject = GetGameObjectByHandle(hObject))
			pObject->CommitPendingDestroy();

		if (hObject.GetIndex() >= m_ObjectLayerLocations.size())
			continue;

		auto& Location = m_ObjectLayerLocations[hObject.GetIndex()];
		if (Location.Matches(hObject))
			Location = {};
	}
	LayerHandles.clear();
}

// 아래 네 dispatch는 FrameStart에서 마스크로 이미 필터링된 view만 순회한다.
// PendingDestroy/ManagedUpdateEnabled와 TimeDomain은 프레임 중 바뀔 수 있어 호출 직전에 판정한다.
void CGameObjectManager::FixedUpdate(
	_float fScaledDelta,
	_float fUnscaledDelta)
{
	ZoneScopedN("CGameObjectManager_FixedUpdate");
	const _bool bTraceObjects =
		m_bTracyDetailedObjectProfiling && TracyIsConnected;

	for (CGameObject* pObj : m_FixedUpdateObjects)
	{
		if (!pObj->GetPendingDestroy() &&
			pObj->IsManagedUpdateEnabled())
		{
			ZoneNamedN(tObjectZone, "GameObject_FixedUpdate", bTraceObjects);
			if (bTraceObjects)
			{
				const std::string sDebugLabel = GetGameObjectDebugLabel(pObj);
				ZoneNameV(tObjectZone, sDebugLabel.data(), sDebugLabel.size());
			}

			const _float fObjectDelta =
				pObj->GetTimeDomain() == TIME_DOMAIN::UNSCALED
				? fUnscaledDelta
				: fScaledDelta;
			pObj->FixedUpdate(fObjectDelta);
		}
	}
}

void CGameObjectManager::PriorityUpdate(
	_float fScaledDelta,
	_float fUnscaledDelta)
{
	ZoneScopedN("CGameObjectManager_PriorityUpdate");
	const _bool bTraceObjects =
		m_bTracyDetailedObjectProfiling && TracyIsConnected;

	for (CGameObject* pObj : m_PriorityUpdateObjects)
	{
		if (!pObj->GetPendingDestroy() &&
			pObj->IsManagedUpdateEnabled())
		{
			ZoneNamedN(tObjectZone, "GameObject_PriorityUpdate", bTraceObjects);
			if (bTraceObjects)
			{
				const std::string sDebugLabel = GetGameObjectDebugLabel(pObj);
				ZoneNameV(tObjectZone, sDebugLabel.data(), sDebugLabel.size());
			}

			const _float fObjectDelta =
				pObj->GetTimeDomain() == TIME_DOMAIN::UNSCALED
				? fUnscaledDelta
				: fScaledDelta;
			pObj->PriorityUpdate(fObjectDelta);
		}
	}
}

void CGameObjectManager::Update(
	_float fScaledDelta,
	_float fUnscaledDelta)
{
	ZoneScopedN("CGameObjectManager_Update");
	const _bool bTraceObjects =
		m_bTracyDetailedObjectProfiling && TracyIsConnected;

	for (CGameObject* pObj : m_UpdateObjects)
	{
		if (!pObj->GetPendingDestroy() &&
			pObj->IsManagedUpdateEnabled())
		{
			ZoneNamedN(tObjectZone, "GameObject_Update", bTraceObjects);
			if (bTraceObjects)
			{
				const std::string sDebugLabel = GetGameObjectDebugLabel(pObj);
				ZoneNameV(tObjectZone, sDebugLabel.data(), sDebugLabel.size());
			}

			const _float fObjectDelta =
				pObj->GetTimeDomain() == TIME_DOMAIN::UNSCALED
				? fUnscaledDelta
				: fScaledDelta;
			pObj->Update(fObjectDelta);
		}
	}
}

void CGameObjectManager::LateUpdate(
	_float fScaledDelta,
	_float fUnscaledDelta)
{
	ZoneScopedN("CGameObjectManager_LateUpdate");
	const _bool bTraceObjects =
		m_bTracyDetailedObjectProfiling && TracyIsConnected;

	for (CGameObject* pObj : m_LateUpdateObjects)
	{
		if (!pObj->GetPendingDestroy() &&
			pObj->IsManagedUpdateEnabled())
		{
			ZoneNamedN(tObjectZone, "GameObject_LateUpdate", bTraceObjects);
			if (bTraceObjects)
			{
				const std::string sDebugLabel = GetGameObjectDebugLabel(pObj);
				ZoneNameV(tObjectZone, sDebugLabel.data(), sDebugLabel.size());
			}

			const _float fObjectDelta =
				pObj->GetTimeDomain() == TIME_DOMAIN::UNSCALED
				? fUnscaledDelta
				: fScaledDelta;
			pObj->LateUpdate(fObjectDelta);
		}
	}
}

HRESULT CGameObjectManager::Initialize()
{
	m_PriorityUpdateObjects.reserve(100);
	m_FixedUpdateObjects.reserve(100);
	m_UpdateObjects.reserve(100);
	m_LateUpdateObjects.reserve(100);
	m_PendingDestroyHandles.reserve(100);
	m_ProcessingDestroyHandles.reserve(100);
	m_ObjectLayerLocations.reserve(100);
	m_PendingLayerRemoveGenerationKeys.reserve(100);
	m_AffectedLayerIndices.reserve(100);
	return S_OK;
}


UPtr<CGameObjectManager> CGameObjectManager::Create()
{
	auto pInstance = ToUPtr(new CGameObjectManager{});
	if (FAILED(pInstance->Initialize()))
	{
		return nullptr;
	}
	return pInstance;
}

// 즉시 해제하지 않고 모든 객체를 PendingDestroy로 예약한다.
// 엔진 종료 시에도 Manager가 살아 있는 상태에서 이어서 FrameEnd를 호출해야
// 객체 소멸자의 Manager 재진입과 추가 삭제 요청을 안전하게 끝낼 수 있다.
void CGameObjectManager::AllReset()
{
	RequestResetByLayers({}, RESET_LAYER_MODE::EXCLUDE);
}

size_t CGameObjectManager::ResetObjectsInLayers(
	std::span<const std::string_view> layerNames)
{
	return RequestResetByLayers(layerNames, RESET_LAYER_MODE::INCLUDE_ONLY);
}

size_t CGameObjectManager::ResetAllObjectsExceptLayers(
	std::span<const std::string_view> excludedLayerNames)
{
	return RequestResetByLayers(excludedLayerNames, RESET_LAYER_MODE::EXCLUDE);
}

size_t CGameObjectManager::RequestResetByLayers(
	std::span<const std::string_view> layerNames,
	RESET_LAYER_MODE eMode)
{
	ZoneScopedN("CGameObjectManager_RequestResetByLayers");

	// 전달받은 레이어와 그 레이어가 소유한 유효 오브젝트 슬롯을 빠르게 판별하기 위한 표식이다.
	std::vector<uint8_t> matchedSlots(m_Objects.size(), 0);
	std::vector<uint8_t> matchedLayers(m_Layers.size(), 0);

	// 존재하는 레이어만 찾고, 세대까지 유효한 Handle의 slot만 제거 후보로 표시한다.
	for (const std::string_view layerName : layerNames)
	{
		const auto layerIter = m_LookupLayers.find(layerName);
		if (layerIter == m_LookupLayers.end())
			continue;

		const size_t layerIndex = layerIter->second;
		matchedLayers[layerIndex] = 1;

		const auto& handles = m_Layers[layerIndex].second;
		for (const CHandle& handle : handles)
		{
			if (GetGameObjectByHandle(handle))
			{
				matchedSlots[handle.GetIndex()] = 1;
			}
		}
	}

	// FrameEnd에서 비어 있을 때 제거할 실제 reset 대상 레이어 이름을 누적한다.
	// 객체가 0개인 레이어도 DelLayer 요청이라면 제거해야 하므로 이름 자체를 별도로 기억한다.
	_bool bHasResetTargetLayer = false;
	for (size_t i = 0; i < m_Layers.size(); ++i)
	{
		const _bool bLayerMatched = matchedLayers[i] != 0;
		const _bool bResetTargetLayer =
			eMode == RESET_LAYER_MODE::INCLUDE_ONLY ?
			bLayerMatched : !bLayerMatched;

		if (bResetTargetLayer)
		{
			m_PendingResetTargetLayers.emplace(m_Layers[i].first);
			bHasResetTargetLayer = true;
		}
	}

	// 모드에 따라 선택된 객체를 실제 삭제하지 않고 PendingDestroy 상태로 예약한다.
	size_t resetObjectCount{};
	for (size_t i = 0; i < m_Objects.size(); ++i)
	{
		auto& slot = m_Objects[i];
		if (!slot.IsOccupied())
			continue;

		CGameObject* pObject = slot.Get();
		const _bool bMatched = matchedSlots[i] != 0;
		const _bool bShouldReset =
			eMode == RESET_LAYER_MODE::INCLUDE_ONLY ? bMatched : !bMatched;

		if (!bShouldReset || pObject->GetPendingDestroy())
			continue;

		pObject->SetPendingDestroy();
		++resetObjectCount;
	}

	if (resetObjectCount > 0 || bHasResetTargetLayer)
	{
		// 여러 reset 요청은 FrameEnd 전까지 합쳐진다. 빈 대상 레이어만 있는 요청도 여기서 처리한다.
		m_bBatchResetPending = true;
	}

	return resetObjectCount;
}

void CGameObjectManager::QueuePendingDestroy(const CHandle& hObject)
{
	// enqueue는 선형 중복 검색 없이 O(1) append만 한다. 일반 중복은 GameObject의 상태 guard가 막고,
	// 취소·stale generation·드문 중복 요청은 FrameEnd의 Handle 검증이 안전하게 걸러낸다.
	m_PendingDestroyHandles.push_back(hObject);
}

_bool CGameObjectManager::DestroyQueuedPendingObjects()
{
	ZoneScopedN("CGameObjectManager_DestroyQueuedPendingObjects");

	const _bool bTracyConnected =
		m_bTracyDetailedObjectProfiling && TracyIsConnected;
	_bool bAnyObjectDestroyed = false;

	// 소멸자가 새 삭제를 예약하면 Pending 큐에 들어가며 다음 wave에서 같은 FrameEnd 안에 계속 drain한다.
	while (!m_PendingDestroyHandles.empty())
	{
		// 현재 wave를 snapshot으로 떼어내 순회 중 새 enqueue가 vector를 무효화하지 않게 한다.
		m_ProcessingDestroyHandles.clear();
		m_ProcessingDestroyHandles.swap(m_PendingDestroyHandles);

		// stale generation과 취소된 요청은 이 wave에서 제거한다.
		// 남은 Handle의 상대 순서는 기존 요청 순서 그대로 유지한다.
		std::erase_if(
			m_ProcessingDestroyHandles,
			[this](const CHandle& hObject)
			{
				const CGameObject* pObject = GetGameObjectByHandle(hObject);
				return pObject == nullptr || !pObject->GetPendingDestroy();
			});

		// 소멸자를 호출하기 전에 레이어에서 이번 wave의 Handle을 먼저 안정 압축한다.
		// 이후 객체 Reset과 레이어 상태가 갈라지지 않도록 이 지점부터 삭제 배치는 확정된다.
		RemovePendingObjectsFromLayers();

		for (const CHandle& hObject : m_ProcessingDestroyHandles)
		{
			CGameObject* pObject = GetGameObjectByHandle(hObject);
			// 이 배치에 들어온 순간 삭제는 확정됐다. 레이어 제거 뒤에는
			// 소멸자 재진입으로 PendingDestroy가 바뀌어도 취소하지 않는다.
			if (!pObject)
				continue;

			ZoneNamedN(tObjectZone, "GameObject_Destroy", bTracyConnected);
			if (bTracyConnected)
			{
				const std::string sDebugLabel = GetGameObjectDebugLabel(pObject);
				ZoneNameV(tObjectZone, sDebugLabel.data(), sDebugLabel.size());
			}

			// Reset은 객체를 소멸시키고 slot generation을 증가시킨다.
			// 그 뒤에만 free list에 넣으므로 같은 wave의 중복/구세대 Handle은 다시 파괴할 수 없다.
			m_Objects[hObject.GetIndex()].Reset();
			m_FreeSlots.push_back(hObject.GetIndex());
			bAnyObjectDestroyed = true;
		}
	}

	m_ProcessingDestroyHandles.clear();

	return bAnyObjectDestroyed;
}

_bool CGameObjectManager::RemoveEmptyResetTargetLayers(
	const std::unordered_set<std::string>& resetTargetLayers)
{
	// 일반 삭제로 우연히 빈 레이어는 유지하고, 이번에 명시적으로 reset/del을 요청한 레이어만 제거한다.
	// stale Handle은 레이어 삭제를 막지 않으며, 소멸 중 추가됐거나 삭제가 취소된 유효 객체가 하나라도 있으면 보존한다.
	const size_t removedLayerCount = std::erase_if(
		m_Layers,
		[this, &resetTargetLayers](const auto& layer)
		{
			if (!resetTargetLayers.contains(layer.first))
				return false;

			return std::none_of(
				layer.second.begin(),
				layer.second.end(),
				[this](const CHandle& hObject)
				{
					return GetGameObjectByHandle(hObject) != nullptr;
				});
		});

	return removedLayerCount > 0;
}

void CGameObjectManager::Free()
{
	CEngineBase::Free();
}
