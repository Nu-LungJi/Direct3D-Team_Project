#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)

struct TERRAIN_CHUNK_COORD
{
	int64_t x = 0;
	int64_t z = 0;

	bool operator==(const TERRAIN_CHUNK_COORD& rhs) const
	{
		return x == rhs.x && z == rhs.z;
	}
};

struct TERRAIN_CHUNK_DESC
{
	TERRAIN_CHUNK_COORD coord{};
	uint32_t vertexCountX = 0;
	uint32_t vertexCountZ = 0;
	_float vertexSpacing = 1.f;
	uint32_t maskResolution = 256;
	std::vector<VTX_NORMAL_TEX> vertices{};
	std::vector<uint32_t> indices{};
};

struct TERRAIN_MASK_DIRTY_RECT
{
	uint32_t left = 0;
	uint32_t top = 0;
	uint32_t right = 0;
	uint32_t bottom = 0;
	bool valid = false;
};

//CTerrainChunk
//├─ 담당 구역의 정점 / 인덱스 복사본
//├─ Vertex / Index Buffer
//├─ 텍스처 Blend Mask
//├─ Blend Mask Texture와 SRV
//└─ 해당 구역의 Bounds

class ENGINE_DLL CTerrainChunk final : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CTerrainChunk, CEngineBase)

private:
	CTerrainChunk(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context);
	~CTerrainChunk() override = default;

public:
	const TERRAIN_CHUNK_COORD& GetCoord() const { return m_Coord; }
	void SetCoord(const TERRAIN_CHUNK_COORD& coord) { m_Coord = coord; }
	uint32_t GetVertexCountX() const { return m_iVertexCountX; }
	uint32_t GetVertexCountZ() const { return m_iVertexCountZ; }
	_float GetVertexSpacing() const { return m_fVertexSpacing; }
	const BoundingBox& GetLocalBounds() const { return m_LocalBounds; }
	const std::vector<VTX_NORMAL_TEX>& GetVertices() const { return m_Vertices; }
	const std::vector<uint32_t>& GetIndices() const { return m_Indices; }
	uint32_t GetMaskResolution() const { return m_iMaskResolution; }
	ID3D11ShaderResourceView* GetBlendMaskSRV() const { return m_pBlendMaskSRV.Get(); }
	const std::vector<uint8_t>& GetBlendMask() const { return m_BlendMask; }

public:
	HRESULT UpdateVertices(const std::vector<VTX_NORMAL_TEX>& vertices);
	bool PaintBlendMask(const _float2& center, const _float2& radius,
		uint32_t layer, _float opacity, _float falloff, TERRAIN_MASK_DIRTY_RECT& dirtyRect);
	HRESULT UploadBlendMask(const TERRAIN_MASK_DIRTY_RECT* dirtyRect = nullptr);
	HRESULT SetBlendMask(const std::vector<uint8_t>& mask);
	void Bind(ID3D11DeviceContext* context) const;
	void Draw(ID3D11DeviceContext* context) const;

private:
	HRESULT Initialize(const TERRAIN_CHUNK_DESC& desc);
	void RecalculateBounds();

private:
	ComPtr<ID3D11Device> m_pDevice{};
	ComPtr<ID3D11DeviceContext> m_pContext{};
	ComPtr<ID3D11Buffer> m_pVertexBuffer{};
	ComPtr<ID3D11Buffer> m_pIndexBuffer{};
	ComPtr<ID3D11Texture2D> m_pBlendMaskTexture{};
	ComPtr<ID3D11ShaderResourceView> m_pBlendMaskSRV{};

	TERRAIN_CHUNK_COORD m_Coord{};
	uint32_t m_iVertexCountX = 0;
	uint32_t m_iVertexCountZ = 0;
	_float m_fVertexSpacing = 1.f;
	uint32_t m_iMaskResolution = 0;

	// 청크별 CPU Blend Mask와 Terrain 로컬 공간 Bounds
	std::vector<uint8_t> m_BlendMask{};
	BoundingBox m_LocalBounds{};

	// 전체 Terrain에서 복사한 청크 범위의 CPU Mesh 데이터
	std::vector<VTX_NORMAL_TEX> m_Vertices{};
	std::vector<uint32_t> m_Indices{};

public:
	static UPtr<CTerrainChunk> Create(const TERRAIN_CHUNK_DESC& desc);
};

NS_END
