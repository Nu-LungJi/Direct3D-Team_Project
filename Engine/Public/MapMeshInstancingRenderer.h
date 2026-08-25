#pragma once
#include "Engine_Base.h"
#include "IRenderable.h"
#include "MapMeshInstanceBatchCollector.h"
#include "MapMeshTextureCache.h"

NS_BEGIN(Engine)

class CMapMeshGpuCuller;
class CResCBuffer;
class CResPixelShader;
class CResSamplerState;
class CResStaticModel;
class CResVertexShader;

// 맵 오브젝트가 제출한 인스턴스를 프레임 단위로 병합하고,
// GPU 컬링과 간접 드로우 명령 생성을 거쳐 실제 렌더링까지 담당
class ENGINE_DLL CMapMeshInstancingRenderer final : public CEngineBase, public IRenderable
{
public:
	CMapMeshInstancingRenderer(const CMapMeshInstancingRenderer&) = delete;
	CMapMeshInstancingRenderer& operator=(const CMapMeshInstancingRenderer& rhs) = delete;

private:
	CMapMeshInstancingRenderer();
	~CMapMeshInstancingRenderer() override;

private:
	HRESULT Initialize();

public:
	// 인스턴싱이 활성화되어 있으면 자신을 맵 메시 렌더 그룹에 등록
	void Update();
	// 프레임 통계를 확정하고 이번 프레임에 수집·생성한 임시 데이터를 비운다
	void FrameEnd();
	// 모든 모델의 메시별 텍스처 캐시를 제거
	void ClearTextureCache();
	// 스트리밍 등으로 해제되는 특정 모델의 텍스처 캐시만 제거
	void EraseTextureCache(const SPtr<CResStaticModel>& model);

public:
	// 인스턴싱 상태와 직전 프레임 통계를 디버그 UI에 제공
	_bool IsInstancingEnabled() const { return m_bInstancingEnabled; }
	// 인스턴싱 활성 상태를 변경하고 기존 프레임 배치를 정리
	void SetInstancingEnabled(_bool enabled);
	const INSTANCING_STATS& GetInstancingStats() const { return m_PreviousFrameStats; }
	_bool IsDebugBoundsEnabled() const { return m_bDebugBoundsEnabled; }
	void SetDebugBoundsEnabled(_bool enabled) { m_bDebugBoundsEnabled = enabled; }

private:
	// 현재 수집 결과를 통계에 반영한 뒤 다음 프레임을 위해 배치를 비운다
	void FinalizeFrameBatches();
	// PrepareDrawPacket에서 만든 프레임 한정 CPU 데이터를 비운다
	void ClearFrameDrawData();
	// 렌더러가 소유한 수집 데이터, 캐시, GPU 컬링 처리기를 모두 해제
	void ReleaseInstancingResources();

private:
	// 하나의 메시를 한 번 간접 드로우하는 데 필요한 CPU 측 정보
	struct DRAW_ITEM
	{
		SPtr<CResStaticModel> model{};
		// 사용할 버텍스 셰이더 분기를 결정
		EMapMeshRenderFeature renderFeature{};
		// 모델 텍스처 캐시를 가리키며 캐시가 지워지기 전까지만 유효
		const CMapMeshTextureCache::MODEL_TEXTURE_SETS* textureSets = nullptr;
		// 이 Draw 직전에 머티리얼 상수 버퍼에 기록할 값
		MATERIAL_DESC materialDesc{};
		// 모델 안에서 렌더링할 메시의 인덱스
		uint32_t meshIndex = 0;
		// 가시 인스턴스 버퍼에서 이 배치가 시작되는 인스턴스 단위 오프셋
		uint32_t instanceOffset = 0;
	};

	// Deferred Context 워커가 Draw 명령을 기록하는 동안 필요한 리소스와
	// Immediate Context에서 캡처한 파이프라인 상태를 한 묶음으로 보관
	struct DRAW_PACKET
	{
		// 현재 맵 메시 패스가 사용하는 렌더 타깃 개수
		static constexpr uint32_t RENDER_TARGET_COUNT = 4;

		// 셰이더 및 상태 리소스
		SPtr<CResVertexShader> vertexStaticShader{};
		SPtr<CResVertexShader> vertexFoliageShader{};
		SPtr<CResPixelShader> pixelShader{};
		SPtr<CResSamplerState> sampler{};
		SPtr<CResCBuffer> materialConstantBuffer{};

		// GPU 컬링 결과와 간접 드로우 인자를 담은 GPU 버퍼
		ComPtr<ID3D11Buffer> visibleInstanceBuffer{};
		ComPtr<ID3D11Buffer> indirectArgsBuffer{};

		// 워커의 Deferred Context에 복원할 현재 렌더 패스 상태
		ComPtr<ID3D11Buffer> perPassConstantBuffer{};
		std::array<ComPtr<ID3D11RenderTargetView>, RENDER_TARGET_COUNT> renderTargets{};
		ComPtr<ID3D11DepthStencilView> depthStencilView{};
		ComPtr<ID3D11DepthStencilState> depthStencilState{};
		ComPtr<ID3D11RasterizerState> rasterizerState{};
		ComPtr<ID3D11BlendState> blendState{};
		ComPtr<ID3D11ShaderResourceView> noiseShaderResourceView{};
		D3D11_VIEWPORT viewport{};
		std::array<_float, 4> blendFactor{};
		uint32_t stencilRef = 0;
		uint32_t sampleMask = 0xffffffff;

		// 모든 리소스 준비와 GPU 컬링이 성공해 Draw 기록이 가능한지 나타낸다
		_bool isReady = false;
	};

	// 수집된 배치로부터 이번 프레임의 완성된 Draw 패킷을 만든다
	HRESULT PrepareDrawPacket(ID3D11DeviceContext* context, const RENDER_CTX& renderContext, DRAW_PACKET& outPacket);
	// Draw에 공통으로 필요한 셰이더, 샘플러, 상수 버퍼를 조회
	HRESULT ResolveDrawResources(DRAW_PACKET& outPacket) const;
	// 배치 전체 크기를 미리 계산해 프레임 벡터의 재할당을 줄인다
	void ReserveFrameDrawData();
	// 모델, 렌더 기능 단위 배치 하나를 GPU 컬링 입력과 메시별 Draw 정보로 펼친다
	HRESULT AppendInstanceBatch(
		const CMapMeshInstanceBatchCollector::MODEL_RENDER_KEY& key,
		const MAPMESH_INSTANCE_BATCH& batch,
		uint32_t& batchIndex);
	// 모든 배치를 병합하고 렌더 기능별 실행 순서까지 구성한다
	HRESULT BuildFrameDrawData(uint32_t& outBatchCount);
	// GPU 컬링을 실행해 가시 인스턴스 버퍼와 간접 드로우 인자 버퍼를 만든다
	HRESULT RunGpuCulling(
		ID3D11DeviceContext* context,
		const RENDER_CTX& renderContext,
		uint32_t batchCount,
		DRAW_PACKET& outPacket);
	// Immediate Context의 현재 렌더 타깃과 파이프라인 상태를 패킷에 보관한다.
	HRESULT CapturePipelineState(ID3D11DeviceContext* context, DRAW_PACKET& outPacket) const;
	// 지정된 Draw 명령 구간을 하나의 Deferred Context에 기록한다
	HRESULT RecordDrawCommands(
		ID3D11DeviceContext* context,
		const DRAW_PACKET& packet,
		uint32_t commandBegin,
		uint32_t commandEnd,
		uint32_t& outDrawCalls);
	// 메시별 머티리얼 값을 상수 버퍼에 기록하고 픽셀 셰이더에 바인딩한다
	static HRESULT BindMapMeshMaterial(
		ID3D11DeviceContext* context,
		const SPtr<CResCBuffer>& materialConstantBuffer,
		const MATERIAL_DESC& materialDesc);

public:
	// MapMeshObject 하나의 인스턴스 정보와 컬링 정보를 현재 프레임 배치에 제출
	HRESULT PushMapObjectInstance(const SPtr<CResStaticModel>& model, EMapMeshRenderFeature renderFeature, const MAPMESH_INSTANCE_DATA& instanceData, MAPMESH_OCCLUSION_DATA& occlusionData);
	// Draw 패킷을 만들고 워커별 명령 목록을 기록한 뒤 GPU에 제출
	HRESULT Render(ID3D11DeviceContext* context, const RENDER_CTX& renderContext) override;
	// 이 렌더러가 기본 렌더 패스에서만 실행됨을 알린다
	bool HasRenderPass(RENDERPASS renderPass) const override;

private:
	// LateUpdate에서 제출되는 인스턴스를 모델·렌더 기능별로 모은다
	CMapMeshInstanceBatchCollector m_InstanceBatchCollector;
	// 모델의 메시별 텍스처 조회 결과를 프레임 사이에 재사용
	CMapMeshTextureCache m_TextureCache;
	// 프러스텀,오클루전 컬링과 간접 드로우 버퍼 생성을 담당
	UPtr<CMapMeshGpuCuller> m_pGpuCuller;

	// 매 프레임 GPU 컬링 입력으로 병합되는 임시 데이터다
	// 세 배열은 같은 인스턴스 인덱스를 공유함
	std::vector<MAPMESH_INSTANCE_DATA> m_FrameInstances;
	std::vector<MAPMESH_OCCLUSION_DATA> m_FrameOcclusionData;
	std::vector<MAPMESH_CULL_META> m_FrameCullMetadata;

	// 각 Draw가 참조하는 GPU 컬링 배치 인덱스
	std::vector<uint32_t> m_BatchIndexByDraw;
	// GPU 컬링 처리기가 InstanceCount와 시작 위치를 채울 간접 드로우 인자 원본
	std::vector<D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS> m_IndirectDrawArguments;
	// Draw 인덱스로 접근하는 메시별 렌더 정보
	std::vector<DRAW_ITEM> m_DrawItems;
	// 실제 워커에 분배할 Draw 인덱스의 최종 실행 순서
	std::vector<uint32_t> m_DrawCommandIndices;

	// 렌더러 동작과 디버그 표시를 제어하는 상태
	_bool m_bInstancingEnabled = true;
	_bool m_bDebugBoundsEnabled = false;
	// 현재 누적 중인 통계와 디버그 UI가 읽는 직전 완료 프레임 통계
	INSTANCING_STATS m_CurrentFrameStats{ true };
	INSTANCING_STATS m_PreviousFrameStats{ true };

	// 렌더 기능 변경을 줄이기 위해 Draw 인덱스를 Static/Foliage 순서로 묶는다
	static constexpr size_t RENDER_FEATURE_COUNT = 2;
	std::array<std::vector<uint32_t>, RENDER_FEATURE_COUNT> m_DrawIndicesByFeature;

public:
	static UPtr<CMapMeshInstancingRenderer> Create();

public:
	void Free() override;
};

NS_END
