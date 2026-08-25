#include "pch.h"
#include "MapManager.h"
#include "MapMeshObject.h"
#include "DecalVolume.h"
#include "ResTexture2D.h"
#include <filesystem>
#include <set>

#include "CameraObject.h"
#include "CollFrustum.h"
#include "MapStaticModelLoader.h"
#include "OctreeNode.h"
#include "ComStaticModelInstance.h"
NS_USING(Engine)

struct Engine::PENDING_CHUNK_APPLY_STATE
{
	PENDING_CHUNK_LOAD_RESULT result{};
	size_t nextObjectIndex{};
	_bool initialized{};
};

namespace
{
	//constexpr _float3 DEFAULT_MAP_CHUNK_SIZE{ 150.f, 150.f, 150.f };

	_bool IsSameChunkSize(const _float3& lhs, const _float3& rhs)
	{
		return std::fabs(lhs.x - rhs.x) <= FLT_EPSILON
			&& std::fabs(lhs.y - rhs.y) <= FLT_EPSILON
			&& std::fabs(lhs.z - rhs.z) <= FLT_EPSILON;
	}

}

#ifdef _DEBUG
std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> CMapManager::_batch = nullptr;
std::unique_ptr<DirectX::BasicEffect> CMapManager::_effect = nullptr;
ComPtr<ID3D11InputLayout> CMapManager::_inputLayout = nullptr;
#endif

CMapManager::CMapManager()
{
}

CMapManager::~CMapManager()
{
#ifdef _DEBUG
	//-------DebugDraw-------
	_batch.reset();
	_effect.reset();
	_inputLayout.Reset();
	//-----------------------
#endif
}

HRESULT CMapManager::Initialize()
{

#ifdef _DEBUG
	// DebugDraw
	// static 객체들이 아직 생성되지 않았다면 최초 1회 생성
	if (_batch == nullptr)
	{
		// Batch 생성
		_batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(CGameInstance::Get().GetGraphicDeviceContext().Get());

		// Effect 생성 및 설정
		_effect = std::make_unique<DirectX::BasicEffect>(CGameInstance::Get().GetGraphicDevice().Get());
		_effect->SetVertexColorEnabled(true); // 정점 색상 사용 활성화

		// InputLayout 생성 (BasicEffect의 셰이더 정보와 VertexPositionColor 구조를 연결)
		void const* shaderByteCode;
		size_t byteCodeLength;
		_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

		HRESULT hr = CGameInstance::Get().GetGraphicDevice()->CreateInputLayout(
			DirectX::VertexPositionColor::InputElements,
			DirectX::VertexPositionColor::InputElementCount,
			shaderByteCode,
			byteCodeLength,
			_inputLayout.GetAddressOf()
		);

		assert(SUCCEEDED(hr));
	}
#endif

	return S_OK;
}

void CMapManager::PriorityUpdate(_float fTimeDelta)
{

}

// 저장해놨던 Chunk들을 심리스 로딩/언로딩
// (저장해놓은 Chunk들만 작동하기때문에 에디터에서 실시간으로 Rebuild한 Cunk는 심리스의 적용대상이 되지않음)

void CMapManager::Update(_float fTimeDelta)
{
	ProcessDeferredModelReleases();
	ProcessLoadedChunkResults();

	if (m_Chunks.empty())
	{
		return;
	}
	CCameraObject* pCamera = CGameInstance::Get().GetActiveCamera();
	if (pCamera == nullptr)
	{
		return;
	}

	// CPU 프러스텀 컬링 비활성화
	//const auto* pFrustumCollider = pCamera->GetFrustumCollider();
	//if (pFrustumCollider == nullptr)
	//	return;

	const auto loadChunks = GetChunksAroundCamera(pCamera, STREAM_LOAD_DIAMETER);
	const auto retainedChunks = GetChunksAroundCamera(pCamera, STREAM_UNLOAD_DIAMETER);
	//const auto& boundingFrustum = pFrustumCollider->GetBoundingFrustum();

	//CullLoadedChunksByCameraFrustum(retainedChunks, boundingFrustum);

	if (m_bChunkStreaming && !m_sMapRootPath.empty())
	{
		UnloadChunksOutsideRange(retainedChunks);
		RequestNeededChunkLoads(loadChunks);
	}

    //  이전 프레임에서 예약한 리소스 해제
    //               ↓
    //  워커가 완료한 청크를 월드에 조금씩 반영
    //               ↓
    //  카메라 주변 6×6×6 청크 계산
    //  카메라 주변 7×7×7 유지 영역 계산
    //               ↓
    //  7×7×7 밖의 청크 언로드
    //               ↓
    //  6×6×6 안에서 가장 가까운 청크 로드 요청
}

std::vector<MAPCHUNK_COORD> CMapManager::GetChunksAroundCamera(const CCameraObject* pCamera, int64_t diameter) const
{
	const auto& pos = pCamera->GetTransform().GetPosition();
	std::vector<MAPCHUNK_COORD> chunks;
	if (diameter <= 0)
		return chunks;

	const size_t chunkCountPerAxis = static_cast<size_t>(diameter);
	chunks.reserve(chunkCountPerAxis * chunkCountPerAxis * chunkCountPerAxis);

	// For an even diameter, move the extra half of the range toward the side of
	// the chunk that contains the camera. This keeps the 6-wide load range inside
	// the centered 7-wide retention range while avoiding a permanent axis bias.
	const auto GetFirstCoord = [diameter](_float position, _float chunkSize)
		{
			return static_cast<int64_t>(std::floor(
				position / chunkSize - static_cast<_float>(diameter - 1) * 0.5f));
		};

	const MAPCHUNK_COORD firstCoord
	{
		GetFirstCoord(pos.x, m_vChunkSize.x),
		GetFirstCoord(pos.y, m_vChunkSize.y),
		GetFirstCoord(pos.z, m_vChunkSize.z)
	};

	for (int64_t y = 0; y < diameter; ++y)
	{
		for (int64_t z = 0; z < diameter; ++z)
		{
			for (int64_t x = 0; x < diameter; ++x)
			{
				chunks.push_back(
					MAPCHUNK_COORD
					{
						firstCoord.x + x,
						firstCoord.y + y,
						firstCoord.z + z
					});
			}
		}
	}

	return chunks;
}

void CMapManager::UnloadChunksOutsideRange(const std::vector<MAPCHUNK_COORD>& neededChunks)
{
	auto isNeededChunk = [&neededChunks](const MAPCHUNK_COORD& coord)
		{
			return std::find(neededChunks.begin(), neededChunks.end(), coord) != neededChunks.end();
		};

	// 모든 Chunk들 중 주변3*3*3 Chunk가 아니라면 UnLoad
	for (auto& [coord, chunk] : m_Chunks)
	{
		if (chunk.CanAutoUnload() && !isNeededChunk(coord))
		{
			UnLoadChunk(coord);
		}
	}
}

void CMapManager::RequestNeededChunkLoads(const std::vector<MAPCHUNK_COORD>& neededChunks)
{
	const uint32_t inFlight = m_AsyncChunkLoadsInFlight.load(std::memory_order_acquire);
	if (inFlight >= MAX_CONCURRENT_CHUNK_LOADS)
		return;
	uint32_t availableSlots = MAX_CONCURRENT_CHUNK_LOADS - inFlight;

	std::vector<MAPCHUNK_COORD> prioritizedChunks = neededChunks;
	if (const auto* camera = CGameInstance::Get().GetActiveCamera())
	{
		const MAPCHUNK_COORD cameraCoord = WorldToChunkCoord(camera->GetTransform().GetPosition());

		std::sort(prioritizedChunks.begin(), prioritizedChunks.end(),
			[&cameraCoord](const MAPCHUNK_COORD& lhs, const MAPCHUNK_COORD& rhs)
			{
				const auto DistanceSquared = [&cameraCoord](const MAPCHUNK_COORD& coord)
					{
						const int64_t dx = coord.x - cameraCoord.x;
						const int64_t dy = coord.y - cameraCoord.y;
						const int64_t dz = coord.z - cameraCoord.z;
						return dx * dx + dy * dy + dz * dz;
					};
				return DistanceSquared(lhs) < DistanceSquared(rhs);
			});
	}

	for (const auto& coord : prioritizedChunks)
	{
		if (availableSlots == 0)
			break;

		auto iter = m_Chunks.find(coord);
		if (iter == m_Chunks.end())
		{
			continue;
		}

		if (iter->second.CanAutoLoad())
		{
			// 각 요청은 워커풀내의 독립적인 작업
			// 가장 가까운 청크부터 제출됨
			if (SUCCEEDED(RequestLoadChunkAsync(coord)))
				--availableSlots;
		}
	}
}

void CMapManager::LateUpdate(_float fTimeDelta)
{

}

void CMapManager::ClearAllChunk()
{
	m_MapGeneration.fetch_add(1, std::memory_order_acq_rel);
	QueueAllChunkModelReleases();
	m_Chunks.clear();
}

void CMapManager::SetMapModelResourceIndex(const std::filesystem::path& staticModelRoot, const std::string& resourceGroup, std::unordered_map<std::string, std::filesystem::path> modelPaths)
{
	std::lock_guard<std::mutex> lock(m_MapModelResourceIndexMutex);
	m_MapModelStaticRoot = staticModelRoot;
	m_MapModelResourceGroup = resourceGroup;
	m_MapModelPaths = std::move(modelPaths);
}

HRESULT CMapManager::EnsureModelResourceLoaded(const MAP_MODEL_RESOURCE_KEY& key)
{
	if (auto model = CGameInstance::Get().GetResourceFirst<CResStaticModel>(key.group, key.tag))
	{
		if (model->GetState() == CResource::STATE::LOADED)
			return S_OK;
		if (model->GetState() != CResource::STATE::UNLOAD && model->GetState() != CResource::STATE::LOADFAIL)
			return E_FAIL;
	}

	std::filesystem::path staticModelRoot;
	std::filesystem::path modelPath;
	{
		std::lock_guard<std::mutex> lock(m_MapModelResourceIndexMutex);
		if (key.group != m_MapModelResourceGroup)
			return E_FAIL;

		const auto pathIter = m_MapModelPaths.find(key.tag);
		if (pathIter == m_MapModelPaths.end())
			return E_FAIL;

		staticModelRoot = m_MapModelStaticRoot;
		modelPath = pathIter->second;
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

SPtr<std::mutex> CMapManager::GetModelResourceMutex(const MAP_MODEL_RESOURCE_KEY& key)
{
	std::lock_guard<std::mutex> lock(m_ModelResourceMutexMapMutex);
	auto& modelMutex = m_ModelResourceMutexes[key];
	if (!modelMutex)
		modelMutex = std::make_shared<std::mutex>();
	return modelMutex;
}

HRESULT CMapManager::AcquireChunkModelResources(CMapChunk& chunk, const std::vector<MAP_MESH_OBJECT_FILE_DATA>& objects)
{
	std::unordered_set<MAP_MODEL_RESOURCE_KEY, MAP_MODEL_RESOURCE_KEY_HASH> uniqueModels;
	for (const auto& object : objects)
	{
		if (!object.modelGroup.empty() && !object.model.empty())
			uniqueModels.insert({ object.modelGroup, object.model });
	}

	auto& modelResources = chunk.GetModelResources();
	modelResources.clear();
	modelResources.reserve(uniqueModels.size());
	for (const auto& key : uniqueModels)
	{
		auto modelMutex = GetModelResourceMutex(key);
		std::lock_guard<std::mutex> resourceLock(*modelMutex);

		if (FAILED(EnsureModelResourceLoaded(key)))
		{
			for (const auto& acquiredKey : modelResources)
			{
				auto refIter = m_ModelChunkRefCounts.find(acquiredKey);
				if (refIter == m_ModelChunkRefCounts.end())
					continue;
				if (refIter->second > 1)
					--refIter->second;
				else
				{
					m_ModelChunkRefCounts.erase(refIter);
					m_DeferredUnusedModelReleases.push_back(acquiredKey);
				}
			}
			modelResources.clear();
			return E_FAIL;
		}

		++m_ModelChunkRefCounts[key];
		modelResources.push_back(key);
	}

	return S_OK;
}

HRESULT CMapManager::PreloadChunkModelResources(PENDING_CHUNK_LOAD_RESULT& result)
{
	ZoneScopedN("ChunkResourcePreloadWorker");
	std::unordered_set<MAP_MODEL_RESOURCE_KEY, MAP_MODEL_RESOURCE_KEY_HASH> uniqueModels;
	for (const auto& object : result.objects)
	{
		if (!object.modelGroup.empty() && !object.model.empty())
			uniqueModels.insert({ object.modelGroup, object.model });
	}

	result.modelResources.clear();
	result.modelResources.reserve(uniqueModels.size());
	for (const auto& key : uniqueModels)
	{
		auto modelMutex = GetModelResourceMutex(key);
		std::lock_guard<std::mutex> resourceLock(*modelMutex);

		if (FAILED(EnsureModelResourceLoaded(key)))
			return E_FAIL;

		result.modelResources.push_back(key);
		{
			std::lock_guard<std::mutex> pendingLock(m_PendingModelRefMutex);
			++m_PendingModelRefCounts[key];
		}
	}
	return S_OK;
}

HRESULT CMapManager::AcquirePreloadedChunkModelResources(CMapChunk& chunk, const PENDING_CHUNK_LOAD_RESULT& result)
{
	for (const auto& key : result.modelResources)
	{
		auto model = CGameInstance::Get().GetResourceFirst<CResStaticModel>(key.group, key.tag);
		if (!model || model->GetState() != CResource::STATE::LOADED)
		{
			ReleasePendingModelResources(result);
			return E_FAIL;
		}
	}

	{
		std::lock_guard<std::mutex> pendingLock(m_PendingModelRefMutex);
		for (const auto& key : result.modelResources)
		{
			auto pendingIter = m_PendingModelRefCounts.find(key);
			if (pendingIter == m_PendingModelRefCounts.end())
				continue;
			if (pendingIter->second > 1)
				--pendingIter->second;
			else
				m_PendingModelRefCounts.erase(pendingIter);
		}
	}

	chunk.SetModelResources(result.modelResources);
	for (const auto& key : chunk.GetModelResources())
		++m_ModelChunkRefCounts[key];
	return S_OK;
}

void CMapManager::ReleasePendingModelResources(const PENDING_CHUNK_LOAD_RESULT& result)
{
	{
		std::lock_guard<std::mutex> pendingLock(m_PendingModelRefMutex);
		for (const auto& key : result.modelResources)
		{
			auto pendingIter = m_PendingModelRefCounts.find(key);
			if (pendingIter == m_PendingModelRefCounts.end())
				continue;
			if (pendingIter->second > 1)
				--pendingIter->second;
			else
				m_PendingModelRefCounts.erase(pendingIter);
		}
	}

	m_DeferredUnusedModelReleases.insert(m_DeferredUnusedModelReleases.end(), result.modelResources.begin(), result.modelResources.end());
}

void CMapManager::QueueChunkModelRelease(CMapChunk& chunk)
{
	if (chunk.GetModelResources().empty())
		return;

	m_DeferredModelReleases.push_back(chunk.TakeModelResources());
}

void CMapManager::QueueAllChunkModelReleases()
{
	for (auto& [coord, chunk] : m_Chunks)
		QueueChunkModelRelease(chunk);
}

void CMapManager::ProcessDeferredModelReleases()
{
	if (m_DeferredModelReleases.empty() && m_DeferredUnusedModelReleases.empty())
		return;

	auto releases = std::move(m_DeferredModelReleases);
	m_DeferredModelReleases.clear();
	auto candidates = std::move(m_DeferredUnusedModelReleases);
	m_DeferredUnusedModelReleases.clear();

	for (const auto& chunkResources : releases)
	{
		for (const auto& key : chunkResources)
		{
			const auto refIter = m_ModelChunkRefCounts.find(key);
			if (refIter == m_ModelChunkRefCounts.end())
				continue;

			if (refIter->second > 1)
			{
				--refIter->second;
				continue;
			}

			m_ModelChunkRefCounts.erase(refIter);
			candidates.push_back(key);
		}
	}

	for (const auto& key : candidates)
	{
		if (m_ModelChunkRefCounts.contains(key))
			continue;

		auto modelMutex = GetModelResourceMutex(key);
		std::unique_lock<std::mutex> resourceLock(*modelMutex, std::try_to_lock);
		if (!resourceLock.owns_lock())
		{
			m_DeferredUnusedModelReleases.push_back(key);
			continue;
		}

		{
			std::lock_guard<std::mutex> pendingLock(m_PendingModelRefMutex);
			if (m_PendingModelRefCounts.contains(key))
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

MAP_MESH_OBJECT_FILE_DATA CMapManager::MakeMapMeshObjectFileData(
	const CMapMeshObject& object,
	const std::string& layerName) const
{
	const auto& transform = object.GetTransform();

	MAP_MESH_OBJECT_FILE_DATA fileData{};
	fileData.objectTag = object.GetObjectTag();
	fileData.protoGroup = "PERMANENT";
	fileData.prototype = "Prototype_GameObject_MapMeshObject";
	fileData.modelGroup = object.GetModelResourceGroup();
	fileData.model = object.GetModelResourceTag();
	fileData.layer = layerName;
	fileData.position = transform.GetPosition();
	fileData.rotation = transform.GetQuaternion();
	fileData.scale = transform.GetScale();
	fileData.windDesc = object.GetWindDesc();
	return fileData;
}

std::optional<CHandle> CMapManager::CreateMapMeshObject(
	const MAP_MESH_OBJECT_FILE_DATA& objectData) const
{
	CMapMeshObject::MAP_MESH_OBJECT_DESC desc{};
	desc.sObjectTag = objectData.objectTag;
	desc.protoGroupTag = objectData.protoGroup;
	desc.prototypeTag = objectData.prototype;
	desc.modelGroupTag = objectData.modelGroup;
	desc.modelResTag = objectData.model;
	desc.windDesc = objectData.windDesc;

	auto handle = CGameInstance::Get().AddGameObjectToLayer(
		desc.protoGroupTag,
		desc.prototypeTag,
		objectData.layer,
		&desc);
	if (!handle)
		return std::nullopt;

	auto* object = CGameInstance::Get().GetGameObjectByHandle(*handle);
	if (!object)
		return std::nullopt;

	auto& transform = object->GetTransform();
	transform.SetPosition(objectData.position);
	transform.SetQuaternion(objectData.rotation);
	transform.SetScale(objectData.scale);
	transform.Update();
	return handle;
}

MAP_DECAL_FILE_DATA CMapManager::MakeDecalFileData(
	const CDecalVolume& decal,
	const std::string& layerName) const
{
	const auto& transform = decal.GetTransform();
	const _bool hasMaskOverride = decal.GetMaskTextureGroup().hash != 0 &&
		decal.GetMaskTextureTag().hash != 0;

	MAP_DECAL_FILE_DATA fileData{};
	fileData.objectTag = decal.GetObjectTag();
	fileData.layer = layerName;
	fileData.materialPath = decal.GetMaterialPath();
	fileData.textureGroup = hasMaskOverride ? decal.GetMaskTextureGroup().GetDbgStr() : "";
	fileData.textureTag = hasMaskOverride ? decal.GetMaskTextureTag().GetDbgStr() : "";
	fileData.texturePath = decal.GetMaskTexturePath();
	fileData.position = transform.GetPosition();
	fileData.rotation = transform.GetQuaternion();
	fileData.scale = transform.GetScale();
	fileData.opacity = decal.GetOpacity();
	fileData.normalThreshold = decal.GetNormalThreshold();
	fileData.edgeSoftness = decal.GetEdgeSoftness();
	fileData.hasMaterialParameters = true;

	for (const auto& parameter : decal.GetMaterialParameters())
	{
		const _float* values = decal.GetMaterialParameterData(parameter.name);
		if (!values)
			continue;

		MAP_DECAL_PARAMETER_DATA parameterData{};
		parameterData.name = parameter.name;
		parameterData.values.assign(values, values + parameter.count);
		fileData.materialParameters.push_back(std::move(parameterData));
	}

	for (UINT slot = CDecalMaterial::TEXTURE_SLOT_BEGIN;
		slot <= CDecalMaterial::TEXTURE_SLOT_END;
		++slot)
	{
		const auto& group = decal.GetTextureOverrideGroup(slot);
		const auto& tag = decal.GetTextureOverrideTag(slot);
		if (group.hash == 0 || tag.hash == 0)
			continue;

		fileData.textureOverrides.push_back(
		{
			slot,
			group.GetDbgStr(),
			tag.GetDbgStr(),
			decal.GetTextureOverridePath(slot)
		});
	}

	return fileData;
}

std::optional<CHandle> CMapManager::CreateDecal(
	const MAP_DECAL_FILE_DATA& decalData) const
{
	auto& gameInstance = CGameInstance::Get();
	const std::string textureGroup = decalData.textureGroup.empty()
		? std::string(TAG_RES_GRP_MAP_DECAL_TEXTURE)
		: decalData.textureGroup;

	if (!decalData.textureTag.empty() &&
		!gameInstance.GetResourceFirst<CResTexture2D>(textureGroup, decalData.textureTag))
	{
		if (decalData.texturePath.empty())
			return std::nullopt;
		auto texture = gameInstance.AddResourceT<CResTexture2D>(
			textureGroup,
			decalData.textureTag,
			CResTexture2D::Create(decalData.texturePath));
		if (!texture || FAILED(texture->Load()))
			return std::nullopt;
	}

	CDecalVolume::DECAL_VOLUME_DESC desc{};
	desc.sObjectTag = decalData.objectTag;
	desc.sMaterialPath = decalData.materialPath.empty()
		? std::string(CDecalVolume::DEFAULT_MATERIAL_PATH)
		: decalData.materialPath;
	desc.fOpacity = decalData.opacity;
	desc.fNormalThreshold = decalData.normalThreshold;
	desc.fEdgeSoftness = decalData.edgeSoftness;
	if (!decalData.textureTag.empty())
	{
		desc.sTextureGroup = textureGroup;
		desc.sMaskTextureTag = decalData.textureTag;
	}

	auto handle = gameInstance.AddGameObjectToLayer(
		decalData.protoGroup,
		decalData.prototype,
		MAPDECALOBJECTLAYER,
		&desc);
	if (!handle)
		return std::nullopt;

	auto* decal = gameInstance.GetGameObjectByHandleT<CDecalVolume>(*handle);
	if (!decal)
		return std::nullopt;

	auto& transform = decal->GetTransform();
	transform.SetPosition(decalData.position);
	transform.SetQuaternion(decalData.rotation);
	transform.SetScale(decalData.scale);
	transform.Update();

	if (decalData.hasMaterialParameters)
	{
		for (const auto& parameter : decal->GetMaterialParameters())
		{
			const auto savedParameter = std::find_if(
				decalData.materialParameters.begin(),
				decalData.materialParameters.end(),
				[&parameter](const MAP_DECAL_PARAMETER_DATA& candidate)
				{
					return candidate.name == parameter.name;
				});
			if (savedParameter == decalData.materialParameters.end())
				continue;

			std::array<_float, 4> values{};
			const size_t copyCount = std::min<size_t>(
				parameter.count, savedParameter->values.size());
			std::copy_n(savedParameter->values.begin(), copyCount, values.begin());
			decal->SetMaterialParameter(parameter.name, values.data(), parameter.count);
		}
	}
	else
	{
		decal->SetMaterialParameter("Albedo", &decalData.legacyAlbedo.x, 4);
		decal->SetMaterialParameter("Emissive Color", &decalData.legacyEmissive.x, 3);
		decal->SetMaterialParameter(
			"Emissive Intensity", &decalData.legacyEmissiveIntensity, 1);
	}

	for (const auto& textureData : decalData.textureOverrides)
	{
		if (textureData.slot < CDecalMaterial::TEXTURE_SLOT_BEGIN ||
			textureData.slot > CDecalMaterial::TEXTURE_SLOT_END ||
			textureData.tag.empty())
			continue;

		const std::string group = textureData.group.empty()
			? std::string(TAG_RES_GRP_MAP_DECAL_TEXTURE)
			: textureData.group;
		if (!gameInstance.GetResourceFirst<CResTexture2D>(group, textureData.tag))
		{
			if (textureData.path.empty())
				continue;
			auto texture = gameInstance.AddResourceT<CResTexture2D>(
				group,
				textureData.tag,
				CResTexture2D::Create(textureData.path));
			if (!texture || FAILED(texture->Load()))
				continue;
		}
		decal->SetTextureOverride(textureData.slot, group, textureData.tag);
	}

	return handle;
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
	mapFileData.chunkSize = m_vChunkSize;

	const auto& layers = CGameInstance::Get().GetGameObjectLayers();

	std::vector<MAPCHUNK_COORD> originallyUnloadedChunks;
	for (auto& [coord, chunk] : m_Chunks)
	{
		if (chunk.GetLoadState() == EChunkLoadState::Unloaded &&
			chunk.GetSaveState() != EChunkSaveState::Unsaved)
		{
			originallyUnloadedChunks.push_back(coord);
			if (FAILED(/*RequestLoadChunkAsync(coord)*/LoadChunk(coord))) // Synchronous load while saving.
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
				mapFileData.decals.push_back(MakeDecalFileData(*decal, pair.first));
				continue;
			}

			CMapMeshObject* pMeshObj = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(objectHandle);

			if (pMeshObj == nullptr)
				continue;

			requiredModels.emplace(pMeshObj->GetModelResourceGroup(), pMeshObj->GetModelResourceTag());
		}
	}

	for (const auto& [modelGroup, model] : requiredModels)
		mapFileData.requiredModels.push_back({ modelGroup, model });

	for (const auto& coord : originallyUnloadedChunks)
	{
		UnLoadChunk(coord);
	}

	if (FAILED(m_ChunkSerializer.SaveMapFile(mapDir / "map.json", mapFileData)))
		return E_FAIL;

	/*----------- 광윤 추가 -----------*/
	SaveMaterial(mapDir.string());
	/*---------------------------------*/
	return S_OK;
}

HRESULT CMapManager::LoadMap(const std::string& path, _bool clearBeforeLoad)
{
	if (clearBeforeLoad)
	{
		m_MapGeneration.fetch_add(1, std::memory_order_acq_rel);
		CGameInstance::Get().DelGameObjectLayer(E::MAPMESHOBJECTLAYER);
		QueueAllChunkModelReleases();
		CGameInstance::Get().DelGameObjectLayer(E::MAPDECALOBJECTLAYER);
		m_Chunks.clear();
	}

	const std::filesystem::path mapDir(path);

	/*----------- 광윤 추가 -----------*/
	LoadMaterial(mapDir.string());
	/*---------------------------------*/

	std::filesystem::path mapFilePath = mapDir / "map.json";
	if (std::filesystem::exists(mapFilePath))
	{
		const _float3 requestedChunkSize = DEFAULT_MAP_CHUNK_SIZE;

		if (FAILED(LoadMapData(path)))
		{
			return E_FAIL;
		}

		if (!IsSameChunkSize(m_vChunkSize, requestedChunkSize))
		{
			std::vector<MAPCHUNK_COORD> oldChunkCoords;
			oldChunkCoords.reserve(m_Chunks.size());
			for (const auto& [coord, chunk] : m_Chunks)
				oldChunkCoords.push_back(coord);

			for (const MAPCHUNK_COORD& coord : oldChunkCoords)
			{
				if (FAILED(LoadChunk(coord)))
					return E_FAIL;
			}

			m_vChunkSize = requestedChunkSize;
			m_MapGeneration.fetch_add(1, std::memory_order_acq_rel);
			QueueAllChunkModelReleases();
			m_Chunks.clear();
			RebuildChunks();

			if (FAILED(SaveMap(path)))
				return E_FAIL;

			// 마이그레이션에는 해당 객체가 일시적으로만 필요
			// 일반적인 스트리밍 맵 로드 시 사용되는 것과 동일한, 메타데이터 전용 상태로 복원
			CGameInstance::Get().DelGameObjectLayer(E::MAPMESHOBJECTLAYER);
			m_MapGeneration.fetch_add(1, std::memory_order_acq_rel);
			QueueAllChunkModelReleases();
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

		// 메타만 Load하고 chunk 로딩은 Update함수에서 스트리밍에게 맡긴다
		//for (const auto& coord : chunkCoords)
		//{
		//	if (FAILED(RequestLoadChunkAsync(coord)/*LoadChunk(coord)*/))
		//	{
		//		return E_FAIL;
		//	}
		//}

		return S_OK;
	}

	mapFilePath = mapDir / "TestMap.json";
	std::vector<MAP_MESH_OBJECT_FILE_DATA> legacyObjects;
	if (FAILED(m_ChunkSerializer.LoadLegacyMapFile(mapFilePath, legacyObjects)))
		return E_FAIL;

	std::unordered_map<MAPCHUNK_COORD, std::vector<MAP_MESH_OBJECT_FILE_DATA>, tagMapChunkCoordHash> legacyObjectsByChunk;
	for (auto& object : legacyObjects)
		legacyObjectsByChunk[WorldToChunkCoord(object.position)].push_back(std::move(object));

	for (auto& [coord, objects] : legacyObjectsByChunk)
	{
		auto& chunk = m_Chunks[coord];
		chunk.SetCoord(coord);
		chunk.SetBounds(MakeChunkBoundingBox(coord));
		chunk.BeginLoading();
		if (FAILED(AcquireChunkModelResources(chunk, objects)))
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

			auto hObject = CGameInstance::Get().AddGameObjectToLayer(
				desc.protoGroupTag, desc.prototypeTag, objectDesc.layer, &desc);
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

			chunkFileData.objects.push_back(MakeMapMeshObjectFileData(*pMeshObj, layerName));
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

	m_sMapRootPath = mapDir.generic_string();
	m_MapGeneration.fetch_add(1, std::memory_order_acq_rel);
	QueueAllChunkModelReleases();
	m_Chunks.clear();
	CGameInstance::Get().DelGameObjectLayer(E::MAPDECALOBJECTLAYER);
	for (const auto& decalData : mapFileData.decals)
	{
		if (!CreateDecal(decalData))
			return E_FAIL;
	}

	m_vChunkSize = mapFileData.chunkSize;

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

	std::filesystem::path chunkPath = std::filesystem::path(m_sMapRootPath) / chunk.GetFilePath();
	if (chunk.GetFilePath().empty())
	{
		chunk.SetFilePath((std::filesystem::path("chunks") /
			m_ChunkSerializer.MakeChunkFileName(coord)).generic_string());
		chunkPath = std::filesystem::path(m_sMapRootPath) / chunk.GetFilePath();
	}

	MAP_CHUNK_FILE_DATA chunkFileData{};
	if (FAILED(m_ChunkSerializer.LoadChunkFile(chunkPath, chunkFileData)))
		return E_FAIL;

	chunk.BeginLoading();

	if (FAILED(AcquireChunkModelResources(chunk, chunkFileData.objects)))
	{
		chunk.CancelLoading();
		return E_FAIL;
	}

	for (const auto& objectData : chunkFileData.objects)
	{
		if (auto hObject = CreateMapMeshObject(objectData))
		{
			chunk.AddObject(hObject.value());
		}
	}

	chunk.CompleteLoading(MakeChunkBoundingBox(coord), EChunkSaveState::Saved);

	/*----------- 광윤 추가 -----------*/
	if (!chunk.GetObjectHandles().empty())
	{
		const BoundingBox& ChangedBounds = chunk.GetCullingBounds();
		CGameInstance::Get().Notify_StaticShadowSceneChanged(ChangedBounds);
	}
	/*---------------------------------*/

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


	/*----------- 광윤 추가 -----------*/
	const _bool bHadObjects = !chunk.GetObjectHandles().empty();
	const BoundingBox RemovedBounds = chunk.GetCullingBounds();
	/*---------------------------------*/


	chunk.BeginUnloading();

	for (const auto& handle : chunk.GetObjectHandles())
	{
		if (auto* pObj = CGameInstance::Get().GetGameObjectByHandle(handle))
		{
			pObj->SetPendingDestroyCascade(true);
		}
	}

	QueueChunkModelRelease(chunk);
	chunk.CompleteUnloading();

	/*----------- 광윤 추가 -----------*/
	if (bHadObjects)	CGameInstance::Get().Notify_StaticShadowSceneChanged(RemovedBounds);
	/*---------------------------------*/


	return S_OK;
}
HRESULT CMapManager::SaveMaterial(const std::string& path)
{
	return m_MaterialRepository.SaveFile(
		std::filesystem::path(path) / "Material.json",
		CollectMapMaterials());
}

HRESULT CMapManager::LoadMaterial(const std::string& path)
{
	if (FAILED(m_MaterialRepository.LoadFile(
		std::filesystem::path(path) / "Material.json")))
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
			auto* mapObject =
				CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(objectHandle);
			if (!mapObject)
				continue;

			const std::string modelName = mapObject->GetModelResourceTag();
			if (modelName.empty() || materials.contains(modelName))
				continue;

			materials.emplace(
				modelName,
				mapObject->GetStaticModelInstance()->GetModel()->GetMaterialDesc());
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
			auto* mapObject =
				CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(objectHandle);
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
				(coord.x + 0.5f) * m_vChunkSize.x,
				(coord.y + 0.5f) * m_vChunkSize.y,
				(coord.z + 0.5f) * m_vChunkSize.z,
			});
}

BoundingBox CMapManager::MakeChunkBoundingBox(const MAPCHUNK_COORD& coord)
{
	_float3 center = GetChunkCenter(coord);
	_float3 extents = {
		m_vChunkSize.x * 0.5f,
		m_vChunkSize.y * 0.5f,
		m_vChunkSize.z * 0.5f
	};
	return BoundingBox(center, extents);
}

MAPCHUNK_COORD CMapManager::WorldToChunkCoord(const _float3& pos) const
{
	return {
		static_cast<int64_t>(std::floor(pos.x / m_vChunkSize.x)),
		static_cast<int64_t>(std::floor(pos.y / m_vChunkSize.y)),
		static_cast<int64_t>(std::floor(pos.z / m_vChunkSize.z))
	};
}

void CMapManager::RebuildChunks()
{
	m_MapGeneration.fetch_add(1, std::memory_order_acq_rel);
	auto previousChunks = std::move(m_Chunks);
	m_Chunks.clear();
	m_Chunks.reserve(previousChunks.size());

	// 저장 상태와 모델 참조는 유지하되 오브젝트 목록과 옥트리는 현재 월드를 기준으로 다시 만든다.
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
		const _bool wasLoaded = previousIter != previousChunks.end()
			&& previousIter->second.IsLoaded();
		if (!wasLoaded && chunk.GetObjectHandles().empty())
			continue;

		chunk.CompleteLoading(MakeChunkBoundingBox(coord), chunk.GetSaveState());
	}
}

HRESULT CMapManager::RegisterMapMeshObject(const CHandle& hObject)
{
	CMapMeshObject* pObj = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(hObject);
	if (pObj == nullptr)
	{
		return E_FAIL;
	}

	/*----------- 광윤 추가 -----------*/
	pObj->GetTransform().Update();
	/*---------------------------------*/


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



	// CPU 프러스텀 컬링 비활성화
	//pObj->SetRenderEnable(true);

	/*----------- 광윤 추가 -----------*/
	BoundingBox ChangedBounds{};

	if (pObj->GetShadowBounds(ChangedBounds)) {
		CGameInstance::Get().Notify_StaticShadowSceneChanged(ChangedBounds);
	}
	else {
		CGameInstance::Get().Notify_StaticShadowSceneChanged(chunk.GetBounds());
	}
	/*---------------------------------*/
	return S_OK;
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
	if (m_bDebugDrawMapChunk == false)
		return E_FAIL;

	CCameraObject* cam = CGameInstance::Get().GetActiveCamera();

	if (cam == nullptr)
		return E_FAIL;

	const auto view = cam->GetView();
	const auto proj = cam->GetProj();

	//const XMVECTOR octreeDepthColors[] =
	//{
	//	Colors::Cyan,
	//	Colors::DeepSkyBlue,
	//	Colors::Yellow,
	//	Colors::Orange,
	//	Colors::Black,
	//	Colors::White,
	//};
	//const size_t octreeDepthColorCount = sizeof(octreeDepthColors) / sizeof(octreeDepthColors[0]);

	for (const auto& [coord, mapChunk] : m_Chunks)
	{
		if (mapChunk.IsLoaded() && mapChunk.GetOctree())
		{
			std::vector<OCTREE_DEBUG_BOUNDS> octreeBounds;
			mapChunk.GetOctree()->CollectDebugBounds(octreeBounds);

			for (const auto& nodeBounds : octreeBounds)
			{
				//const FXMVECTOR nodeColor = octreeDepthColors[
				//	nodeBounds.depth % octreeDepthColorCount];
				DrawBox(nodeBounds.bounds, Colors::Red, view, proj, XMMatrixIdentity());
			}
		}
		FXMVECTOR color = mapChunk.IsLoaded() ? Colors::Lime : Colors::Red;
		DrawBox(mapChunk.GetBounds(), color, view, proj, XMMatrixIdentity());
	}

	//// 카메라 주변 3x3x3 스트리밍 대상 청크 표시
	//const auto& camPos = cam->GetTransform().GetPosition();
	//const MAPCHUNK_COORD camCoord = WorldToChunkCoord(camPos);

	//for (int64_t y = -1; y <= 1; ++y)
	//{
	//	for (int64_t z = -1; z <= 1; ++z)
	//	{
	//		for (int64_t x = -1; x <= 1; ++x)
	//		{
	//			MAPCHUNK_COORD coord
	//			{
	//				camCoord.x + x,
	//				camCoord.y + y,
	//				camCoord.z + z
	//			};

	//			BoundingBox bounds = MakeChunkBoundingBox(coord);

	//			// 빨간색 : 현재 카메라 주변 스트리밍 범위
	//			DrawBox(bounds, Colors::Red, view, proj, XMMatrixIdentity());
	//		}
	//	}
	//}

	return S_OK;
}

void CMapManager::DrawBox(const DirectX::BoundingBox& box, DirectX::FXMVECTOR color, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world)
{
	_effect->SetView(view);
	_effect->SetProjection(projection);
	_effect->SetWorld(world);
	CGameInstance::Get().GetGraphicDeviceContext()->IASetInputLayout(_inputLayout.Get());

	// 그리기
	_effect->Apply(CGameInstance::Get().GetGraphicDeviceContext().Get());
	_batch->Begin();

	// DirectXTK의 내장 Draw 함수 호출
	DX::Draw(_batch.get(), box, color);

	_batch->End();
}
#endif

HRESULT CMapManager::RequestLoadChunkAsync(const MAPCHUNK_COORD& coord)
{
	auto iter = m_Chunks.find(coord);
	if (iter == m_Chunks.end())
		return E_FAIL;

	CMapChunk& chunk = iter->second;

	if (chunk.GetLoadState() == EChunkLoadState::Loaded ||
		chunk.GetLoadState() == EChunkLoadState::Loading)
	{
		return S_OK;
	}

	if (chunk.GetFilePath().empty())
	{
		chunk.SetFilePath((std::filesystem::path("chunks") /
			m_ChunkSerializer.MakeChunkFileName(coord)).generic_string());
	}

	const std::filesystem::path chunkPath = std::filesystem::path(m_sMapRootPath) / chunk.GetFilePath();
	const uint64_t mapGeneration = m_MapGeneration.load(std::memory_order_acquire);

	// 큐에 넣기 전에 메인스레드에서 Loading으로 바꿔야 중복 요청이 안 들어감
	chunk.BeginLoading();
	m_AsyncChunkLoadsInFlight.fetch_add(1, std::memory_order_acq_rel);

	const _bool queued = CGameInstance::Get().WorkerEnqueue("LoadChunk", [this, coord, chunkPath, mapGeneration]()
		{
			PENDING_CHUNK_LOAD_RESULT result{};
			result.coord = coord;
			result.mapGeneration = mapGeneration;

			try
			{
				MAP_CHUNK_FILE_DATA chunkFileData{};
				if (FAILED(m_ChunkSerializer.LoadChunkFile(chunkPath, chunkFileData)))
				{
					result.hr = E_FAIL;
				}
				else
				{
					result.objects = std::move(chunkFileData.objects);

					if (result.mapGeneration != m_MapGeneration.load(std::memory_order_acquire))
						result.hr = E_ABORT;
					else
						result.hr = PreloadChunkModelResources(result);
				}
			}
			catch (const std::exception&)
			{
				result.hr = E_FAIL;
			}

			{
				std::lock_guard<std::mutex> lock(m_LoadResultMutex);
				m_LoadResults.push_back(std::move(result));
			}
			m_AsyncChunkLoadsInFlight.fetch_sub(1, std::memory_order_acq_rel);
		});

	if (!queued)
	{
		m_AsyncChunkLoadsInFlight.fetch_sub(1, std::memory_order_acq_rel);
		chunk.CancelLoading();
		return E_FAIL;
	}

	return S_OK;
}
void CMapManager::ProcessLoadedChunkResults()
{
	ZoneScopedN("ChunkApplyBudget");
	std::vector<PENDING_CHUNK_LOAD_RESULT> results;

	{
		std::lock_guard<std::mutex> lock(m_LoadResultMutex);
		results.swap(m_LoadResults);
	}

	for (auto& result : results)
	{
		auto state = std::make_unique<PENDING_CHUNK_APPLY_STATE>();
		state->result = std::move(result);
		m_ChunkApplyQueue.push_back(std::move(state));
	}

	constexpr auto APPLY_BUDGET = std::chrono::microseconds(2000);
	const auto deadline = std::chrono::steady_clock::now() + APPLY_BUDGET;
	while (!m_ChunkApplyQueue.empty())
	{
		_bool completed = false;
		ContinueApplyLoadedChunkResult(*m_ChunkApplyQueue.front(), deadline, completed);
		if (completed)
			m_ChunkApplyQueue.pop_front();

		if (!completed || std::chrono::steady_clock::now() >= deadline)
			break;
	}
}

HRESULT CMapManager::ContinueApplyLoadedChunkResult(PENDING_CHUNK_APPLY_STATE& state, const std::chrono::steady_clock::time_point& deadline, _bool& completed)
{
	completed = false;
	if (state.result.mapGeneration != m_MapGeneration.load(std::memory_order_acquire))
	{
		if (!state.initialized)
			ReleasePendingModelResources(state.result);
		completed = true;
		return S_OK;
	}

	auto iter = m_Chunks.find(state.result.coord);
	if (iter == m_Chunks.end())
	{
		if (!state.initialized)
			ReleasePendingModelResources(state.result);
		completed = true;
		return E_FAIL;
	}

	CMapChunk& chunk = iter->second;
	if (FAILED(state.result.hr))
	{
		ReleasePendingModelResources(state.result);
		chunk.CancelLoading();
		completed = true;
		return E_FAIL;
	}

	if (m_bChunkStreaming && !IsChunkInStreamingRange(state.result.coord))
	{
		if (state.initialized)
			UnLoadChunk(state.result.coord);
		else
		{
			ReleasePendingModelResources(state.result);
			chunk.CancelLoading();
		}
		completed = true;
		return S_OK;
	}

	if (!state.initialized)
	{
		chunk.BeginLoading();
		if (FAILED(AcquirePreloadedChunkModelResources(chunk, state.result)))
		{
			chunk.CancelLoading();
			completed = true;
			return E_FAIL;
		}
		state.initialized = true;
	}

	do
	{
		if (state.nextObjectIndex >= state.result.objects.size())
			break;

		const auto& objectDesc = state.result.objects[state.nextObjectIndex++];
		CMapMeshObject::MAP_MESH_OBJECT_DESC desc{};
		desc.sObjectTag = objectDesc.objectTag;
		desc.protoGroupTag = objectDesc.protoGroup;
		desc.prototypeTag = objectDesc.prototype;
		desc.modelGroupTag = objectDesc.modelGroup;
		desc.modelResTag = objectDesc.model;
		desc.windDesc = objectDesc.windDesc;

		auto hObject = CGameInstance::Get().AddGameObjectToLayer(
			desc.protoGroupTag,
			desc.prototypeTag,
			objectDesc.layer,
			&desc);

		if (hObject.has_value())
		{
			if (auto* pObject = CGameInstance::Get().GetGameObjectByHandle(hObject.value()))
			{
				auto& transform = pObject->GetTransform();
				transform.SetPosition(objectDesc.position);
				transform.SetQuaternion(objectDesc.rotation);
				transform.SetScale(objectDesc.scale);
				chunk.AddObject(hObject.value());
			}
		}
	}
	while (std::chrono::steady_clock::now() < deadline);

	if (state.nextObjectIndex < state.result.objects.size())
		return S_OK;

	chunk.CompleteLoading(
		MakeChunkBoundingBox(state.result.coord), EChunkSaveState::Saved);

	completed = true;
	return S_OK;
}

HRESULT CMapManager::ApplyLoadedChunkResult(const PENDING_CHUNK_LOAD_RESULT& result)
{
	auto iter = m_Chunks.find(result.coord);
	if (iter == m_Chunks.end())
		return E_FAIL;

	CMapChunk& chunk = iter->second;

	if (FAILED(result.hr))
	{
		chunk.CancelLoading();
		return E_FAIL;
	}

	if (m_bChunkStreaming)
	{
		// 스트리밍 모드일때, 워커스레드가 뒤늦게 로드해준 Chunk 결과가 지금 카메라 위치를 보고 유효한 결과인지 판단
		if (IsChunkInStreamingRange(result.coord) == false)
		{
			chunk.CancelLoading();
			return S_OK;
		}
	}

	chunk.BeginLoading();
	if (FAILED(AcquireChunkModelResources(chunk, result.objects)))
	{
		chunk.CancelLoading();
		return E_FAIL;
	}

	for (const auto& objectDesc : result.objects)
	{
		CMapMeshObject::MAP_MESH_OBJECT_DESC desc{};
		desc.sObjectTag = objectDesc.objectTag;
		desc.protoGroupTag = objectDesc.protoGroup;
		desc.prototypeTag = objectDesc.prototype;
		desc.modelGroupTag = objectDesc.modelGroup;
		desc.modelResTag = objectDesc.model;
		desc.windDesc = objectDesc.windDesc;

		auto hObject = CGameInstance::Get().AddGameObjectToLayer(
			desc.protoGroupTag,
			desc.prototypeTag,
			objectDesc.layer,
			&desc);

		if (!hObject.has_value())
			continue;

		auto* pObject = CGameInstance::Get().GetGameObjectByHandle(hObject.value());
		if (pObject == nullptr)
			continue;

		auto& transform = pObject->GetTransform();
		transform.SetPosition(objectDesc.position);
		transform.SetQuaternion(objectDesc.rotation);
		transform.SetScale(objectDesc.scale);

		chunk.AddObject(hObject.value());
	}

	chunk.CompleteLoading(MakeChunkBoundingBox(result.coord), EChunkSaveState::Saved);


	/*----------- 광윤 추가 -----------*/
	if (!chunk.GetObjectHandles().empty()) {
		const BoundingBox& ChangedBounds = chunk.GetCullingBounds();
		CGameInstance::Get().Notify_StaticShadowSceneChanged(ChangedBounds);
	}
	/*---------------------------------*/

	return S_OK;
}

_bool CMapManager::IsChunkInStreamingRange(const MAPCHUNK_COORD& coord)
{
	const auto pCam = CGameInstance::Get().GetActiveCamera();
	if (pCam == nullptr)
		return false;

	// Discard an async result if it arrived after leaving the load range.
	const auto loadChunks = GetChunksAroundCamera(pCam, STREAM_LOAD_DIAMETER);
	return std::find(loadChunks.begin(), loadChunks.end(), coord) != loadChunks.end();
}

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


