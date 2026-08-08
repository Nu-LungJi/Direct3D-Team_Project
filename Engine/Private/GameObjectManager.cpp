#include "pch.h"
#include "GameObjectManager.h"
#include "GameInstance.h"
#include "GameObject.h"
//#include "Helper.h"
#include "MyTreeNode.h"

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

			const bool bSlotOpened = ImGui::TreeNode(sSlotLabel.c_str());

			if (bInvalidState)
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

	const bool bLayerFilterActive = m_GUISearchFilter[0] != '\0';
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
		ImGui::SetNextItemWidth(-1);
		ImGui::InputTextWithHint(
			"##layer_search",
			"Search layer or object...",
			m_GUISearchFilter,
			sizeof(m_GUISearchFilter));
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

			if (bLayerFilterActive && iMatchedItemCount == 0)
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

			if (ImGui::TreeNode(sLayerLabel.c_str()))
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
						if (ImGui::TreeNode(sObjectLabel.c_str()))
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

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Tree"))
	{
		ImGui::SetNextItemWidth(-1);
		ImGui::InputTextWithHint("##search", "Search...", m_GUISearchFilter, sizeof(m_GUISearchFilter));
		ImGui::Separator();

		for (const auto& rootHandle : m_TreePreparation)
		{
			UpdateGUIDrawTreeNode(rootHandle);
		}

		ImGui::TreePop();
	}

	ImGui::End();
}

bool CGameObjectManager::MatchesGUIFilter(std::string_view sText) const
{
	if (m_GUISearchFilter[0] == '\0')
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

bool CGameObjectManager::MatchesFilter(CGameObject* pObj) const
{
	const std::string label = GetGameObjectDebugLabel(pObj);

	std::string labelLower = label;
	std::string filterLower = m_GUISearchFilter;
	std::transform(labelLower.begin(), labelLower.end(), labelLower.begin(), ::tolower);
	std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);

	if (labelLower.find(filterLower) != std::string::npos)
		return true;

	for (const auto& pChild : pObj->GetChildrenNode())
	{
		if (MatchesFilter(pChild))
			return true;
	}

	return false;
}

void CGameObjectManager::UpdateGUIDrawTreeNode(CGameObject* pObj)
{
	const CHandle handle = pObj->GetHandle();
	std::string label = GetGameObjectDebugLabel(pObj);
	label += "###TreeObject_" + std::to_string(handle.GetIndex()) + "_" +
		std::to_string(handle.GetGeneration());

	if (m_GUISearchFilter[0] != '\0' && !MatchesFilter(pObj))
		return;

	if (ImGui::TreeNode(label.c_str()))
	{
		pObj->UpdateGUI();

		if (!pObj->GetChildrenNode().empty())
		{
			if (ImGui::TreeNode("Childrens"))
			{
				for (const auto& pChild : pObj->GetChildrenNode())
				{
					UpdateGUIDrawTreeNode(pChild);
				}

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}
}

void CGameObjectManager::FrameStart()
{
	ZoneScopedN("CGameObjectManager_FrameStart");

	if (!m_bTreeReBuild)
	{
		return;
	}

	{
		ZoneScopedN("CGameObjectManager_RebuildTree");

		m_TreePreparation.clear();
		for (const auto& [_, Layer] : m_Layers)
		{
			for (const auto& handle : Layer)
			{
				if (auto* pObj = GetGameObjectByHandle(handle))
				{
					if (!pObj->GetParentNode())
					{
						m_TreePreparation.push_back(pObj);
					}
				}
			}
		}

		m_Tree.clear();
		for (const auto& pRootObj : m_TreePreparation)
		{
			MyTreeDFS(pRootObj, [&](auto pObj) {m_Tree.push_back(pObj); }, &m_DFSReserved);
		}
	}

	// TODO: 플래그 완성되면
	//m_bTreeReBuild = false;
}

void CGameObjectManager::FrameEnd()
{
	ZoneScopedN("CGameObjectManager_FrameEnd");
	const _bool bTracyConnected = TracyIsConnected;

	if (m_bAllResetCalled)
	{
		ZoneScopedN("CGameObjectManager_AllObjectsReset");
		m_bAllResetCalled = false;
		AllObjectsReset();
	}
	else
	{
		ZoneScopedN("CGameObjectManager_DestroyPendingObjects");

		_bool bDestroyedAny = false;
		for (auto iter = m_Tree.rbegin(); iter != m_Tree.rend(); ++iter)
		{
			CGameObject* pObj = *iter;
			CHandle hObj = pObj->GetHandle();

			// double check 필요 없을듯
			//if (GetGameObjectByHandle(hObj) == pObj)
			//{
			if (pObj->GetPendingDestroy())
			{
				ZoneNamedN(tObjectZone, "GameObject_Destroy", bTracyConnected);
				if (bTracyConnected)
				{
					const std::string sDebugLabel = GetGameObjectDebugLabel(pObj);
					ZoneNameV(tObjectZone, sDebugLabel.data(), sDebugLabel.size());
				}

				m_bTreeReBuild = true;
				m_Objects[hObj.GetIndex()].Reset();
				m_FreeSlots.push_back(hObj.GetIndex());
				bDestroyedAny = true;
			}
			//}
		}

		if (bDestroyedAny)
		{
			ZoneScopedN("CGameObjectManager_RemoveInvalidLayerHandles");

			for (auto& [_, handles] : m_Layers)
			{
				std::erase_if(handles, [this](const CHandle& handle)
				{
					return GetGameObjectByHandle(handle) == nullptr;
				});
			}
		}
	}
}

std::optional<CHandle> CGameObjectManager::GetFreeHandle() 
{
	CHandle objectHandle{};
	if (m_FreeSlots.empty())
	{
		size_t idx = m_Objects.size();
		uint32_t gen = 0;

		objectHandle = CHandle{ idx, gen };
		m_Objects.push_back({});
		//m_FreeSlots.push_back(idx);
	}
	else
	{
		size_t emptyIdx = m_FreeSlots.back();
		m_FreeSlots.pop_back();

		size_t idx = emptyIdx;
		uint32_t gen = m_Objects[emptyIdx].GetGeneration();

		objectHandle = CHandle{ idx, gen };
	}
	return objectHandle;
}

std::optional<CHandle> CGameObjectManager::AddGameObjectToLayer(const StringID& siProtoGroupTag, const StringID& siPrototypeTag, std::string_view sLayerName, void* pArg)
{
	const _bool bTracyConnected = TracyIsConnected;
	ZoneNamedN(tObjectZone, "CGameObjectManager_AddGameObjectToLayer", bTracyConnected);

	auto pDesc = static_cast<CGameObject::GAMEOBJECT_DESC*>(pArg);
	auto allocHandle = GetFreeHandle();
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

	//auto iter = std::find(m_FreeSlots.begin(), m_FreeSlots.end(), objHandle.GetIndex());
	//if (iter != m_FreeSlots.end())
	//{
	//	m_FreeSlots.erase(iter);
	//}

	m_Objects[objHandle.GetIndex()].Set(std::move(pGameObject));

	auto lookupIter = m_LookupLayers.find(sLayerName);
	if (lookupIter == m_LookupLayers.end())
	{
		m_Layers.push_back({ std::string{sLayerName}, {objHandle} });
		SortLayer();
	}
	else
	{
		m_Layers[lookupIter->second].second.push_back(objHandle);
	}

	m_bTreeReBuild = true;

	return objHandle;
}


std::optional<CHandle> CGameObjectManager::GetHandleByGameObject(CGameObject* pObj) const
{
	uint32_t i{};
	for (i = 0; i < m_Objects.size(); ++i)
	{
		if (m_Objects[i].Get() == pObj)
		{
			break;
		}
	}

	if (i == m_Objects.size() - 1)
	{
		return std::nullopt;
	}

	return CHandle{ i, m_Objects[i].GetGeneration() };
}

void CGameObjectManager::SortLayer()
{
	std::sort(m_Layers.begin(), m_Layers.end(),
		[](const std::pair<std::string, std::vector<CHandle>>& a,
			const std::pair<std::string, std::vector<CHandle>>& b) {
				return a.first < b.first;
		});

	m_LookupLayers.clear();
	m_LookupLayers.reserve(m_Layers.size());

	for (size_t i = 0; i < m_Layers.size(); ++i)
	{
		m_LookupLayers.emplace(m_Layers[i].first, i);
	}
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
	auto iter = m_LookupLayers.find(sLayerName);
	if (iter == m_LookupLayers.end())
	{
		return;
	}
	for (const auto& handle : m_Layers[iter->second].second)
	{
		if (auto pObj = GetGameObjectByHandle(handle))
		{
			pObj->SetPendingDestroy();
		}
	}
	m_Layers[iter->second].second.clear();

	SortLayer();
}

void CGameObjectManager::FixedUpdate(_float fTimeDelta)
{
	ZoneScopedN("CGameObjectManager_FixedUpdate");
	const _bool bTracyConnected = TracyIsConnected;

	for (auto& pObj : m_Tree)
	{
		if (!pObj->GetPendingDestroy())
		{
			ZoneNamedN(tObjectZone, "GameObject_FixedUpdate", bTracyConnected);
			if (bTracyConnected)
			{
				const std::string sDebugLabel = GetGameObjectDebugLabel(pObj);
				ZoneNameV(tObjectZone, sDebugLabel.data(), sDebugLabel.size());
			}

			pObj->FixedUpdate(fTimeDelta);
		}
	}
}

void CGameObjectManager::PriorityUpdate(_float fTimeDelta)
{
	ZoneScopedN("CGameObjectManager_PriorityUpdate");
	const _bool bTracyConnected = TracyIsConnected;

	for (auto& pObj : m_Tree)
	{
		if (!pObj->GetPendingDestroy())
		{
			ZoneNamedN(tObjectZone, "GameObject_PriorityUpdate", bTracyConnected);
			if (bTracyConnected)
			{
				const std::string sDebugLabel = GetGameObjectDebugLabel(pObj);
				ZoneNameV(tObjectZone, sDebugLabel.data(), sDebugLabel.size());
			}

			pObj->PriorityUpdate(fTimeDelta);
		}
	}
}

void CGameObjectManager::Update(_float fTimeDelta)
{
	ZoneScopedN("CGameObjectManager_Update");
	const _bool bTracyConnected = TracyIsConnected;

	for (auto& pObj : m_Tree)
	{
		if (!pObj->GetPendingDestroy())
		{
			ZoneNamedN(tObjectZone, "GameObject_Update", bTracyConnected);
			if (bTracyConnected)
			{
				const std::string sDebugLabel = GetGameObjectDebugLabel(pObj);
				ZoneNameV(tObjectZone, sDebugLabel.data(), sDebugLabel.size());
			}

			pObj->Update(fTimeDelta);
		}
	}
}

void CGameObjectManager::LateUpdate(_float fTimeDelta)
{
	ZoneScopedN("CGameObjectManager_LateUpdate");
	const _bool bTracyConnected = TracyIsConnected;

	for (auto& pObj : m_Tree)
	{
		if (!pObj->GetPendingDestroy())
		{
			ZoneNamedN(tObjectZone, "GameObject_LateUpdate", bTracyConnected);
			if (bTracyConnected)
			{
				const std::string sDebugLabel = GetGameObjectDebugLabel(pObj);
				ZoneNameV(tObjectZone, sDebugLabel.data(), sDebugLabel.size());
			}

			pObj->LateUpdate(fTimeDelta);
		}
	}
}

HRESULT CGameObjectManager::Initialize()
{
	m_DFSReserved.reserve(100);
	m_TreePreparation.reserve(100);
	m_Tree.reserve(100);
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

// 이거 먼저 호출해주어야함
// 왜냐면 매니저가 지워지면서 오브젝트들지워주는데
// 지우는 과정에서 매니저 호출하는데 매니저가 없어서 크래시남
// 그래서 지우기 전에 먼저 이거 호출
void CGameObjectManager::AllReset()
{
	for (auto& pObj : m_Objects)
	{
		if (pObj.IsOccupied() && !pObj.Get()->IsPersistent())
		{
			pObj.Get()->SetPendingDestroy();
		}
	}
	m_bTreeReBuild = true;
	m_bAllResetCalled = true;
	//FrameStart();
	//FrameEnd();

	for (auto& [_, handles] : m_Layers)
	{
		std::erase_if(handles, [this](const CHandle& handle)
		{
			const auto* pObject = GetGameObjectByHandle(handle);
			return pObject == nullptr || !pObject->IsPersistent();
		});
	}
	std::erase_if(m_Layers, [](const auto& layer)
	{
		return layer.second.empty();
	});
	SortLayer();
	m_TreePreparation.clear();
	m_Tree.clear();
}

void CGameObjectManager::AllObjectsReset()
{
	ZoneScopedN("CGameObjectManager_AllObjectsResetInternal");
	const _bool bTracyConnected = TracyIsConnected;

	for (auto& pObj : m_Objects)
	{
		if (pObj.IsOccupied())
		{
			CHandle hObj = pObj.Get()->GetHandle();
			if (pObj.Get()->GetPendingDestroy())
			{
				ZoneNamedN(tObjectZone, "GameObject_Destroy", bTracyConnected);
				if (bTracyConnected)
				{
					const std::string sDebugLabel = GetGameObjectDebugLabel(pObj.Get());
					ZoneNameV(tObjectZone, sDebugLabel.data(), sDebugLabel.size());
				}

				m_Objects[hObj.GetIndex()].Reset();
				m_FreeSlots.push_back(hObj.GetIndex());
			}
		}
	}
}

void CGameObjectManager::Free()
{
	CEngineBase::Free();
}
