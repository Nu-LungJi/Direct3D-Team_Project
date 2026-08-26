#include "pch.h"
#include "MapModelResourceTracker.h"

#include "ResModelMaterial.h"
#include "MapStaticModelLoader.h"

NS_USING(Engine)

void CMapModelResourceTracker::SetResourceIndex(
	const std::filesystem::path& staticModelRoot,
	const std::string& resourceGroup,
	std::unordered_map<std::string, std::filesystem::path> modelPaths)
{
	{
		std::lock_guard<std::mutex> lock(m_ResourceIndexMutex);
		m_StaticModelRoot = staticModelRoot;
		m_ResourceGroup = resourceGroup;
		m_ModelPaths = std::move(modelPaths);
	}

	// 맵 진입 시 한 번 구축하여 스트리밍 워커의 첫 텍스처 검색 비용을 제거한다.
	CResModelMaterial::WarmUpTextureSearchIndex(staticModelRoot);
}

HRESULT CMapModelResourceTracker::AcquireChunkResources(
	CMapChunk& chunk,
	const std::vector<MAP_MESH_OBJECT_FILE_DATA>& objects)
{
	const MODEL_KEY_SET uniqueModels = CollectUniqueModelKeys(objects);
	auto& chunkResources = chunk.GetModelResources();
	chunkResources.clear();
	chunkResources.reserve(uniqueModels.size());

	for (const auto& key : uniqueModels)
	{
		auto resourceMutex = GetResourceMutex(key);
		std::lock_guard<std::mutex> resourceLock(*resourceMutex);

		if (FAILED(EnsureResourceLoaded(key)))
		{
			DecreaseChunkReferences(chunkResources, m_DeferredUnusedCandidates);
			chunkResources.clear();
			return E_FAIL;
		}

		++m_ChunkReferenceCounts[key];
		chunkResources.push_back(key);
	}

	return S_OK;
}

HRESULT CMapModelResourceTracker::PreloadResources(
	const std::vector<MAP_MESH_OBJECT_FILE_DATA>& objects,
	std::vector<MAP_MODEL_RESOURCE_KEY>& outResources)
{
	ZoneScopedN("ChunkResourcePreloadWorker");
	const MODEL_KEY_SET uniqueModels = CollectUniqueModelKeys(objects);
	outResources.clear();
	outResources.reserve(uniqueModels.size());

	for (const auto& key : uniqueModels)
	{
		auto resourceMutex = GetResourceMutex(key);
		std::lock_guard<std::mutex> resourceLock(*resourceMutex);
		if (FAILED(EnsureResourceLoaded(key)))
			return E_FAIL;

		outResources.push_back(key);
		std::lock_guard<std::mutex> pendingLock(m_PendingReferenceMutex);
		++m_PendingReferenceCounts[key];
	}

	return S_OK;
}

HRESULT CMapModelResourceTracker::CommitPreloadedResources(
	CMapChunk& chunk,
	const std::vector<MAP_MODEL_RESOURCE_KEY>& resources)
{
	for (const auto& key : resources)
	{
		auto model = CGameInstance::Get().GetResourceFirst<CResStaticModel>(key.group, key.tag);
		if (!model || model->GetState() != CResource::STATE::LOADED)
		{
			ReleasePreloadedResources(resources);
			return E_FAIL;
		}
	}

	DecreasePendingReferences(resources);
	chunk.SetModelResources(resources);
	for (const auto& key : chunk.GetModelResources())
		++m_ChunkReferenceCounts[key];

	return S_OK;
}

void CMapModelResourceTracker::ReleasePreloadedResources(
	const std::vector<MAP_MODEL_RESOURCE_KEY>& resources)
{
	DecreasePendingReferences(resources);
	m_DeferredUnusedCandidates.insert(
		m_DeferredUnusedCandidates.end(), resources.begin(), resources.end());
}

void CMapModelResourceTracker::QueueChunkRelease(CMapChunk& chunk)
{
	if (!chunk.GetModelResources().empty())
		m_DeferredChunkReleases.push_back(chunk.TakeModelResources());
}

void CMapModelResourceTracker::QueueAllChunkReleases(CHUNK_MAP& chunks)
{
	for (auto& chunkEntry : chunks)
		QueueChunkRelease(chunkEntry.second);
}

void CMapModelResourceTracker::ProcessDeferredReleases()
{
	if (m_DeferredChunkReleases.empty() && m_DeferredUnusedCandidates.empty())
		return;

	auto chunkReleases = std::move(m_DeferredChunkReleases);
	m_DeferredChunkReleases.clear();
	auto unusedCandidates = std::move(m_DeferredUnusedCandidates);
	m_DeferredUnusedCandidates.clear();

	for (const auto& resources : chunkReleases)
		DecreaseChunkReferences(resources, unusedCandidates);

	for (const auto& key : unusedCandidates)
	{
		if (m_ChunkReferenceCounts.contains(key))
			continue;

		auto resourceMutex = GetResourceMutex(key);
		std::unique_lock<std::mutex> resourceLock(*resourceMutex, std::try_to_lock);
		if (!resourceLock.owns_lock())
		{
			m_DeferredUnusedCandidates.push_back(key);
			continue;
		}

		{
			std::lock_guard<std::mutex> pendingLock(m_PendingReferenceMutex);
			if (m_PendingReferenceCounts.contains(key))
				continue;
		}

		auto model = CGameInstance::Get().GetResourceFirst<CResStaticModel>(key.group, key.tag);
		if (!model)
			continue;

		CGameInstance::Get().EraseMapMeshTextureCache(model);
		model->Unload();
		CGameInstance::Get().DelResource(key.group, key.tag);
	}
}

CMapModelResourceTracker::MODEL_KEY_SET CMapModelResourceTracker::CollectUniqueModelKeys(
	const std::vector<MAP_MESH_OBJECT_FILE_DATA>& objects) const
{
	MODEL_KEY_SET uniqueModels;
	for (const auto& object : objects)
	{
		if (!object.modelGroup.empty() && !object.model.empty())
			uniqueModels.insert({ object.modelGroup, object.model });
	}
	return uniqueModels;
}

HRESULT CMapModelResourceTracker::EnsureResourceLoaded(const MAP_MODEL_RESOURCE_KEY& key)
{
	if (auto model = CGameInstance::Get().GetResourceFirst<CResStaticModel>(key.group, key.tag))
	{
		if (model->GetState() == CResource::STATE::LOADED)
			return S_OK;
		if (model->GetState() != CResource::STATE::UNLOAD &&
			model->GetState() != CResource::STATE::LOADFAIL)
			return E_FAIL;
	}

	std::filesystem::path staticModelRoot;
	std::filesystem::path modelPath;
	{
		std::lock_guard<std::mutex> lock(m_ResourceIndexMutex);
		if (key.group != m_ResourceGroup)
			return E_FAIL;

		const auto modelPathIter = m_ModelPaths.find(key.tag);
		if (modelPathIter == m_ModelPaths.end())
			return E_FAIL;

		staticModelRoot = m_StaticModelRoot;
		modelPath = modelPathIter->second;
	}

	return LoadMapStaticModelFile(
		modelPath,
		staticModelRoot,
		key.group,
		nullptr,
		key.tag)
		? S_OK
		: E_FAIL;
}

SPtr<std::mutex> CMapModelResourceTracker::GetResourceMutex(
	const MAP_MODEL_RESOURCE_KEY& key)
{
	std::lock_guard<std::mutex> lock(m_ResourceMutexMapMutex);
	auto& resourceMutex = m_ResourceMutexes[key];
	if (!resourceMutex)
		resourceMutex = std::make_shared<std::mutex>();
	return resourceMutex;
}

void CMapModelResourceTracker::DecreaseChunkReferences(
	const std::vector<MAP_MODEL_RESOURCE_KEY>& resources,
	std::vector<MAP_MODEL_RESOURCE_KEY>& outUnusedCandidates)
{
	for (const auto& key : resources)
	{
		const auto referenceIter = m_ChunkReferenceCounts.find(key);
		if (referenceIter == m_ChunkReferenceCounts.end())
			continue;

		if (referenceIter->second > 1)
		{
			--referenceIter->second;
			continue;
		}

		m_ChunkReferenceCounts.erase(referenceIter);
		outUnusedCandidates.push_back(key);
	}
}

void CMapModelResourceTracker::DecreasePendingReferences(
	const std::vector<MAP_MODEL_RESOURCE_KEY>& resources)
{
	std::lock_guard<std::mutex> pendingLock(m_PendingReferenceMutex);
	for (const auto& key : resources)
	{
		auto referenceIter = m_PendingReferenceCounts.find(key);
		if (referenceIter == m_PendingReferenceCounts.end())
			continue;

		if (referenceIter->second > 1)
			--referenceIter->second;
		else
			m_PendingReferenceCounts.erase(referenceIter);
	}
}
