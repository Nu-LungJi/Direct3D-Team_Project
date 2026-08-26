#pragma once

#include "Engine_Base.h"
#include "MapChunk.h"
#include "MapChunkStreamer.h"
#include "MapChunkSerializer.h"
#include "MapMaterialRepository.h"
#include "MapModelResourceTracker.h"
#include "MapRuntimeObjectFactory.h"

NS_BEGIN(Engine)
#ifdef _DEBUG
class CMapChunkDebugRenderer;
#endif

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
	void Update(_float);

public:
	// 씬 또는 맵을 교체하기 전에 모든 청크와 진행 중인 스트리밍 결과를 정리
	void ClearAllChunk();

public:
	// 맵 메타데이터와 청크 파일 전체의 저장,로드 과정을 조율
	HRESULT SaveMap(const std::string& path);
	HRESULT LoadMap(const std::string& path, _bool clearBeforeLoad = true);
	HRESULT SaveChunk(const MAPCHUNK_COORD& coord, const std::string& chunkPath);

	HRESULT LoadMapData(const std::string& path);
	// 저장과 편집 도구에서 사용하는 메인 스레드 동기 청크 로드
	HRESULT LoadChunk(const MAPCHUNK_COORD& coord);
	HRESULT UnLoadChunk(const MAPCHUNK_COORD& coord);

	HRESULT SaveMaterial(const std::string& path);
	HRESULT LoadMaterial(const std::string& path);
	MATERIAL_DESC FindMaterial(const std::string& modelName) const;

	// 모델 태그 -> 모델 .bin 파일 경로
	void SetMapModelResourceIndex(const std::filesystem::path& staticModelRoot, const std::string& resourceGroup, 
		std::unordered_map<std::string, std::filesystem::path> modelPaths);

public:
	void RebuildChunks();
	HRESULT RegisterMapMeshObject(const CHandle& hObject);
	HRESULT RefreshMapMeshObject(const CHandle& hObject);
	HRESULT UnregisterMapMeshObject(const CHandle& hObject);
	std::vector<CHandle> CollectMapMeshPickCandidates(FXMVECTOR rayOrigin, FXMVECTOR rayDirection) const;
	const std::unordered_map<MAPCHUNK_COORD, CMapChunk, tagMapChunkCoordHash>& GetChunks() const { return m_Chunks; }
	const _float3& GetChunkSize() const { return m_ChunkSize; }
	void SetChunkStreaming(_bool enable) { m_ChunkStreamer.SetEnabled(enable); }
	_bool IsChunkStreaming() const { return m_ChunkStreamer.IsEnabled(); }

private:
	_float3 GetChunkCenter(const MAPCHUNK_COORD& coord);
	BoundingBox MakeChunkBoundingBox(const MAPCHUNK_COORD& coord);
	MAPCHUNK_COORD WorldToChunkCoord(const _float3& pos) const;
	// 월드의 모델 머티리얼을 수집하고 Repository의 값을 런타임 모델에 반영
	CMapMaterialRepository::MATERIAL_MAP CollectMapMaterials() const;
	void ApplyStoredMaterialsToLoadedModels() const;
private:
	_float3 m_ChunkSize = DEFAULT_MAP_CHUNK_SIZE;
	std::string m_MapRootPath;

private:
	std::unordered_map<MAPCHUNK_COORD, CMapChunk, tagMapChunkCoordHash> m_Chunks;
	CMapChunkSerializer m_ChunkSerializer;
	CMapMaterialRepository m_MaterialRepository;
	CMapModelResourceTracker m_ModelResourceTracker; // 청크 모델의 로드 동기화, 참조 수, 지연 해제를 전담
	CMapRuntimeObjectFactory m_ObjectFactory; // 런타임 맵 오브젝트와 저장 데이터 사이의 변환 및 생성을 전담
	CMapChunkStreamer m_ChunkStreamer; // 카메라 기반 청크 선택과 비동기 로드 결과 적용을 전담

#ifdef _DEBUG
public:
	HRESULT RenderDebugMapChunk();
	void SetDebugDrawMapChunk(_bool draw);

private:
	std::unique_ptr<CMapChunkDebugRenderer> m_ChunkDebugRenderer;
#endif

public:
	static UPtr<CMapManager> Create();

public:
	void Free() override;

};

NS_END



