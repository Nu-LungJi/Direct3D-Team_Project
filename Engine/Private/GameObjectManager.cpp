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
	ImGui::Begin("GameObject_Manager");

	if (ImGui::TreeNode("FreeSlots"))
	{
		for (const auto& i : m_FreeSlots)
		{
			ImGui::Text("%i", i);
		}

		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Slots"))
	{
		for (size_t i = 0; i < m_Objects.size(); ++i)
		{
			std::string occ = m_Objects[i].IsOccupied() ? "" : "[Empty]";
			std::string a = "idx_" + std::to_string(i) + "_gen_" + std::to_string(m_Objects[i].GetGeneration());
			occ += a;
			if (ImGui::TreeNode(occ.c_str()))
			{
				if (m_Objects[i].IsOccupied())
				{
					m_Objects[i].Get()->UpdateGUI();
				}

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Layers"))
	{
		for (uint32_t i = 0; i < m_Layers.size(); ++i)
		{
			if (ImGui::TreeNode(m_Layers[i].first.c_str()))
			{
				for (const auto& handle : m_Layers[i].second)
				{
					if (auto* pObj = GetGameObjectByHandle(handle))
					{
						std::string s = std::to_string(handle.GetIndex()) + "_" + std::string{ pObj->GetObjectTag() };
						if (ImGui::TreeNode(s.data()))
						{
							pObj->UpdateGUI();

							ImGui::TreePop();
						}
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
bool CGameObjectManager::MatchesFilter(CGameObject* pObj) const
{
	std::string label =
		std::to_string(pObj->GetHandle().GetIndex()) + "_" + std::string{ pObj->GetObjectTag() };

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
	std::string label =
		std::to_string(pObj->GetHandle().GetIndex()) + "_" + std::string{ pObj->GetObjectTag() };

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
	if (!m_bTreeReBuild)
	{
		return;
	}

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

	// TODO: 플래그 완성되면
	//m_bTreeReBuild = false;
}

void CGameObjectManager::FrameEnd()
{
	for (auto iter = m_Tree.rbegin(); iter != m_Tree.rend(); ++iter)
	{
		CGameObject* pObj = *iter;
		CHandle hObj = pObj->GetHandle();

		// double check 필요 없을듯
		//if (GetGameObjectByHandle(hObj) == pObj)
		//{
			if (pObj->GetPendingDestroy())
			{
				m_bTreeReBuild = true;
				m_Objects[hObj.GetIndex()].Reset();
				m_FreeSlots.push_back(hObj.GetIndex());
			}
		//}
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
	FrameEnd();

	SortLayer();
}

void CGameObjectManager::PriorityUpdate(_float fTimeDelta)
{
	for (auto& pObj : m_Tree)
	{
		if (!pObj->GetPendingDestroy())
		{
			pObj->PriorityUpdate(fTimeDelta);
		}
	}
}

void CGameObjectManager::Update(_float fTimeDelta)
{
	for (auto& pObj : m_Tree)
	{
		if (!pObj->GetPendingDestroy())
		{
			pObj->Update(fTimeDelta);
		}
	}
}

void CGameObjectManager::LateUpdate(_float fTimeDelta)
{
	for (auto& pObj : m_Tree)
	{
		if (!pObj->GetPendingDestroy())
		{
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
		if (pObj.IsOccupied())
		{
			pObj.Get()->SetPendingDestroy();
		}
	}
	m_bTreeReBuild = true;
	FrameStart();
	FrameEnd();

	m_Layers.clear();
	m_LookupLayers.clear();
	m_TreePreparation.clear();
	m_Tree.clear();
}

void CGameObjectManager::Free()
{
	CEngineBase::Free();
}
