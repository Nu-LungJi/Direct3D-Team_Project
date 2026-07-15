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
	// 0. 실시간 메모리 사용량 표시
	// =========================================================
	static PROCESS_MEMORY_COUNTERS_EX pmc{};
	static float memUpdateTimer = 1.0f;
	memUpdateTimer += ImGui::GetIO().DeltaTime;

	if (memUpdateTimer >= 1.0f)
	{
		GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
		memUpdateTimer = 0.0f;
	}

	const double MB = 1024.0 * 1024.0;
	ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[System Memory Usage]");
	ImGui::Text("  WS: %8.2f MB  |  Peak: %8.2f MB", pmc.WorkingSetSize / MB, pmc.PeakWorkingSetSize / MB);
	ImGui::Text("  Private: %8.2f MB  |  PageFile: %8.2f MB", pmc.PrivateUsage / MB, pmc.PagefileUsage / MB);
	ImGui::Separator();
	ImGui::Spacing();

	// =========================================================
	// 1. 통계 정보 계산
	// =========================================================
	{
		std::shared_lock<std::shared_mutex> lock(m_Mutex);// lock

		size_t totalResCount = 0;
		for (const auto& Pair : m_Resources)
			for (const auto& Pair2 : Pair.second)
				totalResCount += Pair2.second.size();

		size_t totalPathKeys = m_PathLookup.size();
		size_t totalPathItems = 0;
		for (const auto& PathPair : m_PathLookup)
			totalPathItems += PathPair.second.size();

		ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[Total Resource Count] : %zu", totalResCount);
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "[PathLookup] Paths: %zu / Total Items: %zu", totalPathKeys, totalPathItems);
		ImGui::Separator();
		ImGui::Spacing();
	}

	// =========================================================
	// 2. [수정] 삭제 예약 버퍼 (반복자 무효화 방지)
	// =========================================================
	std::vector<StringID> groupsToDelete;
	std::vector<std::pair<StringID, StringID>> subGroupsToDelete;

	// =========================================================
	// 3. 그룹 / 서브그룹 리소스 리스트
	// =========================================================
	{
		std::shared_lock<std::shared_mutex> lock(m_Mutex);// lock

		if (ImGui::CollapsingHeader("Group/SubGroup Resources", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static ImGuiTextFilter filterGrp;
			static ImGuiTextFilter filterRes;

			filterGrp.Draw("GrpSearch");
			filterRes.Draw("ResSearch");

			for (auto& Pair : m_Resources)
			{
				const char* groupName = Pair.first.GetDbgStr();

				if (!filterGrp.PassFilter(groupName))
					continue;

				// 하위 검색 필터링 체크
				bool bAnyChildMatches = filterRes.IsActive() == false;
				if (!bAnyChildMatches)
				{
					for (auto& Pair2 : Pair.second)
					{
						if (filterRes.PassFilter(Pair2.first.GetDbgStr()))
						{
							bAnyChildMatches = true; break;
						}
					}
				}
				if (!bAnyChildMatches) continue;

				// 노드 그리기
				size_t groupResCount = 0;
				for (const auto& Pair2 : Pair.second) groupResCount += Pair2.second.size();

				bool bGroupOpen = ImGui::TreeNode((void*)&Pair, "[%s] (Total: %zu)", groupName, groupResCount);

				ImGui::SameLine();
				ImGui::PushID(&Pair);
				if (ImGui::SmallButton("Clear Group"))
				{
					// 즉시 삭제하지 않고 리스트에 담음
					groupsToDelete.push_back(Pair.first);
				}
				ImGui::PopID();

				if (bGroupOpen)
				{
					for (auto& Pair2 : Pair.second)
					{
						const char* subName = Pair2.first.GetDbgStr();
						if (!filterRes.PassFilter(subName))
							continue;

						bool bSubOpen = ImGui::TreeNode((void*)&Pair2, "- %s (Count: %zu)", subName, Pair2.second.size());

						ImGui::SameLine();
						ImGui::PushID(&Pair2);
						if (ImGui::SmallButton("Clear"))
						{
							// 즉시 삭제하지 않고 리스트에 담음
							subGroupsToDelete.push_back({ Pair.first, Pair2.first });
						}
						ImGui::PopID();

						if (bSubOpen)
						{
							for (size_t i = 0; i < Pair2.second.size(); ++i)
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
	}

	// =========================================================
	// 4. [수정] 루프가 끝난 뒤 안전하게 삭제 실행
	// =========================================================
	for (const auto& gID : groupsToDelete)
		DelResource(gID); // 내부에서 RemovePathLookup을 호출하여 PathLookup까지 동기화함

	for (const auto& pairID : subGroupsToDelete)
		DelResource(pairID.first, pairID.second);

	// =========================================================
	// 5. Path Lookup 리스트
	// =========================================================
	ImGui::Spacing();
	if (ImGui::CollapsingHeader("PathLookup List", ImGuiTreeNodeFlags_DefaultOpen))
	{
		std::shared_lock<std::shared_mutex> lock(m_Mutex);

		static ImGuiTextFilter filterPath;
		filterPath.Draw("PathSearch");

		// 직접 m_PathLookup을 순회합니다. (매우 빠르고 가벼움)
		for (auto& PathPair : m_PathLookup)
		{
			const _string& path = PathPair.first;
			const auto& weakVec = PathPair.second;

			if (!filterPath.PassFilter(path.c_str()))
				continue;

			// 1. 살아있는 리소스 개수 먼저 파악 (노드 레이블에 표시할 Count용)
			size_t liveCount = 0;
			for (const auto& wp : weakVec)
			{
				if (!wp.expired()) liveCount++;
			}

			// 2. 살아있는 리소스가 있을 때만 그리기
			if (liveCount > 0)
			{
				// [핵심] 첫 번째 인자로 경로 문자열 포인터(path.c_str())를 전달하여 ID를 고정합니다.
				// 이렇게 하면 리스트가 새로 그려져도 ImGui가 "아, 이 노드구나" 하고 상태를 기억합니다.
				if (ImGui::TreeNode(path.c_str(), "[%s] (Count: %zu)", path.c_str(), liveCount))
				{
					for (size_t i = 0; i < weakVec.size(); ++i)
					{
						// 3. lock()을 통해 즉석에서 살아있는지 확인
						if (SPtr<CResource> pRes = weakVec[i].lock())
						{
							ImGui::Text("%i-----", (int)i);
							pRes->UpdateGUI();
						}
					}
					ImGui::TreePop();
				}
			}
		}
	}

	ImGui::End();
}

void CResourceManager::Initialize()
{
	
}

std::vector<SPtr<CResource>> CResourceManager::GetResource(const StringID& sGroupTag, const StringID& sResTag) const
{
	std::shared_lock<std::shared_mutex> lock(m_Mutex);

	if (auto p = _FindResource(sGroupTag, sResTag))
		return *p;
	return {};
}

std::unordered_map<StringID, std::vector<SPtr<CResource>>> CResourceManager::GetResource(const StringID& sGroupTag) const
{
	std::shared_lock<std::shared_mutex> lock(m_Mutex);

	if (auto p = FindGroup(sGroupTag))
		return *p;
	return {};
}

std::unordered_map<StringID, CResourceManager::RESOURCES> CResourceManager::GetResources() const
{
	std::shared_lock<std::shared_mutex> lock(m_Mutex); 
	return m_Resources;
}

std::unordered_map<_string, std::vector<SPtr<CResource>>> CResourceManager::GetResourcesByPath() 
{
	std::unique_lock<std::shared_mutex> lock(m_Mutex);

	// [주의] 전체 순회하며 수정하므로 비용이 큽니다.
		// [중요] std::unique_lock<std::shared_mutex> lock(m_Mutex); 필수!

	std::unordered_map<_string, std::vector<SPtr<CResource>>> result;

	for (auto it = m_PathLookup.begin(); it != m_PathLookup.end(); )
	{
		auto& vec = it->second;

		// 1. 해당 경로의 벡터에서 죽은 것들 정리
		vec.erase(
			std::remove_if(vec.begin(), vec.end(), [](const std::weak_ptr<CResource>& wp) {
				return wp.expired();
				}),
			vec.end()
		);

		// 2. 살아있는 것들만 결과에 담기
		for (const auto& weakRes : vec)
		{
			if (SPtr<CResource> sharedRes = weakRes.lock())
			{
				result[it->first].push_back(sharedRes);
			}
		}

		// 3. 빈 경로면 키 삭제 및 반복자 처리
		if (vec.empty())
			it = m_PathLookup.erase(it);
		else
			++it;
	}

	return result;
}

std::vector<SPtr<CResource>> CResourceManager::GetResourcesByPath(const _string& sPath) 
{
	std::unique_lock<std::shared_mutex> lock(m_Mutex);

	auto it = m_PathLookup.find(sPath);
	if (it == m_PathLookup.end())
		return {};

	auto& vec = it->second;

	// Erase-Remove Idiom으로 죽은(expired) 것들 한 번에 정리
	vec.erase(
		std::remove_if(vec.begin(), vec.end(), [](const std::weak_ptr<CResource>& wp) {
			return wp.expired(); // lock()보다 expired()가 더 빠르고 명확합니다.
			}),
		vec.end()
	);

	// 결과 벡터 생성
	std::vector<SPtr<CResource>> result;
	result.reserve(vec.size());
	for (const auto& weakRes : vec)
	{
		if (SPtr<CResource> sharedRes = weakRes.lock())
		{
			result.push_back(sharedRes);
		}
	}

	// 경로에 더 이상 남은 리소스가 없으면 키 삭제
	if (vec.empty())
		m_PathLookup.erase(it);

	return result;
}

void CResourceManager::RemovePathLookup(const _string& sPath, SPtr<CResource> pRes)
{
	std::unique_lock<std::shared_mutex> lock(m_Mutex); 
	_RemovePathLookup(sPath, pRes);
}

void CResourceManager::_RemovePathLookup(const _string& sPath, SPtr<CResource> pRes)
{
	if (m_bIsShutdown)
		return;

	auto it = m_PathLookup.find(sPath);
	if (it != m_PathLookup.end())
	{
		auto& vec = it->second;

		// erase-remove idiom: 해당 포인터와 일치하는 것만 삭제
		vec.erase(
			std::remove_if(vec.begin(), vec.end(),
				[&pRes](const std::weak_ptr<CResource>& wp) {
					SPtr<CResource> locked = wp.lock();
					// 1. 찾고자 하는 타겟(pRes)이거나
					// 2. 이미 메모리에서 해제되어 껍데기만 남은 weak_ptr(nullptr)이면 같이 지워라!
					return locked == pRes || locked == nullptr;
				}
			),
			vec.end()
		);

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
	std::unique_lock<std::shared_mutex> lock(m_Mutex);

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
				m_PathLookup[resPath].push_back(pAsset);
			}
		}
		
	}
	return pAsset;
}




SPtr<CResource> CResourceManager::GetResourceFirst(const StringID& sGroupTag, const StringID& sResTag) const
{
	std::shared_lock<std::shared_mutex> lock(m_Mutex);

	if (auto p = GetResourcePtr(sGroupTag, sResTag))
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


//HRESULT CResourceManager::LoadResource(const StringID& sGroupTag)
//{
//	HRESULT hr = S_OK;
//	for (auto& Pair : *FindGroup(sGroupTag))
//	{
//		if (FAILED(LoadResource(sGroupTag, Pair.first)))
//		{
//			hr = E_FAIL;
//		}
//	}
//
//	return hr;
//}
//
//HRESULT CResourceManager::LoadResource(const StringID& sGroupTag, const StringID& sResTag)
//{
//	HRESULT hr = S_OK;
//	for (auto& pAsset : *_FindResource(sGroupTag, sResTag))
//	{
//		if (FAILED(pAsset->Load()))
//		{
//			hr = E_FAIL;
//		}
//	}
//
//	return hr;
//}

//HRESULT CResourceManager::UnLoadResource(const StringID& sGroupTag)
//{
//	HRESULT hr = S_OK;
//	for (auto& Pair : *FindGroup(sGroupTag))
//	{
//		if (FAILED(UnLoadResource(sGroupTag, Pair.first)))
//		{
//			hr = E_FAIL;
//		}
//	}
//
//	return hr;
//}
//
//HRESULT CResourceManager::UnLoadResource(const StringID& sGroupTag, const StringID& sResTag)
//{
//	HRESULT hr = S_OK;
//	for (auto& pAsset : *_FindResource(sGroupTag, sResTag))
//	{
//		if (FAILED(pAsset->Unload()))
//		{
//			hr = E_FAIL;
//		}
//	}
//
//	return hr;
//}

void CResourceManager::DelResource(const StringID& sGroupTag)
{
	std::unique_lock<std::shared_mutex> lock(m_Mutex);

	auto iter = m_Resources.find(sGroupTag);
	if (iter != m_Resources.end())
	{
		// 1. 해당 그룹 내의 모든 리소스에 대해 RemovePathLookup 호출
		for (auto& subGroup : iter->second)
		{
			for (auto& res : subGroup.second)
			{
				if (res)
				{
					// 리소스의 경로를 찾아서 PathLookup에서 제거 요청
					_RemovePathLookup(res->GetPath(), res);
				}
			}
		}

		// 2. 이제 안전하게 메인 저장소에서 삭제
		m_Resources.erase(iter);
	}
}

void CResourceManager::DelResource(const StringID& sGroupTag, const StringID& sResTag)
{
	std::unique_lock<std::shared_mutex> lock(m_Mutex);

	auto pGroup = FindGroup(sGroupTag);
    if (pGroup)
    {
        auto iter = pGroup->find(sResTag);
        if (iter != pGroup->end())
        {
            // 1. 해당 리소스 벡터 내의 모든 리소스에 대해 RemovePathLookup 호출
            for (auto& res : iter->second)
            {
                if (res)
                {
                    _RemovePathLookup(res->GetPath(), res);
                }
            }

            // 2. 메인 저장소에서 삭제
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
	CEngineBase::Free();
}

void CResourceManager::Release()
{
	std::unique_lock<std::shared_mutex> lock(m_Mutex);

	// 루아매니저 와처 타이밍이슈
	m_bIsShutdown = true;

	m_Resources.clear();
	m_PathLookup.clear();
}
