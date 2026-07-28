#pragma once

#include "Engine_NvClothDefines.h"
#include "Engine_Struct_Vertex.h"
#include "Resource.h"

#include <limits>

NS_BEGIN(Engine)

class ENGINE_DLL CResNvClothMesh final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResNvClothMesh, CResource)

	struct DESC
	{
		_matrix PreTransformMatrix;
		_string sSimulationAnchorBone{};
		uint32_t iSimulationMeshIndex{
			std::numeric_limits<uint32_t>::max() };
		uint32_t iRenderMeshIndex{
			std::numeric_limits<uint32_t>::max() };
		_float fWeldTolerance{ 1.e-5f };
		_float fFixedTopRatio{ 0.08f };
	};

	struct SECTION
	{
		uint32_t iSourceMeshIndex{};
		uint32_t iMaterialIndex{};
		uint32_t iVertexCount{};
		uint32_t iIndexCount{};
		uint32_t iVertexStride{};
		ComPtr<ID3D11Buffer> pVertexBuffer{};
		ComPtr<ID3D11Buffer> pIndexBuffer{};
	};

private:
	CResNvClothMesh(
		const _string& sPath,
		ComPtr<ID3D11Device> pDevice);
	~CResNvClothMesh() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

	const NVCLOTH_FABRIC_DESC& GetFabricDesc() const
	{
		return m_tFabricDesc;
	}

	const std::vector<SECTION>& GetSections() const
	{
		return m_Sections;
	}

	uint32_t GetParticleCount() const
	{
		return static_cast<uint32_t>(
			m_tFabricDesc.vecPositions.size());
	}

	static SPtr<CResNvClothMesh> Create(const _string& sPath);

private:
	ComPtr<ID3D11Device> m_pDevice{};
	NVCLOTH_FABRIC_DESC m_tFabricDesc{};
	std::vector<SECTION> m_Sections{};
};

NS_END
