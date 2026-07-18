#pragma once
#include "Engine_Base.h"

NS_BEGIN(Engine)

class CResStructuredBuffer;
class CHizBuffer;

struct CB_MAPMESH_GPU_CULL
{
	_float4x4 matViewProj{};
	_float2 screenSize{};
	_float2 hizSize{};
	uint32_t instanceCount = 0;
	uint32_t mipCount = 0;
	uint32_t useHiz = 0;
	_float hizBias = 0.0005f;
};

class ENGINE_DLL CMapMeshGpuCuller : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CMapMeshGpuCuller, CEngineBase)

private:
	CMapMeshGpuCuller() = default;
	~CMapMeshGpuCuller() override = default;

public:
	HRESULT EnsureCapacity(uint32_t instanceCount, uint32_t batchCount, uint32_t drawCount);
	HRESULT BuildVisibleInstancesAndIndirectArgs(
		ID3D11DeviceContext* pContext,
		const std::vector<MAPMESH_INSTANCE_DATA>& instances,
		const std::vector<MAPMESH_OCCLUSION_DATA>& occlusionData,
		const std::vector<MAPMESH_CULL_META>& cullMeta,
		uint32_t batchCount,
		const std::vector<uint32_t>& drawBatchIndices,
		const std::vector<D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS>& indirectArgs,
		const CHizBuffer* pPrevHizBuffer,
		_matrix matViewProj,
		const _float2& screenSize);

	ID3D11Buffer* GetVisibleInstanceBuffer() const;
	ID3D11Buffer* GetIndirectArgsBuffer() const;

private:
	uint32_t m_iCapacity = 0;
	uint32_t m_iBatchCapacity = 0;
	uint32_t m_iDrawCapacity = 0;

	SPtr<CResStructuredBuffer> m_pInstanceInputBuffer{};
	SPtr<CResStructuredBuffer> m_pOcclusionInputBuffer{};
	SPtr<CResStructuredBuffer> m_pCullMetaInputBuffer{};
	SPtr<CResStructuredBuffer> m_pDrawBatchInputBuffer{};
	SPtr<CResStructuredBuffer> m_pBatchVisibleCountBuffer{};
	SPtr<CResStructuredBuffer> m_pVisibleInstanceBuffer{};

	ComPtr<ID3D11Buffer> m_pVisibleInstanceVertexBuffer{};
	ComPtr<ID3D11Buffer> m_pIndirectArgsBuffer{};
	ComPtr<ID3D11UnorderedAccessView> m_pIndirectArgsUAV{};
	ComPtr<ID3D11Buffer> m_pCBuffer{};
	ComPtr<ID3D11Buffer> m_pArgsCBuffer{};

public:
	static UPtr<CMapMeshGpuCuller> Create();
};

NS_END
