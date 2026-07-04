#pragma once

#ifdef _DEBUG
#include "DebugDraw.h"
#include <directxtk/Effects.h>
#endif

#include <functional>
#include "Engine_Base.h"

NS_BEGIN(Engine)
typedef struct tagMapChunk
{
	MAPCHUNK_COORD coord{};
	std::vector<CHandle> hObjects{};
	BoundingBox bounds{};
	_bool m_bDirty = false;
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
	HRESULT SaveMap(const std::string& path);
	HRESULT LoadMap(const std::string& path, _bool clearBeforeLoad = true);

// ---------------------------------MapChunk-----------------------------------
public:
	void RebuildChunks();
	const std::unordered_map<MAPCHUNK_COORD, MAPCHUNK, tagMapChunkCoordHash>& GetChunks() const { return m_Chunks; }
	const _float3& GetChunkSize() const { return m_vChunkSize; }

private:
	_float3 GetChunkCenter(const MAPCHUNK_COORD& coord);
	BoundingBox MakeChunkBoundingBox(const MAPCHUNK_COORD& coord);
	MAPCHUNK_COORD WorldToChunkCoord(const _float3& pos) const;
private:
	_float3 m_vChunkSize = { 25.f, 25.f, 25.f };

private:
	std::unordered_map<MAPCHUNK_COORD, MAPCHUNK, tagMapChunkCoordHash> m_Chunks;


// ---------------------------------MapChunk-----------------------------------

#ifdef _DEBUG
	// Debug Draw
	// Debug Draw를 위한 도구들
	// static으로 관리
	static std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> _batch;
	static std::unique_ptr<BasicEffect> _effect;
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

