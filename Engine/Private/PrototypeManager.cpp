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
	ImGui::Begin("CPrototype_Manager");

	for (const auto& [groupID, protoes] : m_pPrototypes)
	{
		if (ImGui::TreeNode(groupID.GetDbgStr()))
		{
			for (const auto& [protoID, proto] : protoes)
			{
				ImGui::Text(protoID.GetDbgStr());
			}

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
	//if (Find_Prototype(svGroupTag, svPrototypeTag))
	//{
	//	return E_FAIL;
	//}

	auto group = Find_Group(svGroupTag);
	if (group)
	{
		group->emplace(svPrototypeTag, std::move(pPrototype));
	}
	else
	{
		CPrototypeManager::PROTOTYPES newPrototypes{};
		newPrototypes.emplace(svPrototypeTag, std::move(pPrototype));
		m_pPrototypes.emplace(svGroupTag, std::move(newPrototypes));
	}
	return S_OK;
}

UPtr<CPrototype> CPrototypeManager::ClonePrototype(const StringID& svGroupTag, const StringID& svPrototypeTag, void* pArg)
{
	CPrototype* pPrototype = Find_Prototype(svGroupTag, svPrototypeTag);
	if (pPrototype == nullptr)
	{
		return nullptr;
	}
	return pPrototype->Clone(pArg);
}

void CPrototypeManager::DelPrototype(const StringID& sGroupTag)
{
	auto iter = m_pPrototypes.find(sGroupTag);
	if (iter != m_pPrototypes.end())
	{
		m_pPrototypes.erase(iter);
	}
}

const CPrototypeManager::PROTOTYPES* CPrototypeManager::GetPrototype(const StringID& svGroupTag) const
{
	auto iter = m_pPrototypes.find(svGroupTag);
	if (iter != m_pPrototypes.end())
	{
		return &iter->second;
	}
	return nullptr;
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
