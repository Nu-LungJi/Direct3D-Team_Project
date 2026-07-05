#pragma once

#ifdef _DEBUG
#include "DebugDraw.h"
#include <directxtk/Effects.h>
#endif

#include <functional>
#include "Engine_Base.h"

NS_BEGIN(Engine)

enum class EChunkLoadState
{
	Unloading,  // 언로드 요청 중
	Unloaded,   // 메타만 있고 월드에는 없음
	Loading,    // 비동기 로딩 요청 중
	Loaded,     // 월드에 올라와 있음
};

enum class EChunkSaveState
{
	Unsaved,    // 아직 저장 파일 없음
	Saved,      // 저장 파일 있음
};

typedef struct tagMapChunk
{
	MAPCHUNK_COORD coord{};
	std::vector<CHandle> hObjects{};
	BoundingBox bounds{};

	EChunkLoadState loadState = EChunkLoadState::Unloaded;
	EChunkSaveState saveState = EChunkSaveState::Unsaved;

	std::string filePath;
}MAPCHUNK;

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
	HRESULT SaveMap(const std::string& path); // 메타 + 모든 청크 저장
	HRESULT LoadMap(const std::string& path, _bool clearBeforeLoad = true); // 메타 + 모든 청크 불러오기
	HRESULT SaveChunk(const MAPCHUNK_COORD& coord, const std::string& chunkPath); // 청크 단위 저장

	HRESULT LoadMapData(const std::string& path);
	HRESULT LoadChunk(const MAPCHUNK_COORD& coord);
	HRESULT UnLoadChunk(const MAPCHUNK_COORD& coord);

// ---------------------------------MapChunk-----------------------------------
public:
	void RebuildChunks();
	const std::unordered_map<MAPCHUNK_COORD, MAPCHUNK, tagMapChunkCoordHash>& GetChunks() const { return m_Chunks; }
	const _float3& GetChunkSize() const { return m_vChunkSize; }
	void SetChunkStreaming(_bool enable) { m_bChunkStreaming = enable; }
	_bool IsChunkStreaming() const { return m_bChunkStreaming; }

private:
	_float3 GetChunkCenter(const MAPCHUNK_COORD& coord);
	BoundingBox MakeChunkBoundingBox(const MAPCHUNK_COORD& coord);
	MAPCHUNK_COORD WorldToChunkCoord(const _float3& pos) const;
private:
	_float3 m_vChunkSize = { 50.f, 50.f, 50.f };
	std::string m_sMapRootPath;

private:
	std::unordered_map<MAPCHUNK_COORD, MAPCHUNK, tagMapChunkCoordHash> m_Chunks;
	_bool m_bChunkStreaming = true;


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
//public:
//	void FrameStart();
//	void FrameEnd();

public:
	static UPtr<CMapManager> Create();

public:
	void Free() override;

};

NS_END



