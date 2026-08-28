#pragma once

#include "GameObject.h"
#include "TerrainChunk.h"

NS_BEGIN(Engine)

class CComConstantBuffer;
class CResPixelShader;
class CResTexture2D;
class CResVertexShader;
class CResPhysXMaterial;
class CResPhysXTriMeshGeometry;
class CComPxRigidBody;
class CComPxTriMeshCollider;

// CTerrain
// ├─ 전체 지형 원본 정점 / 인덱스 보관
// ├─ 지형을 여러 CTerrainChunk로 분할
// ├─ 높이·텍스처 편집 관리
// ├─ 변경된 청크를 GPU에 반영
// ├─ 보이는 청크만 선별하여 렌더링
// └─ Terrain 저장 / 불러오기

class ENGINE_DLL CTerrain final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(Engine::CTerrain, CGameObject)

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_string heightMapPath{};
		_string textureGroup{};
		_string textureTag{};
		// 청크 한 개가 X/Z 방향으로 가지는 Quad 개수
		// 150이면 일반 청크는 151 x 151개의 정점을 가진다
		uint32_t chunkQuadCount = 150;
		uint32_t vertexCountX = 151;
		uint32_t vertexCountZ = 151;
		// 인접한 Terrain 정점 사이의 로컬 공간 간격
		_float vertexSpacing = 1.f;
		// Height Map 픽셀값을 실제 Terrain 높이로 변환하는 배율
		_float heightScale = 0.1f;
		// 청크별 텍스처 Blend Mask의 가로·세로 해상도
		uint32_t maskResolution = 256;
		PX_FILTER_DESC tPhysicsFilter{};
	};

private:
	CTerrain();
	CTerrain(const CTerrain& prototype);
	~CTerrain() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* context, const RENDER_CTX& renderContext) override;
	bool IsOcclusionCullable() const override;
	bool GetOcclusionBounds(BoundingBox& outBounds) const override;

public:
	const std::vector<VTX_NORMAL_TEX>& GetVertices() const { return m_Vertices; }
	const std::vector<uint32_t>& GetIndices() const { return m_Indices; }
	const std::vector<UPtr<CTerrainChunk>>& GetChunks() const { return m_Chunks; }
	const std::vector<CTerrainChunk*>& GetVisibleChunks() const { return m_VisibleChunks; }

	uint32_t GetVertexCountX() const { return m_iVertexCountX; }
	uint32_t GetVertexCountZ() const { return m_iVertexCountZ; }
	uint32_t GetVisibleChunkCount() const { return static_cast<uint32_t>(m_VisibleChunks.size()); }
	uint32_t GetChunkQuadCount() const { return m_iChunkQuadCount; }
	_float GetVertexSpacing() const { return m_fVertexSpacing; }
	uint32_t GetPhysicsEnabledChunkCount() const;
	_bool IsChunkPhysicsEnabled(uint32_t chunkX, uint32_t chunkZ) const;
	HRESULT SetChunkPhysicsEnabled(uint32_t chunkX, uint32_t chunkZ, _bool enabled);

	_float GetVertexHeight(uint32_t x, uint32_t z) const;
	bool TryGetLocalHeight(_float localX, _float localZ, _float& outHeight) const;
	HRESULT SetVertexHeight(uint32_t x, uint32_t z, _float height);
	HRESULT CommitAllHeights();
	HRESULT CommitHeightRegion(uint32_t minX, uint32_t minZ, uint32_t maxX, uint32_t maxZ);
	HRESULT SetTileTexture(uint32_t layer, SPtr<CResTexture2D> texture);
	SPtr<CResTexture2D> GetTileTexture(uint32_t layer) const;
	HRESULT PaintTileLocal(const _float2& center, const _float2& radius, uint32_t layer, _float opacity, _float falloff);
	HRESULT AddChunkPositiveX();
	HRESULT AddChunkPositiveZ();
	HRESULT AddChunkNegativeX();
	HRESULT AddChunkNegativeZ();
	HRESULT SaveTerrain(const _string& metadataPath) const;
	HRESULT LoadTerrain(const _string& metadataPath, std::optional<CHandle> physicsTarget = std::nullopt);
	void SetPhysicsTarget(std::optional<CHandle> target);
private:
	std::optional<TERRAIN_CHUNK_COORD> GetChunkCoordFromLocalPosition(const _float3& localPosition) const;
	HRESULT LoadHeightMap(const DESC& desc);
	HRESULT CreateFlatTerrain(const DESC& desc);
	void BuildGridIndices();
	HRESULT BuildChunks(uint32_t chunkQuadCount, _float vertexSpacing, uint32_t maskResolution);
	UPtr<CTerrainChunk> CreateChunk(uint32_t chunkX, uint32_t chunkZ) const;
	HRESULT ExpandTerrain(bool positiveX);
	HRESULT PrependTerrain(bool negativeX);
	void RebuildChunkLookup();
	CTerrainChunk* FindChunk(uint32_t chunkX, uint32_t chunkZ) const;
	void RecalculateNormals();
	void RecalculateNormals(uint32_t minX, uint32_t minZ, uint32_t maxX, uint32_t maxZ);
	void RecalculateBounds();
	void ExpandBoundsForRegion(uint32_t minX, uint32_t minZ, uint32_t maxX, uint32_t maxZ);
	void UpdateChunkVisibility();
	HRESULT UpdateChunks(uint32_t minX, uint32_t minZ, uint32_t maxX, uint32_t maxZ);
	void UpdateChunkPhysX();
	void ClearPhysicsChunks();

private:
	// --------------------렌더링----------------------
	std::array<SPtr<CResTexture2D>, 4> m_pTerrainTextures{};
	SPtr<CResPixelShader> m_pPixelShader{};
	SPtr<CResVertexShader> m_pVertexShader{};
	CComConstantBuffer* m_pCBufferPerObject = nullptr;
	// 렌더링 중인 청크의 위치 및 마스크 정보를 전달하는 청크 전용 Constant Buffer
	ComPtr<ID3D11Buffer> m_pChunkCBuffer{};
	// -----------------------------------------------

	// 전체 Terrain의 X/Z 방향 정점 개수
	uint32_t m_iVertexCountX = 0;
	uint32_t m_iVertexCountZ = 0;

	// 청크 한 개가 X/Z 방향으로 가지는 Quad 개수
	uint32_t m_iChunkQuadCount = 0;

	// 청크별 RGBA Blend Mask 해상도
	uint32_t m_iMaskResolution = 256;

	// 인접한 정점 사이의 로컬 공간 거리
	_float m_fVertexSpacing = 1.f;

	// 전체 지형의 원본
	std::vector<VTX_NORMAL_TEX> m_Vertices{}; // 높이 편집은 이 배열을 먼저 수정한 후 해당 청크에 반영
	std::vector<uint32_t> m_Indices{};

	// 소유 TerrainChunk
	std::vector<UPtr<CTerrainChunk>> m_Chunks{};
	std::vector<CTerrainChunk*> m_VisibleChunks{};

	// 청크 좌표로 빠르게 TerrainChunk검색
	std::unordered_map<uint64_t, CTerrainChunk*> m_ChunkLookup{};

	// Terrain GameObject가 하나의 static actor를 소유하고, 활성 청크마다 TriMesh shape를 붙인다.
	CComPxRigidBody* m_pTerrainRigidBody{};
	SPtr<CResPhysXMaterial> m_pTerrainPhysicsMaterial{};
	PX_FILTER_DESC m_tTerrainPhysicsFilter{};
	// Physics 스트리밍 기준 타겟 (플레이어), 없으면 PhysX 리소스 로드 X
	std::optional<CHandle> m_PhysicsTarget{};
	TERRAIN_CHUNK_COORD m_CurrentPhysicsCenter{ -1,-1 };
	// 처음맵로드엔 경로만 저장해둠
	std::unordered_map<uint64_t, _string> m_PhysicsCookedPaths{};

	// PhysX 활성 청크 런타임 데이터
	struct TERRAIN_PHYSICS_RUNTIME
	{
		TERRAIN_CHUNK_COORD coord{};
		SPtr<CResPhysXTriMeshGeometry> mesh{};
		CComPxTriMeshCollider* collider{};
		_string componentTag{};
	};
	std::unordered_map<uint64_t, TERRAIN_PHYSICS_RUNTIME> m_ActivePhysicsChunks{};

	// 전체 터레인 감싸는 로컬공간 바운딩박스
	BoundingBox m_LocalBounds{};

public:
	static UPtr<CTerrain> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
