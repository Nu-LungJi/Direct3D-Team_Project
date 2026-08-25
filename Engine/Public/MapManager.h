#pragma once

#ifdef _DEBUG
#include "DebugDraw.h"
#include <directxtk/Effects.h>
#endif

#include <functional>
#include <chrono>
#include <deque>
#include "Engine_Base.h"
#include "MapChunk.h"
#include "MapChunkSerializer.h"
#include "MapMaterialRepository.h"

NS_BEGIN(Engine)
class COctreeNode;
class CCameraObject;
class CMapMeshObject;
class CDecalVolume;

struct tagMapChunkCoordHash
{
	size_t operator()(const MAPCHUNK_COORD& coord) const
	{
		size_t h1 = std::hash<int64_t>{}(coord.x);
		size_t h2 = std::hash<int64_t>{}(coord.y);
		size_t h3 = std::hash<int64_t>{}(coord.z);

		return h1 ^ (h2 << 1) ^ (h3 << 3);
	}
};

// 전방선언
struct PENDING_CHUNK_LOAD_RESULT;
struct PENDING_CHUNK_APPLY_STATE;

constexpr _float3 DEFAULT_MAP_CHUNK_SIZE{ 150.f, 150.f, 150.f };

class ENGINE_DLL CMapManager : public CEngineBase
{
public:
	CMapManager(const CMapManager&) = delete;
	CMapManager& operator=(const CMapManager& rhs) = delete;

private:
	CMapManager();
	~CMapManager() override;

private:
	HRESULT Initialize();

public:
	void PriorityUpdate(_float fTimeDelta);
	void Update(_float fTimeDelta);
	void LateUpdate(_float fTimeDelta);

public:
	void ClearAllChunk(); // 씬 전환할 때 Clear하셈 


public:
	HRESULT SaveMap(const std::string& path); // 메타 + 모든 청크 저장
	HRESULT LoadMap(const std::string& path, _bool clearBeforeLoad = true); // 메타 + 모든 청크 불러오기
	HRESULT SaveChunk(const MAPCHUNK_COORD& coord, const std::string& chunkPath); // 청크 단위 저장

	HRESULT LoadMapData(const std::string& path);
	HRESULT LoadChunk(const MAPCHUNK_COORD& coord); // 메인스레드 동기 로드, 저장/툴용
	HRESULT UnLoadChunk(const MAPCHUNK_COORD& coord);

	HRESULT SaveMaterial(const std::string& path);
	HRESULT LoadMaterial(const std::string& path);
	MATERIAL_DESC FindMaterial(const std::string& modelName) const;

	// 모델 태그 → 모델 .bin 파일 경로
	void SetMapModelResourceIndex(const std::filesystem::path& staticModelRoot, const std::string& resourceGroup, std::unordered_map<std::string, std::filesystem::path> modelPaths);

// ---------------------------------MapChunk-----------------------------------
public:
	void RebuildChunks();
	HRESULT RegisterMapMeshObject(const CHandle& hObject);
	std::vector<CHandle> CollectMapMeshPickCandidates(FXMVECTOR rayOrigin, FXMVECTOR rayDirection) const;
	const std::unordered_map<MAPCHUNK_COORD, CMapChunk, tagMapChunkCoordHash>& GetChunks() const { return m_Chunks; }
	const _float3& GetChunkSize() const { return m_vChunkSize; }
	void SetChunkStreaming(_bool enable) { m_bChunkStreaming = enable; }
	_bool IsChunkStreaming() const { return m_bChunkStreaming; } 

private:
	_float3 GetChunkCenter(const MAPCHUNK_COORD& coord);
	BoundingBox MakeChunkBoundingBox(const MAPCHUNK_COORD& coord);
	MAPCHUNK_COORD WorldToChunkCoord(const _float3& pos) const;
	std::vector<MAPCHUNK_COORD> GetChunksAroundCamera(
		const CCameraObject* pCamera, int64_t diameter) const;
	void UnloadChunksOutsideRange(const std::vector<MAPCHUNK_COORD>& neededChunks);
	void RequestNeededChunkLoads(const std::vector<MAPCHUNK_COORD>& neededChunks);
	// CPU 프러스텀 컬링 비활성화
	//void CullLoadedChunksByCameraFrustum(const std::vector<MAPCHUNK_COORD>& neededChunks, const BoundingFrustum& boundingFrustum);


	HRESULT AcquireChunkModelResources(CMapChunk& chunk, const std::vector<MAP_MESH_OBJECT_FILE_DATA>& objects);
	HRESULT PreloadChunkModelResources(PENDING_CHUNK_LOAD_RESULT& result);
	HRESULT AcquirePreloadedChunkModelResources(CMapChunk& chunk, const PENDING_CHUNK_LOAD_RESULT& result);
	void ReleasePendingModelResources(const PENDING_CHUNK_LOAD_RESULT& result);

	// 청크가 런타임에 로드될 때 m_MapModelPaths에서 파일 경로를 찾고 해당 모델만 로드
	HRESULT EnsureModelResourceLoaded(const MAP_MODEL_RESOURCE_KEY& key);
	SPtr<std::mutex> GetModelResourceMutex(const MAP_MODEL_RESOURCE_KEY& key);

	void QueueChunkModelRelease(CMapChunk& chunk);
	void QueueAllChunkModelReleases();
	void ProcessDeferredModelReleases();

	// 런타임 게임 오브젝트와 파일 전용 데이터 사이의 변환을 담당한다.
	MAP_MESH_OBJECT_FILE_DATA MakeMapMeshObjectFileData(
		const CMapMeshObject& object,
		const std::string& layerName) const;
	std::optional<CHandle> CreateMapMeshObject(
		const MAP_MESH_OBJECT_FILE_DATA& objectData) const;
	MAP_DECAL_FILE_DATA MakeDecalFileData(
		const CDecalVolume& decal,
		const std::string& layerName) const;
	std::optional<CHandle> CreateDecal(const MAP_DECAL_FILE_DATA& decalData) const;

	// 월드의 모델 머티리얼을 수집하고 Repository의 값을 런타임 모델에 반영한다.
	CMapMaterialRepository::MATERIAL_MAP CollectMapMaterials() const;
	void ApplyStoredMaterialsToLoadedModels() const;
private:
	static constexpr int64_t STREAM_LOAD_DIAMETER = 6;   // 6 x 6 x 6
	static constexpr int64_t STREAM_UNLOAD_DIAMETER = 7; // 7 x 7 x 7
	static constexpr uint32_t MAX_CONCURRENT_CHUNK_LOADS = 4;
	_float3 m_vChunkSize = DEFAULT_MAP_CHUNK_SIZE;
	std::string m_sMapRootPath;

private:
	std::unordered_map<MAPCHUNK_COORD, CMapChunk, tagMapChunkCoordHash> m_Chunks;
	CMapChunkSerializer m_ChunkSerializer;
	CMapMaterialRepository m_MaterialRepository;
	_bool m_bChunkStreaming = true;

	std::atomic_uint64_t m_MapGeneration{};
	std::atomic_uint32_t m_AsyncChunkLoadsInFlight{};

	// 모델 태그 → 모델 .bin 파일 경로
	std::filesystem::path m_MapModelStaticRoot;
	std::string m_MapModelResourceGroup;
	std::unordered_map<std::string, std::filesystem::path> m_MapModelPaths;

	std::mutex m_MapModelResourceIndexMutex;
	std::mutex m_ModelResourceMutexMapMutex;
	std::unordered_map<MAP_MODEL_RESOURCE_KEY, SPtr<std::mutex>, MAP_MODEL_RESOURCE_KEY_HASH> m_ModelResourceMutexes;
	std::mutex m_PendingModelRefMutex;

	// 완전히 생성된 청크들이 모델을 사용 중
	std::unordered_map<MAP_MODEL_RESOURCE_KEY, size_t, MAP_MODEL_RESOURCE_KEY_HASH> m_ModelChunkRefCounts;
	// 워커 로드는 끝났지만 청크 생성은 아직 안 끝남
	std::unordered_map<MAP_MODEL_RESOURCE_KEY, size_t, MAP_MODEL_RESOURCE_KEY_HASH> m_PendingModelRefCounts;


	std::vector<std::vector<MAP_MODEL_RESOURCE_KEY>> m_DeferredModelReleases;
	std::vector<MAP_MODEL_RESOURCE_KEY> m_DeferredUnusedModelReleases;

// ---------------------------------MapChunk-----------------------------------

#ifdef _DEBUG
	// Debug Draw
	// Debug Draw를 위한 도구들
	// static으로 관리
	static std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> _batch;
	static std::unique_ptr<DirectX::BasicEffect> _effect;
	static ComPtr<ID3D11InputLayout> _inputLayout;
public:
	HRESULT RenderDebugMapChunk();
	void SetDebugDrawMapChunk(_bool draw) { m_bDebugDrawMapChunk = draw; }
private:
	void DrawBox(const DirectX::BoundingBox& box, DirectX::FXMVECTOR color, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world);

private:
	_bool m_bDebugDrawMapChunk = false;
#endif

// -------------------------------Worker---------------------------------------
public:
	HRESULT RequestLoadChunkAsync(const MAPCHUNK_COORD& coord); // 워커에게 청크 로딩 요청 // 스트리밍용 비동기 요청
	void ProcessLoadedChunkResults();

private:
	HRESULT ApplyLoadedChunkResult(const PENDING_CHUNK_LOAD_RESULT& result); // 실제 오브젝트 생성 // 메인스레드 월드 반영
	_bool IsChunkInStreamingRange(const MAPCHUNK_COORD& coord); // 나중에 도착한 Chunk로딩 결과가 유효한지 확인하기 위한 함수

private:
	HRESULT ContinueApplyLoadedChunkResult(PENDING_CHUNK_APPLY_STATE& state, const std::chrono::steady_clock::time_point& deadline, _bool& completed);
	std::mutex m_LoadResultMutex{};
	std::deque<std::unique_ptr<PENDING_CHUNK_APPLY_STATE>> m_ChunkApplyQueue{};
	std::vector<PENDING_CHUNK_LOAD_RESULT> m_LoadResults{}; //워커가 로딩한 결과모음
// -------------------------------Worker---------------------------------------

public:
	static UPtr<CMapManager> Create();

public:
	void Free() override;

};

struct PENDING_CHUNK_LOAD_RESULT
{
	MAPCHUNK_COORD coord{};
	uint64_t mapGeneration{};
	HRESULT hr = E_FAIL;
	std::vector<MAP_MESH_OBJECT_FILE_DATA> objects{};
	std::vector<MAP_MODEL_RESOURCE_KEY> modelResources{};
};
NS_END



