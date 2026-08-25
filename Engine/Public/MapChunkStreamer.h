#pragma once

#include "Engine_Base.h"
#include "MapChunk.h"
#include "MapChunkSerializer.h"
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <mutex>

NS_BEGIN(Engine)

class CMapModelResourceTracker;
class CMapRuntimeObjectFactory;
class CCameraObject;

// 카메라 위치를 기준으로 청크의 비동기 로드·언로드 일정을 관리한다.
// 워커 결과 큐, 동시 로드 제한, 오래된 결과 무효화, 프레임 적용 예산을 직접 소유한다.
class ENGINE_DLL CMapChunkStreamer final
{
public:
	using CHUNK_MAP = std::unordered_map<MAPCHUNK_COORD, CMapChunk, tagMapChunkCoordHash>;
	using UNLOAD_CHUNK_CALLBACK = std::function<HRESULT(const MAPCHUNK_COORD&)>;

public:
	// Streamer가 사용할 청크 저장소와 서비스, 실제 언로드 함수를 연결한다.
	HRESULT Initialize(
		CHUNK_MAP& chunks,
		CMapChunkSerializer& serializer,
		CMapModelResourceTracker& resourceTracker,
		CMapRuntimeObjectFactory& objectFactory,
		UNLOAD_CHUNK_CALLBACK unloadChunkCallback);

	// 완료 결과를 월드에 반영하고 카메라 주변 청크의 로드·언로드를 갱신한다.
	void Update(const std::string& mapRootPath, const _float3& chunkSize);

	// 맵 교체 전에 호출하여 이전 맵에서 출발한 워커 결과를 무효화한다.
	void InvalidatePendingLoads();

	void SetEnabled(_bool enabled) { m_IsEnabled = enabled; }
	_bool IsEnabled() const { return m_IsEnabled; }

private:
	// 워커가 읽은 청크 파일과 사전 로드한 모델 목록을 메인 스레드로 전달한다.
	struct CHUNK_LOAD_RESULT
	{
		MAPCHUNK_COORD coord{};
		uint64_t mapGeneration{};
		HRESULT hr = E_FAIL;
		std::vector<MAP_MESH_OBJECT_FILE_DATA> objects{};
		std::vector<MAP_MODEL_RESOURCE_KEY> modelResources{};
	};

	// 한 프레임에 모두 생성하지 못한 청크 오브젝트의 적용 진행 상태다.
	struct CHUNK_APPLY_STATE
	{
		CHUNK_LOAD_RESULT result{};
		size_t nextObjectIndex{};
		_bool initialized{};
	};

private:
	std::vector<MAPCHUNK_COORD> GetChunksAroundCamera(const CCameraObject* camera, int64_t diameter, const _float3& chunkSize) const;
	MAPCHUNK_COORD WorldToChunkCoord(const _float3& position, const _float3& chunkSize) const;
	BoundingBox MakeChunkBoundingBox(const MAPCHUNK_COORD& coord, const _float3& chunkSize) const;

	void UnloadChunksOutsideRange(const std::vector<MAPCHUNK_COORD>& retainedChunks);
	void RequestNeededChunkLoads(const std::vector<MAPCHUNK_COORD>& loadChunks, const std::string& mapRootPath, const _float3& chunkSize);
	HRESULT RequestLoadChunkAsync(const MAPCHUNK_COORD& coord, const std::string& mapRootPath);

	void ProcessLoadedChunkResults(const _float3& chunkSize);
	HRESULT ContinueApplyLoadedChunkResult(CHUNK_APPLY_STATE& state, const std::chrono::steady_clock::time_point& deadline, const _float3& chunkSize,_bool& completed);
	_bool IsChunkInStreamingRange(const MAPCHUNK_COORD& coord, const _float3& chunkSize) const;

private:
	static constexpr int64_t STREAM_LOAD_DIAMETER = 6;
	static constexpr int64_t STREAM_UNLOAD_DIAMETER = 7;
	static constexpr uint32_t MAX_CONCURRENT_CHUNK_LOADS = 4;
	static constexpr auto CHUNK_APPLY_BUDGET = std::chrono::microseconds(2000);

	// MapManager가 소유하며 Streamer보다 오래 유지되는 협력 객체들이다.
	CHUNK_MAP* m_Chunks{};
	CMapChunkSerializer* m_Serializer{};
	CMapModelResourceTracker* m_ResourceTracker{};
	CMapRuntimeObjectFactory* m_ObjectFactory{};
	UNLOAD_CHUNK_CALLBACK m_UnloadChunkCallback{};

	_bool m_IsEnabled = true;
	std::atomic_uint64_t m_MapGeneration{};
	std::atomic_uint32_t m_AsyncLoadsInFlight{};

	// 워커 완료 결과는 mutex로 보호하고, 적용 큐는 메인 스레드에서만 사용한다.
	std::mutex m_LoadResultMutex{};
	std::vector<CHUNK_LOAD_RESULT> m_LoadResults{};
	std::deque<std::unique_ptr<CHUNK_APPLY_STATE>> m_ApplyQueue{};
};

NS_END
