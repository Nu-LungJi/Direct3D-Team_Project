#pragma once
#include "Engine_Defines.h"
#include "Resource.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResourceManager final: public CEngineBase
{
private:
	CResourceManager(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
	~CResourceManager() override;

public:
	void UpdateGUI();

public:
	void Initialize();
	void Release();

private:
	typedef std::unordered_map<StringID, std::vector<SPtr<CResource>>> RESOURCES;
	std::unordered_map<StringID, RESOURCES> m_Resources{};

public:
	SPtr<CResource> AddResource(const StringID& sGroupTag, const StringID& sResTag, _string_id eAssetType, const _string& sPath, void* pArg);
	SPtr<CResource> AddResource(const StringID& sGroupTag, const StringID& sResTag, SPtr<CResource> pAsset);
	template<typename T>
	SPtr<T> AddResourceT(const StringID& sGroupTag, const StringID& sResTag, const _string& sPath, void* pArg);
	template<typename T>
	SPtr<T> AddResourceT(const StringID& sGroupTag, const StringID& sResTag, SPtr<T> pAsset);
	template<typename T, typename CreateFunc>
	SPtr<T> GetOrCreateResourceByPath(const _string& sPath, CreateFunc&& createFunc);
	SPtr<CResource> GetResourceFirst(const StringID& sGroupTag, const StringID& sResTag) const;
	template<typename T>
	SPtr<T> GetResourceFirst(const StringID& sGroupTag, const StringID& sResTag) const;
	std::vector<SPtr<CResource>> GetResource(const StringID& sGroupTag, const StringID& sResTag) const;
	std::unordered_map<StringID, std::vector<SPtr<CResource>>> GetResource(const StringID& sGroupTag) const;
	std::unordered_map<StringID, RESOURCES> GetResources() const;

	void DelResource(const StringID& sGroupTag);
	void DelResource(const StringID& sGroupTag, const StringID& sResTag);

private:
	const std::vector<SPtr<CResource>>* GetResourcePtr(const StringID& sGroupTag, const StringID& sResTag) const { return _FindResource(sGroupTag, sResTag); };
	RESOURCES* FindGroup(const StringID& sGroupTag) ;
	const RESOURCES* FindGroup(const StringID& sGroupTag) const;
	std::vector<SPtr<CResource>>* _FindResource(const StringID& sGroupTag, const StringID& sResTag);
	const std::vector<SPtr<CResource>>* _FindResource(const StringID& sGroupTag, const StringID& sResTag) const;
	SPtr<CResource> CreateResource(_string_id eAssetType, const _string& sPath, void* pArg) const;

public:
	std::unordered_map<_string, std::vector<SPtr<CResource>>> GetResourcesByPath();
	std::vector<SPtr<CResource>> GetResourcesByPath(const _string& sPath);
	void RemovePathLookup(const _string& sPath, SPtr<CResource> pRes);
private:
	void _RemovePathLookup(const _string& sPath, SPtr<CResource> pRes);
	std::unordered_map<_string, std::vector<WPtr<CResource>>> m_PathLookup{};

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};

private:
	mutable std::shared_mutex m_Mutex{}; // (const 함수에서도 락을 걸기 위해 mutable 사용)

public:
	static UPtr<CResourceManager> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);

private:
	_bool m_bIsShutdown{ false };
	void Free() override;
};

NS_END

template<typename T>
inline Engine::SPtr<T> Engine::CResourceManager::GetResourceFirst(const StringID& sGroupTag, const StringID& sResTag) const
{
	auto base = GetResourceFirst(sGroupTag, sResTag);
	if (!base) return nullptr;

	if (!base->IsA(T::StaticType))
	{
		return nullptr;
	}

	//if (base->GetType() != T::StaticType)
	//	return nullptr;

	return std::static_pointer_cast<T>(base);
}

template<typename T>
inline Engine::SPtr<T> Engine::CResourceManager::AddResourceT(const StringID& sGroupTag, const StringID& sResTag, const _string& sPath, void* pArg)
{
	SPtr<CResource> pAdded = AddResource(sGroupTag, sResTag, T::StaticType, sPath, pArg);
	if (!pAdded)
	{
		return nullptr;
	}
	
	return std::static_pointer_cast<T>(pAdded);
}

template<typename T>
inline  Engine::SPtr<T> Engine::CResourceManager::AddResourceT(const StringID& sGroupTag, const StringID& sResTag, SPtr<T> pAsset)
{
	SPtr<CResource> pAdded = AddResource(sGroupTag, sResTag, pAsset);
	if (!pAdded)
	{
		return nullptr;
	}

	return std::static_pointer_cast<T>(pAdded);
}

template<typename T, typename CreateFunc>
inline Engine::SPtr<T> Engine::CResourceManager::GetOrCreateResourceByPath(const _string& sPath, CreateFunc&& createFunc)
{
	static_assert(std::is_base_of_v<CResource, T>);

	if (sPath.empty())
		return std::forward<CreateFunc>(createFunc)();

	const _string normalizedPath = std::filesystem::path{ sPath }.lexically_normal().generic_string();
	std::unique_lock<std::shared_mutex> lock{ m_Mutex };

	if (m_bIsShutdown)
		return nullptr;

	auto& cachedResources = m_PathLookup[normalizedPath];
	std::erase_if(cachedResources,
		[](const WPtr<CResource>& resource)
		{
			return resource.expired();
		});

	for (const auto& weakResource : cachedResources)
	{
		auto resource = weakResource.lock();
		if (resource && resource->IsA(T::StaticType))
			return std::static_pointer_cast<T>(resource);
	}

	auto resource = std::forward<CreateFunc>(createFunc)();
	if (!resource)
		return nullptr;

	cachedResources.emplace_back(resource);
	return resource;
}
