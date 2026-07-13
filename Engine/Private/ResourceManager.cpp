#include "pch.h"
#include "ResourceManager.h"
#include "ResFmodSound.h"
#include "ResJson.h"
#include "ResVertexShader.h"
#include "ResPixelShader.h"
#include "ResTessHullShader.h"
#include "ResComputeShader.h"
#include "ResTessDomainShader.h"
#include "ResGeometryShader.h"
#include "ResGeoShaderStreamOut.h"

NS_USING(Engine)
CResourceManager::CResourceManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
	: m_pDevice{ pDevice }
	, m_pContext{ pContext }
{

}

CResourceManager::~CResourceManager()
{
}

void CResourceManager::UpdateGUI()
{
	ImGui::Begin("CResourceManager");

	// =========================================================
	// 0. [NEW] 실시간 메모리 사용량 표시 (1초마다 갱신)
	// =========================================================
	static PROCESS_MEMORY_COUNTERS_EX pmc{};
	static float memUpdateTimer = 1.0f; // 처음 켤 때 바로 갱신되도록 1.0으로 초기화
	memUpdateTimer += ImGui::GetIO().DeltaTime;

	// 1초(1.0f)마다 메모리 정보 갱신
	if (memUpdateTimer >= 1.0f)
	{
		GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
		memUpdateTimer = 0.0f; // 타이머 초기화
	}

	const double MB = 1024.0 * 1024.0;
	ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[System Memory Usage]");
	ImGui::Text("  WS: %8.2f MB  |  Peak: %8.2f MB", pmc.WorkingSetSize / MB, pmc.PeakWorkingSetSize / MB);
	ImGui::Text("  Private: %8.2f MB  |  PageFile: %8.2f MB", pmc.PrivateUsage / MB, pmc.PagefileUsage / MB);
	ImGui::Separator();
	ImGui::Spacing();

	// =========================================================
	// 1. 전체 카운트 사전 계산
	// =========================================================
	size_t totalResCount = 0;
	for (const auto& Pair : m_Resources)
	{
		for (const auto& Pair2 : Pair.second)
		{
			totalResCount += Pair2.second.size();
		}
	}

	size_t totalPathKeys = m_PathLookup.size();
	size_t totalPathItems = 0;
	for (const auto& PathPair : m_PathLookup)
	{
		totalPathItems += PathPair.second.size();
	}

	// =========================================================
	// 2. 전체 통계 정보 표시 (맨 위)
	// =========================================================
	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[Total Resource Count] : %zu", totalResCount);
	ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "[PathLookup] Paths: %zu / Total Items: %zu", totalPathKeys, totalPathItems);
	ImGui::Separator();
	ImGui::Spacing(); // 약간의 여백

	// =========================================================
	// 3. 그룹 / 서브그룹 리소스 리스트 (중간)
	// =========================================================
	if (ImGui::CollapsingHeader("Group/SubGroup Resources", ImGuiTreeNodeFlags_DefaultOpen))
	{
		static ImGuiTextFilter filterGrp;
		static ImGuiTextFilter filterRes;

		// 그룹 리스트 바로 위에 해당 필터 배치
		filterGrp.Draw("GrpSearch");
		filterRes.Draw("ResSearch");

		for (const auto& Pair : m_Resources)
		{
			const char* groupName = Pair.first.GetDbgStr();

			if (!filterGrp.PassFilter(groupName))
				continue;

			size_t groupResCount = 0;
			for (const auto& Pair2 : Pair.second)
			{
				groupResCount += Pair2.second.size();
			}

			if (ImGui::TreeNode((void*)&Pair, "[%s] (Total: %zu)", groupName, groupResCount))
			{
				for (const auto& Pair2 : Pair.second)
				{
					const char* subName = Pair2.first.GetDbgStr();

					if (!filterRes.PassFilter(subName))
						continue;

					size_t subResCount = Pair2.second.size();

					if (ImGui::TreeNode((void*)&Pair2, "- %s (Count: %zu)", subName, subResCount))
					{
						for (size_t i = 0; i < subResCount; ++i)
						{
							ImGui::Text("%i-----", (int)i);
							Pair2.second[i]->UpdateGUI();
						}
						ImGui::TreePop();
					}
				}
				ImGui::TreePop();
			}
		}
	}

	ImGui::Spacing(); // 그룹과 패스 리스트 사이의 여백

	// =========================================================
	// 4. Path Lookup 리스트 (맨 아래)
	// =========================================================
	if (ImGui::CollapsingHeader("PathLookup List", ImGuiTreeNodeFlags_DefaultOpen))
	{
		static ImGuiTextFilter filterPath;

		// 패스 리스트 바로 위에 해당 필터 배치
		filterPath.Draw("PathSearch");

		for (const auto& PathPair : m_PathLookup)
		{
			const char* pathStr = PathPair.first.c_str();

			if (!filterPath.PassFilter(pathStr))
				continue;

			size_t pathResCount = PathPair.second.size();

			if (ImGui::TreeNode((void*)&PathPair, "[%s] (Count: %zu)", pathStr, pathResCount))
			{
				for (size_t i = 0; i < pathResCount; ++i)
				{
					ImGui::Text("%i-----", (int)i);
					PathPair.second[i]->UpdateGUI();
				}
				ImGui::TreePop();
			}
		}
	}

	ImGui::End();
}

void CResourceManager::Initialize()
{
	
}

const std::vector<CResource*>* CResourceManager::GetResourcesByPath(const _string& sPath) const
{
	auto it = m_PathLookup.find(sPath);

	// 경로가 맵에 존재하지 않으면 nullptr 반환
	if (it == m_PathLookup.end())
	{
		return nullptr;
	}

	// 찾은 벡터의 주소를 반환
	return &it->second;
}

void CResourceManager::RemovePathLookup(const _string& sPath, CResource* pRes)
{
	if (m_bIsShutdown)
		return;

	auto it = m_PathLookup.find(sPath);
	if (it != m_PathLookup.end())
	{
		auto& vec = it->second;

		// erase-remove idiom: 해당 포인터와 일치하는 것만 삭제
		vec.erase(std::remove(vec.begin(), vec.end(), pRes), vec.end());

		// 더 이상 해당 경로를 쓰는 리소스가 없으면 키 자체를 제거 (메모리 최적화)
		if (vec.empty())
		{
			m_PathLookup.erase(it);
		}
	}
}

SPtr<CResource> CResourceManager::AddResource(const StringID& sGroupTag, const StringID& sResTag,
	_string_id eAssetType, const _string& sPath, void* pArg)
{
	auto pCreatedAsset = CreateResource(eAssetType, sPath, pArg);
	if (!pCreatedAsset)
	{
		return nullptr;
	}

	auto addedResource = AddResource(sGroupTag, sResTag, pCreatedAsset);
	if (!addedResource)
	{
		return nullptr;
	};

	return addedResource;
}
SPtr<CResource> CResourceManager::AddResource(const StringID& sGroupTag, const StringID& sResTag, SPtr<CResource> pAsset)
{
	auto pGroup = FindGroup(sGroupTag);
	if (pGroup)
	{
		auto pAssetVec = _FindResource(sGroupTag, sResTag);
		if (pAssetVec)
		{
			pAssetVec->push_back(pAsset);
		}
		else
		{
			std::vector<SPtr<CResource>> newVecAsset{};
			newVecAsset.push_back(pAsset);
			pGroup->emplace(sResTag, newVecAsset);
		}
	}
	else
	{
		std::vector<SPtr<CResource>> newVecAsset{};
		newVecAsset.push_back(pAsset);

		CResourceManager::RESOURCES newAssets{};
		newAssets.emplace(sResTag, newVecAsset);

		m_Resources.emplace(sGroupTag, newAssets);
	}
	
	// RegisterPathLookup(CResource* pRes)
	{
		if (pAsset)
		{
			const _string& resPath = pAsset->GetPath();
			if (!resPath.empty())
			{
				// 동일한 경로를 가진 리소스가 여러 개일 수 있으므로 vector에 추가
				m_PathLookup[resPath].push_back(pAsset.get());
			}
		}
		
	}
	return pAsset;
}




SPtr<CResource> CResourceManager::GetResourceFirst(const StringID& sGroupTag, const StringID& sResTag) const
{
	if (auto p = GetResource(sGroupTag, sResTag))
	{
		if (!p->empty())
		{
			return (*p)[0];
		}
	}
	return nullptr;
}

//HRESULT CAsset_Manager::AddAsset(const _wstring& sGroupTag, const _wstring& sResTag, SPtr<CAsset> pAsset)
//{
//	auto pGroup = Find_Group(sGroupTag);
//	if (pGroup)
//	{
//		auto pAssetVec = Find_Asset(sGroupTag, sResTag);
//		if (pAssetVec)
//		{
//			pAssetVec->push_back(pAsset);
//		}
//		else
//		{
//			std::vector<SPtr<CAsset>> newVecAsset{};
//			newVecAsset.push_back(pAsset);
//			pGroup->emplace(sResTag, newVecAsset);
//		}
//	}
//	else
//	{
//		CAsset_Manager::ASSETS newAssets{};
//		m_Assets.emplace(sGroupTag, newAssets);
//		std::vector<SPtr<CAsset>> newVecAsset{};
//		newVecAsset.push_back(pAsset);
//		newAssets.emplace(sResTag, newVecAsset);
//	}
//
//	return S_OK;
//}

HRESULT CResourceManager::LoadResource(const StringID& sGroupTag)
{
	HRESULT hr = S_OK;
	for (auto& Pair : *FindGroup(sGroupTag))
	{
		if (FAILED(LoadResource(sGroupTag, Pair.first)))
		{
			hr = E_FAIL;
		}
	}

	return hr;
}

HRESULT CResourceManager::LoadResource(const StringID& sGroupTag, const StringID& sResTag)
{
	HRESULT hr = S_OK;
	for (auto& pAsset : *_FindResource(sGroupTag, sResTag))
	{
		if (FAILED(pAsset->Load()))
		{
			hr = E_FAIL;
		}
	}

	return hr;
}

HRESULT CResourceManager::UnLoadResource(const StringID& sGroupTag)
{
	HRESULT hr = S_OK;
	for (auto& Pair : *FindGroup(sGroupTag))
	{
		if (FAILED(UnLoadResource(sGroupTag, Pair.first)))
		{
			hr = E_FAIL;
		}
	}

	return hr;
}

HRESULT CResourceManager::UnLoadResource(const StringID& sGroupTag, const StringID& sResTag)
{
	HRESULT hr = S_OK;
	for (auto& pAsset : *_FindResource(sGroupTag, sResTag))
	{
		if (FAILED(pAsset->Unload()))
		{
			hr = E_FAIL;
		}
	}

	return hr;
}

void CResourceManager::DelResource(const StringID& sGroupTag)
{
	auto iter = m_Resources.find(sGroupTag);
	if (iter != m_Resources.end())
	{
		m_Resources.erase(iter);
	}
}

void CResourceManager::DelResource(const StringID& sGroupTag, const StringID& sResTag)
{
	auto pGroup = FindGroup(sGroupTag);
	if (pGroup)
	{
		auto iter = pGroup->find(sResTag);
		if (iter != pGroup->end())
		{
			pGroup->erase(iter);
		}
	}
	
}

CResourceManager::RESOURCES* CResourceManager::FindGroup(const StringID& sGroupTag)
{
	return const_cast<RESOURCES*>(static_cast<const CResourceManager*>(this)->FindGroup(sGroupTag));
}
const CResourceManager::RESOURCES* CResourceManager::FindGroup(const StringID& sGroupTag) const
{
	auto iter = m_Resources.find(sGroupTag);
	if (iter == m_Resources.end())
	{
		return nullptr;
	}
	return &iter->second;
}


std::vector<SPtr<CResource>>* CResourceManager::_FindResource(const StringID& sGroupTag, const StringID& sResTag)
{
	return const_cast<std::vector<SPtr<CResource>>*>(static_cast<const CResourceManager*>(this)->_FindResource(sGroupTag, sResTag));
}

const std::vector<SPtr<CResource>>* CResourceManager::_FindResource(const StringID& sGroupTag, const StringID& sResTag) const
{
	auto pGroup = FindGroup(sGroupTag);
	if (!pGroup)
	{
		return nullptr;
	}

	auto iter = pGroup->find(sResTag);
	if (iter == pGroup->end())
	{
		return nullptr;
	}

	return &iter->second;
}

SPtr<CResource> CResourceManager::CreateResource(_string_id eAssetType, const _string& sPath, void* pArg) const
{
	
	switch (eAssetType)
	{
	case CResFmodSound::StaticType:
		return CResFmodSound::Create(sPath);
	case CResJson::StaticType:
		return CResJson::Create(sPath);
	case CResVertexShader::StaticType:
		return CResVertexShader::Create(sPath);
	case CResPixelShader::StaticType:
		return CResPixelShader::Create(sPath);
	case CResGeometryShader::StaticType:
		return CResGeometryShader::Create(sPath);
	case CResTessHullShader::StaticType:
		return CResTessHullShader::Create(sPath);
	case CResTessDomainShader::StaticType:
		return CResTessDomainShader::Create(sPath);
	case CResComputeShader::StaticType:
		return CResComputeShader::Create(sPath);
	case CResGeoShaderStreamOut::StaticType:
		return CResGeoShaderStreamOut::Create(sPath);
	//case Engine::CAsset::TYPE::FMOD_SOUND:
	//	return CResFmodSound::Create(sPath);
	//case Engine::CAsset::TYPE::JSON:
	//	return CAssetJson::Create(sPath);
	//case Engine::CAsset::TYPE::VERTEX_SHADER:
	//	return CAssetVertexShader::Create(sPath, m_pDevice, m_pContext);
	//case Engine::CAsset::TYPE::PIXEL_SHADER:
	//	return CAssetPixelShader::Create(sPath, m_pDevice, m_pContext);
	}
	return nullptr;
}

UPtr<CResourceManager> CResourceManager::Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
{
	return ToUPtr(new CResourceManager{ pDevice , pContext });
}

void CResourceManager::Free()
{
	m_bIsShutdown = true;

	m_Resources.clear();

	CEngineBase::Free();
}
