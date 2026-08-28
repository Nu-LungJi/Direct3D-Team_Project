#include "pch.h"
#include "MapChunkStreamer.h"

#include "CameraObject.h"
#include "MapModelResourceTracker.h"
#include "MapRuntimeObjectFactory.h"

NS_USING(Engine)

HRESULT CMapChunkStreamer::Initialize(
	CHUNK_MAP& chunks,
	CMapChunkSerializer& serializer,
	CMapModelResourceTracker& resourceTracker,
	CMapRuntimeObjectFactory& objectFactory,
	UNLOAD_CHUNK_CALLBACK unloadChunkCallback)
{
	if (!unloadChunkCallback)
		return E_INVALIDARG;

	m_Chunks = &chunks;
	m_Serializer = &serializer;
	m_ResourceTracker = &resourceTracker;
	m_ObjectFactory = &objectFactory;
	m_UnloadChunkCallback = std::move(unloadChunkCallback);

	return S_OK;
}

void CMapChunkStreamer::Update(const std::string& mapRootPath, const _float3& chunkSize)
{
	if (!m_Chunks || !m_Serializer || !m_ResourceTracker || !m_ObjectFactory)
		return;

	ProcessLoadedChunkResults(chunkSize);
	if (m_Chunks->empty())
		return;

	const auto* camera = CGameInstance::Get().GetActiveCamera();
	if (!camera)
		return;

	const auto loadChunks = GetChunksAroundCamera(camera, STREAM_LOAD_DIAMETER, chunkSize);
	const auto retainedChunks = GetChunksAroundCamera(camera, STREAM_UNLOAD_DIAMETER, chunkSize);

	if (m_IsEnabled && !mapRootPath.empty())
	{
		UnloadChunksOutsideRange(retainedChunks);
		RequestNeededChunkLoads(loadChunks, mapRootPath, chunkSize);
	}
}

void CMapChunkStreamer::InvalidatePendingLoads()
{
	m_MapGeneration.fetch_add(1, std::memory_order_acq_rel);
}

std::vector<MAPCHUNK_COORD> CMapChunkStreamer::GetChunksAroundCamera(const CCameraObject* camera, int64_t diameter, const _float3& chunkSize) const
{
	std::vector<MAPCHUNK_COORD> chunks;
	if (!camera || diameter <= 0)
		return chunks;

	const size_t chunkCountPerAxis = static_cast<size_t>(diameter);
	chunks.reserve(chunkCountPerAxis * chunkCountPerAxis * chunkCountPerAxis);

	const auto& cameraPosition = camera->GetTransform().GetPosition();

	// 짝수 크기 범위가 카메라 청크를 기준으로 한쪽에 치우치지 않도록 시작 좌표를 정함
	const auto getFirstCoord = [diameter](_float position, _float axisChunkSize)
		{
			return static_cast<int64_t>(std::floor(position / axisChunkSize - static_cast<_float>(diameter - 1) * 0.5f));
		};

	const MAPCHUNK_COORD firstCoord
	{
		getFirstCoord(cameraPosition.x, chunkSize.x),
		getFirstCoord(cameraPosition.y, chunkSize.y),
		getFirstCoord(cameraPosition.z, chunkSize.z)
	};

	for (int64_t y = 0; y < diameter; ++y)
	{
		for (int64_t z = 0; z < diameter; ++z)
		{
			for (int64_t x = 0; x < diameter; ++x)
			{
				chunks.push_back(
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

MAPCHUNK_COORD CMapChunkStreamer::WorldToChunkCoord(const _float3& position, const _float3& chunkSize) const
{
	return
	{
		static_cast<int64_t>(std::floor(position.x / chunkSize.x)),
		static_cast<int64_t>(std::floor(position.y / chunkSize.y)),
		static_cast<int64_t>(std::floor(position.z / chunkSize.z))
	};
}

BoundingBox CMapChunkStreamer::MakeChunkBoundingBox(const MAPCHUNK_COORD& coord, const _float3& chunkSize) const
{
	const _float3 center
	{
		(static_cast<_float>(coord.x) + 0.5f) * chunkSize.x,
		(static_cast<_float>(coord.y) + 0.5f) * chunkSize.y,
		(static_cast<_float>(coord.z) + 0.5f) * chunkSize.z
	};
	const _float3 extents
	{
		chunkSize.x * 0.5f,
		chunkSize.y * 0.5f,
		chunkSize.z * 0.5f
	};

	return BoundingBox(center, extents);
}

void CMapChunkStreamer::UnloadChunksOutsideRange(const std::vector<MAPCHUNK_COORD>& retainedChunks)
{
	const auto isRetained = [&retainedChunks](const MAPCHUNK_COORD& coord)
		{
			return std::find(retainedChunks.begin(), retainedChunks.end(), coord) != retainedChunks.end();
		};

	for (auto& chunkEntry : *m_Chunks)
	{
		CMapChunk& chunk = chunkEntry.second;
		if (chunk.CanAutoUnload() && !isRetained(chunkEntry.first))
			m_UnloadChunkCallback(chunkEntry.first);
	}
}

void CMapChunkStreamer::RequestNeededChunkLoads(const std::vector<MAPCHUNK_COORD>& loadChunks, const std::string& mapRootPath, const _float3& chunkSize)
{
	const uint32_t inFlight = m_AsyncLoadsInFlight.load(std::memory_order_acquire);
	if (inFlight >= MAX_CONCURRENT_CHUNK_LOADS)
		return;

	uint32_t availableSlots = MAX_CONCURRENT_CHUNK_LOADS - inFlight;
	std::vector<MAPCHUNK_COORD> prioritizedChunks = loadChunks;
	if (const auto* camera = CGameInstance::Get().GetActiveCamera())
	{
		const MAPCHUNK_COORD cameraCoord = WorldToChunkCoord(camera->GetTransform().GetPosition(), chunkSize);
		std::sort(prioritizedChunks.begin(), prioritizedChunks.end(),
			[&cameraCoord](const MAPCHUNK_COORD& lhs, const MAPCHUNK_COORD& rhs)
			{
				const auto distanceSquared = [&cameraCoord](const MAPCHUNK_COORD& coord)
					{
						const int64_t dx = coord.x - cameraCoord.x;
						const int64_t dy = coord.y - cameraCoord.y;
						const int64_t dz = coord.z - cameraCoord.z;
						return dx * dx + dy * dy + dz * dz;
					};
				return distanceSquared(lhs) < distanceSquared(rhs);
			});
	}

	for (const auto& coord : prioritizedChunks)
	{
		if (availableSlots == 0)
			break;

		const auto chunkIter = m_Chunks->find(coord);
		if (chunkIter == m_Chunks->end() || !chunkIter->second.CanAutoLoad())
			continue;

		if (SUCCEEDED(RequestLoadChunkAsync(coord, mapRootPath)))
			--availableSlots;
	}
}

HRESULT CMapChunkStreamer::RequestLoadChunkAsync(const MAPCHUNK_COORD& coord, const std::string& mapRootPath)
{
	const auto chunkIter = m_Chunks->find(coord);
	if (chunkIter == m_Chunks->end())
		return E_FAIL;

	CMapChunk& chunk = chunkIter->second;
	if (chunk.GetLoadState() == EChunkLoadState::Loaded || chunk.GetLoadState() == EChunkLoadState::Loading)
		return S_OK;

	if (chunk.GetFilePath().empty())
	{
		chunk.SetFilePath((std::filesystem::path("chunks") / m_Serializer->MakeChunkFileName(coord)).generic_string());
	}

	const std::filesystem::path chunkPath = std::filesystem::path(mapRootPath) / chunk.GetFilePath();
	const uint64_t mapGeneration = m_MapGeneration.load(std::memory_order_acquire);

	// 큐 제출 전에 Loading 상태로 바꿔 같은 청크가 중복 요청되는 것을 막음
	chunk.BeginLoading();
	m_AsyncLoadsInFlight.fetch_add(1, std::memory_order_acq_rel);

	const _bool queued = CGameInstance::Get().WorkerEnqueue(
		"LoadChunk",
		[this, coord, chunkPath, mapGeneration]()
		{
			CHUNK_LOAD_RESULT result{};
			result.coord = coord;
			result.mapGeneration = mapGeneration;

			try
			{
				MAP_CHUNK_FILE_DATA chunkFileData{};
				if (FAILED(m_Serializer->LoadChunkFile(chunkPath, chunkFileData)))
					result.hr = E_FAIL;
				else
				{
					result.objects = std::move(chunkFileData.objects);
					if (result.mapGeneration !=
						m_MapGeneration.load(std::memory_order_acquire))
					{
						result.hr = E_ABORT;
					}
					else
					{
						result.hr = m_ResourceTracker->PreloadResources(
							result.objects, result.modelResources);
					}
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
			m_AsyncLoadsInFlight.fetch_sub(1, std::memory_order_acq_rel);
		});

	if (!queued)
	{
		m_AsyncLoadsInFlight.fetch_sub(1, std::memory_order_acq_rel);
		chunk.CancelLoading();
		return E_FAIL;
	}

	return S_OK;
}

void CMapChunkStreamer::ProcessLoadedChunkResults(const _float3& chunkSize)
{
	ZoneScopedN("ChunkApplyBudget");
	std::vector<CHUNK_LOAD_RESULT> completedLoads;
	{
		std::lock_guard<std::mutex> lock(m_LoadResultMutex);
		completedLoads.swap(m_LoadResults);
	}

	for (auto& result : completedLoads)
	{
		auto applyState = std::make_unique<CHUNK_APPLY_STATE>();
		applyState->result = std::move(result);
		m_ApplyQueue.push_back(std::move(applyState));
	}

	const auto deadline = std::chrono::steady_clock::now() + CHUNK_APPLY_BUDGET;
	while (!m_ApplyQueue.empty())
	{
		_bool completed = false;
		ContinueApplyLoadedChunkResult(*m_ApplyQueue.front(), deadline, chunkSize, completed);

		if (completed)
			m_ApplyQueue.pop_front();

		if (!completed || std::chrono::steady_clock::now() >= deadline)
			break;
	}
}

HRESULT CMapChunkStreamer::ContinueApplyLoadedChunkResult(
	CHUNK_APPLY_STATE& state,
	const std::chrono::steady_clock::time_point& deadline,
	const _float3& chunkSize,
	_bool& completed)
{
	completed = false;
	if (state.result.mapGeneration != m_MapGeneration.load(std::memory_order_acquire))
	{
		if (!state.initialized)
			m_ResourceTracker->ReleasePreloadedResources(state.result.modelResources);
		completed = true;
		return S_OK;
	}

	const auto chunkIter = m_Chunks->find(state.result.coord);
	if (chunkIter == m_Chunks->end())
	{
		if (!state.initialized)
			m_ResourceTracker->ReleasePreloadedResources(state.result.modelResources);
		completed = true;
		return E_FAIL;
	}

	CMapChunk& chunk = chunkIter->second;
	if (FAILED(state.result.hr))
	{
		m_ResourceTracker->ReleasePreloadedResources(state.result.modelResources);
		chunk.CancelLoading();
		completed = true;
		return E_FAIL;
	}

	if (m_IsEnabled && !IsChunkInStreamingRange(state.result.coord, chunkSize))
	{
		if (state.initialized)
			m_UnloadChunkCallback(state.result.coord);
		else
		{
			m_ResourceTracker->ReleasePreloadedResources(state.result.modelResources);
			chunk.CancelLoading();
		}
		completed = true;
		return S_OK;
	}

	if (!state.initialized)
	{
		chunk.BeginLoading();
		if (FAILED(m_ResourceTracker->CommitPreloadedResources(chunk, state.result.modelResources)))
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

		const auto& objectData = state.result.objects[state.nextObjectIndex++];
		auto objectHandle = m_ObjectFactory->CreateMapMeshObject(objectData, false);
		if (!objectHandle)
			continue;

		chunk.AddObject(*objectHandle);
	}
	while (std::chrono::steady_clock::now() < deadline);

	if (state.nextObjectIndex < state.result.objects.size())
		return S_OK;

	chunk.CompleteLoading(MakeChunkBoundingBox(state.result.coord, chunkSize), EChunkSaveState::Saved);
	if (FAILED(CGameInstance::Get().RegisterMapMeshResidentChunk(state.result.coord, chunk.GetObjectHandles())))
	{
		m_UnloadChunkCallback(state.result.coord);
		completed = true;
		return E_FAIL;
	}

	if (!chunk.GetObjectHandles().empty())
	{
		CGameInstance::Get().Notify_StaticShadowSceneChanged(chunk.GetCullingBounds());
	}
	completed = true;

	return S_OK;
}

_bool CMapChunkStreamer::IsChunkInStreamingRange(const MAPCHUNK_COORD& coord, const _float3& chunkSize) const
{
	const auto* camera = CGameInstance::Get().GetActiveCamera();
	if (!camera)
		return false;

	const auto loadChunks = GetChunksAroundCamera(camera, STREAM_LOAD_DIAMETER, chunkSize);

	return std::find(loadChunks.begin(), loadChunks.end(), coord) != loadChunks.end();
}
