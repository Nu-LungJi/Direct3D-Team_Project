#pragma once
#include "Engine_Base.h"

NS_BEGIN(Engine)

class CResStructuredBuffer;
class CHizBuffer;

// GPU 컬링 컴퓨트 셰이더에 전달하는 프레임 상수
struct CB_MAPMESH_GPU_CULL
{
	_float4x4 matViewProj{};
	// 투영 경계 계산에 사용하는 현재 렌더 해상도
	_float2 screenSize{};
	// 이전 프레임 Hi-Z 텍스처의 원본 해상도
	_float2 hizSize{};
	// 이번 Dispatch에서 검사할 전체 인스턴스 수
	uint32_t instanceCount = 0;
	// Hi-Z 텍스처에서 사용할 수 있는 밉 레벨 수
	uint32_t mipCount = 0;
	// 유효한 이전 Hi-Z가 있을 때만 오클루전 컬링을 활성화
	uint32_t useHiz = 0;
	// 깊이 정밀도 오차로 인한 잘못된 컬링을 완화하는 여유값
	_float hizBias = 0.0005f;
};

// CPU에서 받은 맵 메시 인스턴스를 GPU에서 프러스텀·Hi-Z 컬링하고,
// 가시 인스턴스 버퍼와 DrawIndexedInstancedIndirect 인자 버퍼를 생성한다
class ENGINE_DLL CMapMeshGpuCuller : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CMapMeshGpuCuller, CEngineBase)

private:
	CMapMeshGpuCuller() = default;
	~CMapMeshGpuCuller() override = default;

public:
	// 요청한 인스턴스·배치·Draw 수를 담을 수 있도록 GPU 버퍼를 확장한다
	// 기존 용량으로 충분하면 버퍼를 다시 만들지 않는다
	HRESULT EnsureCapacity(uint32_t instanceCount, uint32_t batchCount, uint32_t drawCount);
	// 청크 변경 프레임에만 컬링 입력을 GPU에 업로드하고, 매 프레임 컬링을 실행한다.
	// uploadResidentData가 false면 기존 GPU 입력을 그대로 재사용한다.
	HRESULT BuildVisibleInstancesAndIndirectArgs(
		ID3D11DeviceContext* context,
		const std::vector<MAPMESH_INSTANCE_DATA>& instances,
		const std::vector<MAPMESH_OCCLUSION_DATA>& occlusionData,
		const std::vector<MAPMESH_CULL_META>& cullMeta,
		uint32_t batchCount,
		const std::vector<uint32_t>& drawBatchIndices,
		const std::vector<D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS>& indirectArgs,
		_bool uploadResidentData,
		const CHizBuffer* previousHizBuffer,
		_matrix matViewProj,
		const _float2& screenSize);

	// 인스턴스 버텍스 스트림으로 바인딩할 컬링 결과 버퍼를 반환
	ID3D11Buffer* GetVisibleInstanceBuffer() const;
	// DrawIndexedInstancedIndirect에서 사용할 인자 버퍼를 반환
	ID3D11Buffer* GetIndirectArgsBuffer() const;

private:
	// 필요한 수를 수용하도록 현재 용량을 두 배 단위로 확장해 계산
	static uint32_t CalculateExpandedCapacity(
		uint32_t currentCapacity,
		uint32_t requiredCapacity,
		uint32_t minimumCapacity);
	// 요소 수와 용도에 맞는 구조화 버퍼를 생성
	static SPtr<CResStructuredBuffer> CreateStructuredBuffer(
		uint32_t elementCount,
		uint32_t stride,
		uint32_t bindFlags);

private:
	// 재할당 여부를 판단하기 위한 현재 GPU 버퍼별 최대 요소 수
	uint32_t m_InstanceCapacity = 0;
	uint32_t m_BatchCapacity = 0;
	uint32_t m_DrawCapacity = 0;

	// 청크 변경 시에만 CPU가 갱신하고 평상시에는 GPU에 상주하는 입력 버퍼
	SPtr<CResStructuredBuffer> m_pInstanceInputBuffer{};
	SPtr<CResStructuredBuffer> m_pOcclusionInputBuffer{};
	SPtr<CResStructuredBuffer> m_pCullMetaInputBuffer{};
	// 각 Draw가 어느 컬링 배치의 가시 개수를 사용할지 저장
	SPtr<CResStructuredBuffer> m_pDrawBatchInputBuffer{};

	// 첫 번째 컴퓨트 단계가 배치별 가시 인스턴스 수와 데이터를 기록한다
	SPtr<CResStructuredBuffer> m_pBatchVisibleCountBuffer{};
	SPtr<CResStructuredBuffer> m_pVisibleInstanceBuffer{};

	// 구조화 버퍼의 컬링 결과를 실제 인스턴스 버텍스 스트림으로 제공한다
	ComPtr<ID3D11Buffer> m_pVisibleInstanceVertexBuffer{};
	// 두 번째 컴퓨트 단계가 InstanceCount를 채우는 간접 드로우 인자 버퍼
	ComPtr<ID3D11Buffer> m_pIndirectArgsBuffer{};
	ComPtr<ID3D11UnorderedAccessView> m_pIndirectArgsUAV{};

	// 각각 컬링 단계와 간접 인자 생성 단계에 전달하는 상수 버퍼
	ComPtr<ID3D11Buffer> m_pCullConstantBuffer{};
	ComPtr<ID3D11Buffer> m_pIndirectArgsConstantBuffer{};

public:
	// GPU 버퍼는 첫 작업 시 필요한 크기로 지연 생성된다
	static UPtr<CMapMeshGpuCuller> Create();
};

NS_END
