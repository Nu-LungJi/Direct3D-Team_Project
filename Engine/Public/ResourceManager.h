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

	SPtr<CResource> GetResourceFirst(const StringID& sGroupTag, const StringID& sResTag) const;
	template<typename T>
	SPtr<T> GetResourceFirst(const StringID& sGroupTag, const StringID& sResTag) const;

	std::vector<SPtr<CResource>>* GetResource(const StringID& sGroupTag, const StringID& sResTag) { return _FindResource(sGroupTag, sResTag); };
	const std::vector<SPtr<CResource>>* GetResource(const StringID& sGroupTag, const StringID& sResTag) const { return _FindResource(sGroupTag, sResTag); };
	std::unordered_map<StringID, std::vector<SPtr<CResource>>>* GetResource(const StringID& sGroupTag) { return FindGroup(sGroupTag); };
	const std::unordered_map<StringID, std::vector<SPtr<CResource>>>* GetResource(const StringID& sGroupTag) const { return FindGroup(sGroupTag); };
	const std::unordered_map<StringID, RESOURCES>& GetResources() const { return m_Resources; }
	HRESULT LoadResource(const StringID& sGroupTag);
	HRESULT LoadResource(const StringID& sGroupTag, const StringID& sResTag);
	HRESULT UnLoadResource(const StringID& sGroupTag);
	HRESULT UnLoadResource(const StringID& sGroupTag, const StringID& sResTag);
	void DelResource(const StringID& sGroupTag);
	void DelResource(const StringID& sGroupTag, const StringID& sResTag);

private:
	RESOURCES* FindGroup(const StringID& sGroupTag) ;
	const RESOURCES* FindGroup(const StringID& sGroupTag) const;
	std::vector<SPtr<CResource>>* _FindResource(const StringID& sGroupTag, const StringID& sResTag);
	const std::vector<SPtr<CResource>>* _FindResource(const StringID& sGroupTag, const StringID& sResTag) const;
	SPtr<CResource> CreateResource(_string_id eAssetType, const _string& sPath, void* pArg) const;

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};

public:
	static UPtr<CResourceManager> Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext);
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
