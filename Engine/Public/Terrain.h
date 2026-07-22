#pragma once

#include "GameObject.h"
#include "TerrainChunk.h"

NS_BEGIN(Engine)

class CComConstantBuffer;
class CResPixelShader;
class CResTexture2D;
class CResVertexShader;

class ENGINE_DLL CTerrain final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CTerrain, CGameObject)

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_string heightMapPath{};
		_string textureGroup{};
		_string textureTag{};
		_string shaderGroup{};
		_string vertexShaderTag{};
		_string pixelShaderTag{};
		uint32_t chunkQuadCount = 150;
		uint32_t vertexCountX = 151;
		uint32_t vertexCountZ = 151;
		_float vertexSpacing = 1.f;
		_float heightScale = 0.1f;
		uint32_t maskResolution = 256;
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
	_float GetVertexHeight(uint32_t x, uint32_t z) const;
	bool TryGetLocalHeight(_float localX, _float localZ, _float& outHeight) const;
	HRESULT SetVertexHeight(uint32_t x, uint32_t z, _float height);
	HRESULT CommitHeightRegion(uint32_t minX, uint32_t minZ, uint32_t maxX, uint32_t maxZ);
	HRESULT SetTileTexture(uint32_t layer, SPtr<CResTexture2D> texture);
	SPtr<CResTexture2D> GetTileTexture(uint32_t layer) const;
	HRESULT PaintTileLocal(const _float2& center, const _float2& radius,
		uint32_t layer, _float opacity, _float falloff);
	HRESULT AddChunkPositiveX();
	HRESULT AddChunkPositiveZ();
	HRESULT AddChunkNegativeX();
	HRESULT AddChunkNegativeZ();
	HRESULT SaveTerrain(const _string& metadataPath) const;
	HRESULT LoadTerrain(const _string& metadataPath);

private:
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

private:
	std::array<SPtr<CResTexture2D>, 4> m_pTerrainTextures{};
	SPtr<CResPixelShader> m_pPixelShader{};
	SPtr<CResVertexShader> m_pVertexShader{};
	CComConstantBuffer* m_pCBufferPerObject = nullptr;
	ComPtr<ID3D11Buffer> m_pChunkCBuffer{};
	uint32_t m_iVertexCountX = 0;
	uint32_t m_iVertexCountZ = 0;
	uint32_t m_iChunkQuadCount = 0;
	uint32_t m_iMaskResolution = 256;
	_float m_fVertexSpacing = 1.f;
	std::vector<VTX_NORMAL_TEX> m_Vertices{};
	std::vector<uint32_t> m_Indices{};
	std::vector<UPtr<CTerrainChunk>> m_Chunks{};
	std::vector<CTerrainChunk*> m_VisibleChunks{};
	std::unordered_map<uint64_t, CTerrainChunk*> m_ChunkLookup{};
	BoundingBox m_LocalBounds{};

public:
	static UPtr<CTerrain> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
