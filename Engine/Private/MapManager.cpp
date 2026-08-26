#include "pch.h"
#include "MapManager.h"
#include "MapMeshObject.h"
#include "DecalVolume.h"
#include <filesystem>
#include <set>

#include "OctreeNode.h"
#include "ComStaticModelInstance.h"
#ifdef _DEBUG
#include "MapChunkDebugRenderer.h"
#endif
NS_USING(Engine)

namespace
{
	//constexpr _float3 DEFAULT_MAP_CHUNK_SIZE{ 150.f, 150.f, 150.f };

	_bool IsSameChunkSize(const _float3& lhs, const _float3& rhs)
	{
		return std::fabs(lhs.x - rhs.x) 
			&& std::fabs(lhs.y - rhs.y)
			&& std::fabs(lhs.z - rhs.z);
	}

}

CMapManager::CMapManager()
{
}

CMapManager::~CMapManager() = default;

HRESULT CMapManager::Initialize()
{
	if (FAILED(m_ChunkStreamer.Initialize(m_Chunks, m_ChunkSerializer, m_ModelResourceTracker, m_ObjectFactory,
		[this](const MAPCHUNK_COORD& coord) { return UnLoadChunk(coord); })))
	{
		return E_FAIL;
	}

#ifdef _DEBUG
	m_ChunkDebugRenderer = std::make_unique<CMapChunkDebugRenderer>();
	if (FAILED(m_ChunkDebugRenderer->Initialize()))
		return E_FAIL;
#endif

	return S_OK;
}

void CMapManager::Update(_float)
{
	m_ModelResourceTracker.ProcessDeferredReleases();
	m_ChunkStreamer.Update(m_MapRootPath, m_ChunkSize);
}

void CMapManager::ClearAllChunk()
{
	m_ChunkStreamer.InvalidatePendingLoads();
	CGameInstance::Get().ClearMapMeshResidentChunks();
	m_ModelResourceTracker.QueueAllChunkReleases(m_Chunks);
	m_Chunks.clear();
}

void CMapManager::SetMapModelResourceIndex(const std::filesystem::path& staticModelRoot, const std::string& resourceGroup, std::unordered_map<std::string, std::filesystem::path> modelPaths)
{
	m_ModelResourceTracker.SetResourceIndex(staticModelRoot, resourceGroup, std::move(modelPaths));
}

HRESULT CMapManager::SaveMap(const std::string& path)
{
	RebuildChunks();

	const std::filesystem::path mapDir(path);
	const std::filesystem::path chunkDir = mapDir / "chunks";
	std::error_code ec;
	std::filesystem::create_directories(chunkDir, ec);
	if (ec)
	{
		return E_FAIL;
	}

	MAP_FILE_DATA mapFileData{};
	mapFileData.chunkSize = m_ChunkSize;

	const auto& layers = CGameInstance::Get().GetGameObjectLayers();

	std::vector<MAPCHUNK_COORD> originallyUnloadedChunks;
	for (auto& [coord, chunk] : m_Chunks)
	{
		if (chunk.GetLoadState() == EChunkLoadState::Unloaded &&
			chunk.GetSaveState() != EChunkSaveState::Unsaved)
		{
			originallyUnloadedChunks.push_back(coord);
			// 저장 중에는 파일 내용이 필요하므로 청크를 동기식으로 불러와야
			if (FAILED(LoadChunk(coord)))
			{
				return E_FAIL;
			}
		}
	}

	for (auto& [coord, chunk] : m_Chunks)
	{
		const std::string fileName = m_ChunkSerializer.MakeChunkFileName(coord);
		const std::filesystem::path relativePath = std::filesystem::path("chunks") / fileName;
		const std::filesystem::path chunkPath = mapDir / relativePath;

		chunk.SetFilePath(relativePath.generic_string());

		if (FAILED(SaveChunk(coord, chunkPath.generic_string())))
		{
			return E_FAIL;
		}

		chunk.SetSaveState(EChunkSaveState::Saved);

		mapFileData.chunks.push_back(
		{
			coord,
			chunk.GetFilePath(),
			chunk.GetObjectHandles().size()
		});
	}

	std::set<std::pair<std::string, std::string>> requiredModels;
	for (const auto& pair : layers)
	{
		const auto& objects = pair.second;

		for(const auto& objectHandle : objects)
		{
			if (auto* decal = CGameInstance::Get().GetGameObjectByHandleT<CDecalVolume>(objectHandle))
			{
				mapFileData.decals.push_back(m_ObjectFactory.MakeDecalFileData(*decal, pair.first));
				continue;
			}

			CMapMeshObject* pMeshObj = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(objectHandle);

			if (pMeshObj == nullptr)
				continue;

			requiredModels.emplace(pMeshObj->GetModelResourceGroup(), pMeshObj->GetModelResourceTag());
		}
	}

	for (const auto& [modelGroup, model] : requiredModels)
	{
		mapFileData.requiredModels.push_back({ modelGroup, model });
	}

	for (const auto& coord : originallyUnloadedChunks)
	{
		UnLoadChunk(coord);
	}

	if (FAILED(m_ChunkSerializer.SaveMapFile(mapDir / "map.json", mapFileData)))
		return E_FAIL;

	// 런타임에서 수정된 머티리얼도 맵과 함께 저장
	SaveMaterial(mapDir.string());
	return S_OK;
}

HRESULT CMapManager::LoadMap(const std::string& path, _bool clearBeforeLoad)
{
	if (clearBeforeLoad)
	{
		m_ChunkStreamer.InvalidatePendingLoads();
		CGameInstance::Get().DelGameObjectLayer(E::MAPMESHOBJECTLAYER);
		m_ModelResourceTracker.QueueAllChunkReleases(m_Chunks);
		CGameInstance::Get().DelGameObjectLayer(E::MAPDECALOBJECTLAYER);
		m_Chunks.clear();
	}

	const std::filesystem::path mapDir(path);

	// 저장된 머티리얼을 먼저 복원해 이후 생성되는 모델에 적용할 수 있게 함
	LoadMaterial(mapDir.string());

	std::filesystem::path mapFilePath = mapDir / "map.json";
	if (std::filesystem::exists(mapFilePath))
	{
		const _float3 requestedChunkSize = DEFAULT_MAP_CHUNK_SIZE;

		if (FAILED(LoadMapData(path)))
		{
			return E_FAIL;
		}

		if (!IsSameChunkSize(m_ChunkSize, requestedChunkSize))
		{
			std::vector<MAPCHUNK_COORD> oldChunkCoords;
			oldChunkCoords.reserve(m_Chunks.size());

			for (const auto& [coord, chunk] : m_Chunks)
			{
				oldChunkCoords.push_back(coord);
			}

			for (const MAPCHUNK_COORD& coord : oldChunkCoords)
			{
				if (FAILED(LoadChunk(coord)))
					return E_FAIL;
			}

			m_ChunkSize = requestedChunkSize;
			m_ChunkStreamer.InvalidatePendingLoads();
			m_ModelResourceTracker.QueueAllChunkReleases(m_Chunks);
			m_Chunks.clear();
			RebuildChunks();

			if (FAILED(SaveMap(path)))
				return E_FAIL;

			// 마이그레이션에는 해당 객체가 일시적으로만 필요
			// 일반적인 스트리밍 맵 로드 시 사용되는 것과 동일한, 메타데이터 전용 상태로 복원
			CGameInstance::Get().DelGameObjectLayer(E::MAPMESHOBJECTLAYER);
			m_ChunkStreamer.InvalidatePendingLoads();
			m_ModelResourceTracker.QueueAllChunkReleases(m_Chunks);
			m_Chunks.clear();
			if (FAILED(LoadMapData(path)))
				return E_FAIL;
		}

		std::vector<MAPCHUNK_COORD> chunkCoords;
		chunkCoords.reserve(m_Chunks.size());
		for (const auto& [coord, chunk] : m_Chunks)
		{
			chunkCoords.push_back(coord);
		}

		// 메타데이터만 복원하고 실제 청크 로드는 Streamer가 카메라 위치에 따라 요청
		return S_OK;
	}

	mapFilePath = mapDir / "TestMap.json";
	std::vector<MAP_MESH_OBJECT_FILE_DATA> legacyObjects;
	if (FAILED(m_ChunkSerializer.LoadLegacyMapFile(mapFilePath, legacyObjects)))
		return E_FAIL;

	std::unordered_map<MAPCHUNK_COORD, std::vector<MAP_MESH_OBJECT_FILE_DATA>, tagMapChunkCoordHash> legacyObjectsByChunk;
	for (auto& object : legacyObjects)
	{
		legacyObjectsByChunk[WorldToChunkCoord(object.position)].push_back(std::move(object));
	}

	for (auto& [coord, objects] : legacyObjectsByChunk)
	{
		auto& chunk = m_Chunks[coord];
		chunk.SetCoord(coord);
		chunk.SetBounds(MakeChunkBoundingBox(coord));
		chunk.BeginLoading();
		if (FAILED(m_ModelResourceTracker.AcquireChunkResources(chunk, objects)))
		{
			chunk.CancelLoading();
			return E_FAIL;
		}

		for (const auto& objectDesc : objects)
		{
			CMapMeshObject::MAP_MESH_OBJECT_DESC desc{};
			desc.sObjectTag = objectDesc.objectTag;
			desc.protoGroupTag = objectDesc.protoGroup;
			desc.prototypeTag = objectDesc.prototype;
			desc.modelGroupTag = objectDesc.modelGroup;
			desc.modelResTag = objectDesc.model;
			desc.windDesc = objectDesc.windDesc;

			auto hObject = CGameInstance::Get().AddGameObjectToLayer(desc.protoGroupTag, desc.prototypeTag, objectDesc.layer, &desc);
			if (!hObject)
				continue;

			auto* object = CGameInstance::Get().GetGameObjectByHandle(hObject.value());
			if (!object)
				continue;

			object->GetTransform().SetPosition(objectDesc.position);
			object->GetTransform().SetQuaternion(objectDesc.rotation);
			object->GetTransform().SetScale(objectDesc.scale);
			chunk.AddObject(hObject.value());
		}

		chunk.CompleteLoading(MakeChunkBoundingBox(coord), EChunkSaveState::Saved);
		if (FAILED(CGameInstance::Get().RegisterMapMeshResidentChunk(coord, chunk.GetObjectHandles())))
		{
			return E_FAIL;
		}
	}
	return S_OK;
}

HRESULT CMapManager::SaveChunk(const MAPCHUNK_COORD& coord, const std::string& chunkPath)
{
	const auto iter = m_Chunks.find(coord);
	if (iter == m_Chunks.end())
	{
		return E_FAIL;
	}

	const CMapChunk& chunk = iter->second;
	MAP_CHUNK_FILE_DATA chunkFileData{};
	chunkFileData.coord = coord;
	chunkFileData.bounds = chunk.GetBounds();

	const auto& layers = CGameInstance::Get().GetGameObjectLayers();
	for (const auto& [layerName, layer] : layers)
	{
		for (const auto& handle : layer)
		{
			if (!chunk.ContainsObject(handle))
			{
				continue;
			}

			CMapMeshObject* pMeshObj = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(handle);
			if (pMeshObj == nullptr)
			{
				continue;
			}

			chunkFileData.objects.push_back(m_ObjectFactory.MakeMapMeshObjectFileData(*pMeshObj, layerName));
		}
	}

	return m_ChunkSerializer.SaveChunkFile(chunkPath, chunkFileData);
}

HRESULT CMapManager::LoadMapData(const std::string& path)
{
	const std::filesystem::path mapDir(path);
	const std::filesystem::path mapFilePath = mapDir / "map.json";

	MAP_FILE_DATA mapFileData{};
	if (FAILED(m_ChunkSerializer.LoadMapFile(mapFilePath, mapFileData)))
		return E_FAIL;

	m_MapRootPath = mapDir.generic_string();
	m_ChunkStreamer.InvalidatePendingLoads();
	CGameInstance::Get().ClearMapMeshResidentChunks();
	m_ModelResourceTracker.QueueAllChunkReleases(m_Chunks);
	m_Chunks.clear();
	CGameInstance::Get().DelGameObjectLayer(E::MAPDECALOBJECTLAYER);

	for (const auto& decalData : mapFileData.decals)
	{
		if (!m_ObjectFactory.CreateDecal(decalData))
			return E_FAIL;
	}

	m_ChunkSize = mapFileData.chunkSize;

	for (const auto& chunkMetadata : mapFileData.chunks)
	{
		const MAPCHUNK_COORD coord = chunkMetadata.coord;
		CMapChunk chunk{ coord, MakeChunkBoundingBox(coord) };
		chunk.SetFilePath(chunkMetadata.filePath);
		chunk.SetSaveState(EChunkSaveState::Saved);

		m_Chunks.emplace(coord, std::move(chunk));
	}

	return S_OK;
}

HRESULT CMapManager::LoadChunk(const MAPCHUNK_COORD& coord)
{
	auto iter = m_Chunks.find(coord);
	if (iter == m_Chunks.end())
	{
		return E_FAIL;
	}

	CMapChunk& chunk = iter->second;
	if (chunk.IsLoaded())
	{
		return S_OK;
	}

	std::filesystem::path chunkPath = std::filesystem::path(m_MapRootPath) / chunk.GetFilePath();
	if (chunk.GetFilePath().empty())
	{
		chunk.SetFilePath((std::filesystem::path("chunks") / m_ChunkSerializer.MakeChunkFileName(coord)).generic_string());
		chunkPath = std::filesystem::path(m_MapRootPath) / chunk.GetFilePath();
	}

	MAP_CHUNK_FILE_DATA chunkFileData{};
	if (FAILED(m_ChunkSerializer.LoadChunkFile(chunkPath, chunkFileData)))
		return E_FAIL;

	chunk.BeginLoading();

	if (FAILED(m_ModelResourceTracker.AcquireChunkResources(chunk, chunkFileData.objects)))
	{
		chunk.CancelLoading();
		return E_FAIL;
	}

	for (const auto& objectData : chunkFileData.objects)
	{
		if (auto hObject = m_ObjectFactory.CreateMapMeshObject(objectData))
		{
			chunk.AddObject(hObject.value());
		}
	}

	chunk.CompleteLoading(MakeChunkBoundingBox(coord), EChunkSaveState::Saved);
	if (FAILED(CGameInstance::Get().RegisterMapMeshResidentChunk(coord, chunk.GetObjectHandles())))
	{
		UnLoadChunk(coord);
		return E_FAIL;
	}

	if (!chunk.GetObjectHandles().empty())
	{
		const BoundingBox& changedBounds = chunk.GetCullingBounds();
		CGameInstance::Get().Notify_StaticShadowSceneChanged(changedBounds);
	}

	return S_OK;
}

HRESULT CMapManager::UnLoadChunk(const MAPCHUNK_COORD& coord)
{
	auto iter = m_Chunks.find(coord);
	if (iter == m_Chunks.end())
	{
		return E_FAIL;
	}

	CMapChunk& chunk = iter->second;
	if (chunk.GetSaveState() == EChunkSaveState::Unsaved)
	{
		return E_FAIL;
	}

	const _bool hadObjects = !chunk.GetObjectHandles().empty();
	const BoundingBox removedBounds = chunk.GetCullingBounds();

	CGameInstance::Get().UnregisterMapMeshResidentChunk(coord);
	chunk.BeginUnloading();

	for (const auto& handle : chunk.GetObjectHandles())
	{
		if (auto* pObj = CGameInstance::Get().GetGameObjectByHandle(handle))
		{
			pObj->SetPendingDestroyCascade(true);
		}
	}

	m_ModelResourceTracker.QueueChunkRelease(chunk);
	chunk.CompleteUnloading();

	if (hadObjects)
		CGameInstance::Get().Notify_StaticShadowSceneChanged(removedBounds);

	return S_OK;
}
HRESULT CMapManager::SaveMaterial(const std::string& path)
{
	return m_MaterialRepository.SaveFile(std::filesystem::path(path) / "Material.json", CollectMapMaterials());
}

HRESULT CMapManager::LoadMaterial(const std::string& path)
{
	if (FAILED(m_MaterialRepository.LoadFile(std::filesystem::path(path) / "Material.json")))
		return E_FAIL;

	ApplyStoredMaterialsToLoadedModels();

	return S_OK;
}

MATERIAL_DESC CMapManager::FindMaterial(const std::string& modelName) const
{
	return m_MaterialRepository.Find(modelName);
}

CMapMaterialRepository::MATERIAL_MAP CMapManager::CollectMapMaterials() const
{
	CMapMaterialRepository::MATERIAL_MAP materials;
	const auto& layers = CGameInstance::Get().GetGameObjectLayers();
	for (const auto& [layerName, objects] : layers)
	{
		for (const auto& objectHandle : objects)
		{
			auto* mapObject = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(objectHandle);
			if (!mapObject)
				continue;

			const std::string modelName = mapObject->GetModelResourceTag();
			if (modelName.empty() || materials.contains(modelName))
				continue;

			materials.emplace(modelName, mapObject->GetStaticModelInstance()->GetModel()->GetMaterialDesc());
		}
	}
	return materials;
}

void CMapManager::ApplyStoredMaterialsToLoadedModels() const
{
	const auto materials = m_MaterialRepository.GetSnapshot();
	const auto& layers = CGameInstance::Get().GetGameObjectLayers();
	for (const auto& [layerName, objects] : layers)
	{
		for (const auto& objectHandle : objects)
		{
			auto* mapObject = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(objectHandle);
			if (!mapObject)
				continue;

			const auto material = materials.find(mapObject->GetModelResourceTag());
			if (material != materials.end())
				mapObject->GetStaticModelInstance()->GetModel()->SetMaterialDesc(material->second);
		}
	}
}
_float3 CMapManager::GetChunkCenter(const MAPCHUNK_COORD& coord)
{
	return _float3(
			{
				(coord.x + 0.5f) * m_ChunkSize.x,
				(coord.y + 0.5f) * m_ChunkSize.y,
				(coord.z + 0.5f) * m_ChunkSize.z,
			});
}

BoundingBox CMapManager::MakeChunkBoundingBox(const MAPCHUNK_COORD& coord)
{
	_float3 center = GetChunkCenter(coord);
	_float3 extents = {
		m_ChunkSize.x * 0.5f,
		m_ChunkSize.y * 0.5f,
		m_ChunkSize.z * 0.5f
	};
	return BoundingBox(center, extents);
}

MAPCHUNK_COORD CMapManager::WorldToChunkCoord(const _float3& pos) const
{
	return {
		static_cast<int64_t>(std::floor(pos.x / m_ChunkSize.x)),
		static_cast<int64_t>(std::floor(pos.y / m_ChunkSize.y)),
		static_cast<int64_t>(std::floor(pos.z / m_ChunkSize.z))
	};
}

void CMapManager::RebuildChunks()
{
	m_ChunkStreamer.InvalidatePendingLoads();
	CGameInstance::Get().ClearMapMeshResidentChunks();
	auto previousChunks = std::move(m_Chunks);
	m_Chunks.clear();
	m_Chunks.reserve(previousChunks.size());

	// 저장 상태와 모델 참조는 유지하되 오브젝트 목록과 옥트리는 현재 월드를 기준으로 다시 만듦
	for (auto& [coord, previousChunk] : previousChunks)
	{
		CMapChunk rebuiltChunk{ coord, previousChunk.GetBounds() };
		rebuiltChunk.SetFilePath(previousChunk.GetFilePath());
		rebuiltChunk.SetSaveState(previousChunk.GetSaveState());
		rebuiltChunk.SetModelResources(previousChunk.TakeModelResources());
		m_Chunks.emplace(coord, std::move(rebuiltChunk));
	}

	const auto& layers = CGameInstance::Get().GetGameObjectLayers();

	for (const auto& [layerName, layer] : layers)
	{
		for (const auto& handle : layer)
		{
			CMapMeshObject* pObj = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(handle);

			if (pObj == nullptr)
				continue;

			const auto& pos = pObj->GetTransform().GetPosition();

			const MAPCHUNK_COORD coord = WorldToChunkCoord(pos);

			auto chunkIter = m_Chunks.find(coord);
			if (chunkIter == m_Chunks.end())
			{
				CMapChunk newChunk{ coord, MakeChunkBoundingBox(coord) };
				newChunk.SetSaveState(EChunkSaveState::Unsaved);
				chunkIter = m_Chunks.emplace(coord, std::move(newChunk)).first;
			}

			chunkIter->second.AddObject(handle);
		}
	}

	for (auto& [coord, chunk] : m_Chunks)
	{
		const auto previousIter = previousChunks.find(coord);
		const _bool wasLoaded = previousIter != previousChunks.end() && previousIter->second.IsLoaded();

		if (!wasLoaded && chunk.GetObjectHandles().empty())
			continue;

		chunk.CompleteLoading(MakeChunkBoundingBox(coord), chunk.GetSaveState());
		CGameInstance::Get().RegisterMapMeshResidentChunk(coord, chunk.GetObjectHandles());
	}
}

HRESULT CMapManager::RegisterMapMeshObject(const CHandle& hObject)
{
	CMapMeshObject* pObj = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(hObject);
	if (pObj == nullptr)
	{
		return E_FAIL;
	}

	pObj->GetTransform().Update();

	const MAPCHUNK_COORD coord = WorldToChunkCoord(pObj->GetTransform().GetPosition());
	auto chunkIter = m_Chunks.find(coord);
	if (chunkIter == m_Chunks.end())
	{
		CMapChunk newChunk{ coord, MakeChunkBoundingBox(coord) };
		chunkIter = m_Chunks.emplace(coord, std::move(newChunk)).first;
	}

	auto& chunk = chunkIter->second;
	chunk.AddObject(hObject);
	chunk.CompleteLoading(MakeChunkBoundingBox(coord), EChunkSaveState::Unsaved);

	if (FAILED(CGameInstance::Get().RegisterMapMeshResidentChunk(coord, chunk.GetObjectHandles())))
	{
		return E_FAIL;
	}

	BoundingBox changedBounds{};
	if (pObj->GetShadowBounds(changedBounds))
	{
		CGameInstance::Get().Notify_StaticShadowSceneChanged(changedBounds);
	}
	else
	{
		CGameInstance::Get().Notify_StaticShadowSceneChanged(chunk.GetBounds());
	}

	return S_OK;
}

HRESULT CMapManager::RefreshMapMeshObject(const CHandle& hObject)
{
	auto* mapObject = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(hObject);
	if (mapObject == nullptr)
		return E_FAIL;

	mapObject->GetTransform().Update();
	const MAPCHUNK_COORD newCoord = WorldToChunkCoord(mapObject->GetTransform().GetPosition());

	auto oldChunkIter = std::find_if(m_Chunks.begin(), m_Chunks.end(), 
		[&](const auto& entry) { return entry.second.ContainsObject(hObject); });

	if (oldChunkIter != m_Chunks.end() && oldChunkIter->first != newCoord)
	{
		CMapChunk& oldChunk = oldChunkIter->second;
		oldChunk.RemoveObject(hObject);
		oldChunk.CompleteLoading(oldChunk.GetBounds(), EChunkSaveState::Unsaved);
		if (FAILED(CGameInstance::Get().RegisterMapMeshResidentChunk(oldChunkIter->first, oldChunk.GetObjectHandles())))
		{
			return E_FAIL;
		}
	}

	auto newChunkIter = m_Chunks.find(newCoord);
	if (newChunkIter == m_Chunks.end())
	{
		CMapChunk newChunk{ newCoord, MakeChunkBoundingBox(newCoord) };
		newChunkIter = m_Chunks.emplace(newCoord, std::move(newChunk)).first;
	}

	CMapChunk& newChunk = newChunkIter->second;
	newChunk.AddObject(hObject);
	newChunk.CompleteLoading(MakeChunkBoundingBox(newCoord), EChunkSaveState::Unsaved);
	if (FAILED(CGameInstance::Get().RegisterMapMeshResidentChunk(newCoord, newChunk.GetObjectHandles())))
	{
		return E_FAIL;
	}

	BoundingBox changedBounds{};
	if (mapObject->GetShadowBounds(changedBounds))
		CGameInstance::Get().Notify_StaticShadowSceneChanged(changedBounds);

	return S_OK;
}

HRESULT CMapManager::UnregisterMapMeshObject(const CHandle& hObject)
{
	BoundingBox removedBounds{};
	if (auto* mapObject = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(hObject))
		mapObject->GetShadowBounds(removedBounds);

	for (auto& [coord, chunk] : m_Chunks)
	{
		if (!chunk.RemoveObject(hObject))
			continue;

		chunk.CompleteLoading(chunk.GetBounds(), EChunkSaveState::Unsaved);
		const HRESULT result = CGameInstance::Get().RegisterMapMeshResidentChunk(coord, chunk.GetObjectHandles());
		CGameInstance::Get().Notify_StaticShadowSceneChanged(removedBounds);

		return result;
	}
	return E_FAIL;
}

std::vector<CHandle> CMapManager::CollectMapMeshPickCandidates(FXMVECTOR rayOrigin, FXMVECTOR rayDirection) const
{
	std::vector<CHandle> candidates;

	for (const auto& [coord, chunk] : m_Chunks)
	{
		if (!chunk.IsLoaded() || !chunk.GetOctree())
			continue;

		chunk.GetOctree()->CollectRayCandidates(rayOrigin, rayDirection, candidates);
	}

	return candidates;
}

#ifdef _DEBUG

HRESULT CMapManager::RenderDebugMapChunk()
{
	return m_ChunkDebugRenderer
		? m_ChunkDebugRenderer->Render(m_Chunks)
		: E_FAIL;
}

void CMapManager::SetDebugDrawMapChunk(_bool draw)
{
	if (m_ChunkDebugRenderer)
		m_ChunkDebugRenderer->SetEnabled(draw);
}
#endif

UPtr<CMapManager> CMapManager::Create()
{
	auto pInstance = ToUPtr(new CMapManager{});
	if (FAILED(pInstance->Initialize()))
	{
		return nullptr;
	}
	return pInstance;
}

void CMapManager::Free()
{
	CEngineBase::Free();
}


