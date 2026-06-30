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

	static ImGuiTextFilter filterGrp;
	static ImGuiTextFilter filterRes;
	filterGrp.Draw("GrpSearch");
	filterRes.Draw("ResSearch");

	for (const auto& Pair : m_Resources)
	{
		const char* groupName = Pair.first.GetDbgStr();

		if (!filterGrp.PassFilter(groupName))
			continue;

		if (ImGui::TreeNode(groupName))
		{
			for (const auto& Pair2 : Pair.second)
			{
				const char* subName = Pair2.first.GetDbgStr();

				if (!filterRes.PassFilter(subName))
					continue;

				if (ImGui::TreeNode(subName))
				{
					for (size_t i = 0; i < Pair2.second.size(); ++i)
					{
						// 필요하면 여기까지 필터 적용 가능
						ImGui::Text("%i-----", (int)i);
						Pair2.second[i]->UpdateGUI();
					}

					ImGui::TreePop();
				}
			}

			ImGui::TreePop();
		}
	}

	ImGui::End();
}

void CResourceManager::Initialize()
{
	
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
