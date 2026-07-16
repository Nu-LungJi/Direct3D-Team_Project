#include "pch.h"
#include "PrototypeManager.h"
NS_USING(Engine)

CPrototypeManager::CPrototypeManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	:m_pDevice{ pDevice }, m_pContext{ pContext }
{
}
CPrototypeManager::~CPrototypeManager()
{
}

void CPrototypeManager::UpdateGUI()
{
	std::vector<std::pair<StringID, std::vector<StringID>>> prototypeTags{};
	{
		std::shared_lock lock{ m_PrototypeMutex };
		prototypeTags.reserve(m_pPrototypes.size());
		for (const auto& [groupID, prototypes] : m_pPrototypes)
		{
			auto& [groupTag, tags] = prototypeTags.emplace_back(groupID, std::vector<StringID>{});
			tags.reserve(prototypes.size());
			for (const auto& [prototypeID, prototype] : prototypes)
				tags.emplace_back(prototypeID);
		}
	}

	ImGui::Begin("CPrototype_Manager");

	for (const auto& [groupID, tags] : prototypeTags)
	{
		if (ImGui::TreeNode(groupID.GetDbgStr()))
		{
			for (const auto& protoID : tags)
				ImGui::Text(protoID.GetDbgStr());

			ImGui::TreePop();
		}
	}

	ImGui::End();
}

HRESULT CPrototypeManager::Initialize()
{
	//m_iNumLevels = iNumLevels;
	//m_pPrototypes = std::make_unique<PROTOTYPES[]>(iNumLevels);
	return S_OK;
}

HRESULT CPrototypeManager::AddPrototype(const StringID& svGroupTag, const StringID& svPrototypeTag, UPtr<CPrototype> pPrototype)
{
	if (!pPrototype)
		return E_INVALIDARG;

	std::unique_lock lock{ m_PrototypeMutex };
	auto& group = m_pPrototypes[svGroupTag];
	group.insert_or_assign(svPrototypeTag, std::move(pPrototype));
	return S_OK;
}

UPtr<CPrototype> CPrototypeManager::ClonePrototype(const StringID& svGroupTag, const StringID& svPrototypeTag, void* pArg)
{
	std::shared_lock lock{ m_PrototypeMutex };
	CPrototype* pPrototype = Find_Prototype(svGroupTag, svPrototypeTag);
	if (pPrototype == nullptr)
	{
		return nullptr;
	}
	return pPrototype->Clone(pArg);
}

void CPrototypeManager::DelPrototype(const StringID& sGroupTag)
{
	std::unique_lock lock{ m_PrototypeMutex };
	auto iter = m_pPrototypes.find(sGroupTag);
	if (iter != m_pPrototypes.end())
	{
		m_pPrototypes.erase(iter);
	}
}

std::vector<StringID> CPrototypeManager::GetPrototypeTags(const StringID& svGroupTag) const
{
	std::vector<StringID> tags{};
	std::shared_lock lock{ m_PrototypeMutex };
	auto iter = m_pPrototypes.find(svGroupTag);
	if (iter == m_pPrototypes.end())
		return tags;

	tags.reserve(iter->second.size());
	for (const auto& [prototypeTag, prototype] : iter->second)
		tags.emplace_back(prototypeTag);
	return tags;
}

//const std::unordered_map<StringID, UPtr<CPrototype>>& GetPrototype(const StringID& svGroupTag) const
//{
//	//std::unordered_map<StringID, CPrototype*> ret{};
//	//auto iter = m_pPrototypes.find(svGroupTag);
//	//if (iter != m_pPrototypes.end())
//	//{
//	//	for (const auto& [k, b] : iter->second)
//	//	{
//	//		ret.emplace(k, b.get());
//	//	}
//	//	return ret;
//	//}
//	//return ret;
//}

CPrototypeManager::PROTOTYPES* CPrototypeManager::Find_Group(const StringID& svGroupTag)
{
	auto iter = m_pPrototypes.find(svGroupTag);
	if (iter == m_pPrototypes.end())
	{
		return nullptr;
	}

	return &iter->second;
}

CPrototype* CPrototypeManager::Find_Prototype(const StringID& svGroupTag, const StringID& svPrototypeTag)
{
	//if (iLevelIndex >= m_iNumLevels)
	//{
	//	return nullptr;
	//}

	auto group = Find_Group(svGroupTag);
	if (group == nullptr)
	{
		return nullptr;
	}

	auto protoIter = group->find(svPrototypeTag);
	if (protoIter == group->end())
	{
		return nullptr;
	}

	//auto iter = m_pPrototypes[iLevelIndex].find(svPrototypeTag);
	//if (iter == m_pPrototypes[iLevelIndex].end())
	//{
	//	return nullptr;
	//}

	return protoIter->second.get();
}

UPtr<CPrototypeManager> CPrototypeManager::Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
{
	auto		pInstance = UPtr<CPrototypeManager>(new CPrototypeManager{ pDevice , pContext });

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CPrototypeManager");
		return nullptr;
	}

	return pInstance;
}
