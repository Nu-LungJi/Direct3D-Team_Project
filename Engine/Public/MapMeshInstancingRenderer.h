#pragma once
#include "Engine_Base.h"
#include "IRenderable.h"
#include "MapChunk.h"
#include "MapMeshInstanceBatchCollector.h"
#include "MapMeshTextureCache.h"

NS_BEGIN(Engine)

class CMapMeshGpuCuller;
class CResCBuffer;
class CResPixelShader;
class CResSamplerState;
class CResStaticModel;
class CResVertexShader;

// 로드된 청크의 정적 인스턴스를 GPU 입력 버퍼에 상주
// 평상시에는 상주 입력을 재사용해 GPU 컬링과 간접 드로우만 수행
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
	HRESULT RenderShadow(ID3D11DeviceContext* context, const RENDER_CTX& renderContext, LIGHT_TYPE lightType);
	// Draw 패킷을 만들고 워커별 명령 목록을 기록한 뒤 GPU에 제출
	HRESULT Render(ID3D11DeviceContext* context, const RENDER_CTX& renderContext) override;
	// 이 렌더러가 기본 렌더 패스에서만 실행됨을 알린다
	bool HasRenderPass(RENDERPASS renderPass) const override;

public:
	// 인스턴싱이 활성화되어 있으면 자신을 맵 메시 렌더 그룹에 등록
	void Update();
	// 프레임 통계를 확정한다. 상주 인스턴스와 Draw 데이터는 유지한다.
	void FrameEnd();
	// 모든 모델의 메시별 텍스처 캐시를 제거
	void ClearTextureCache();
	// 스트리밍 등으로 해제되는 특정 모델의 텍스처 캐시만 제거
	void EraseTextureCache(const SPtr<CResStaticModel>& model);
	// 청크의 현재 오브젝트를 정적 인스턴스 데이터로 변환해 상주 목록에 등록한다.
	HRESULT RegisterResidentChunk(const MAPCHUNK_COORD& coord, const std::vector<CHandle>& objectHandles);
	// 언로드되는 청크의 상주 데이터를 제거하고 다음 렌더 시 GPU 입력 재구성을 예약한다.
	void UnregisterResidentChunk(const MAPCHUNK_COORD& coord);
	// 맵 교체 시 모든 청크의 상주 데이터와 Draw 데이터를 비운다.
	void ClearResidentChunks();

public:
	// 인스턴싱 상태와 직전 프레임 통계를 디버그 UI에 제공
	_bool IsInstancingEnabled() const { return m_bInstancingEnabled; }
	// 인스턴싱 활성 상태를 변경한다. 맵 메시는 상주 인스턴싱 사용을 전제로 한다.
	void SetInstancingEnabled(_bool enabled);
	const INSTANCING_STATS& GetInstancingStats() const { return m_PreviousFrameStats; }
	_bool IsDebugBoundsEnabled() const { return m_bDebugBoundsEnabled; }
	void SetDebugBoundsEnabled(_bool enabled) { m_bDebugBoundsEnabled = enabled; }

private:
	// 현재 상주 배치와 이번 프레임 Draw 통계를 확정한다.
	void FinalizeFrameBatches();
	// 상주 목록이 변경됐을 때 기존 모델별 배치와 Draw 데이터를 다시 만든다.
	HRESULT RebuildResidentDrawData();
	// 청크 변경 시 다시 만들 CPU 상주 Draw 데이터를 비운다.
	void ClearResidentDrawData();
	// 렌더러가 소유한 수집 데이터, 캐시, GPU 컬링 처리기를 모두 해제
	void ReleaseInstancingResources();

	void MarkResidentSceneDirty();

	void InvalidateCommandListCache(); // 캐싱해놓은 커맨드리스트 캐시 무효화

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

	// MapMeshObject에서 한 번 추출해 청크가 언로드될 때까지 재사용하는 렌더 데이터다.
	struct RESIDENT_INSTANCE
	{
		SPtr<CResStaticModel> model{};
		EMapMeshRenderFeature renderFeature{};
		MAPMESH_INSTANCE_DATA instanceData{};
		MAPMESH_OCCLUSION_DATA occlusionData{};
	};

	struct SHADOW_DRAW_PACKET
	{
		SPtr<CResVertexShader> vertexStaticShader;
		SPtr<CResVertexShader> vertexFoliageShader;
		SPtr<CResPixelShader> pixelShader;

		ComPtr<ID3D11Buffer> visibleInstanceBuffer;
		ComPtr<ID3D11Buffer> indirectArgsBuffer;

		ComPtr<ID3D11DepthStencilView> depthStencilView;
		ComPtr<ID3D11DepthStencilState> depthStencilState;
		ComPtr<ID3D11RasterizerState> rasterizerState;
		D3D11_VIEWPORT viewPort{};
		uint32_t stencilRef = 0;

		// MapMesh Shadow VS/PS에서 참조하는 상수버퍼
		ComPtr<ID3D11Buffer> lightConstantBuffer;
		ComPtr<ID3D11Buffer> shadowConstantBuffer;

		_bool isReady = false;
	};

	struct SHADOW_COMMANDLIST_CACHE_ENTRY
	{
		LIGHT_TYPE lightType{};
		ComPtr<ID3D11DepthStencilView> depthStencilView;
		ComPtr<ID3D11CommandList> commandList;
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

	HRESULT PrepareShadowDrawPacket(ID3D11DeviceContext* context, const RENDER_CTX& shadowContext, LIGHT_TYPE lightType, SHADOW_DRAW_PACKET& outPacket);
	// 수집된 배치로부터 이번 프레임의 완성된 Draw 패킷을 만든다
	HRESULT PrepareDrawPacket(ID3D11DeviceContext* context, const RENDER_CTX& renderContext, DRAW_PACKET& outPacket);

	// 커맨드리스트 캐싱 재구성하는 함수
	HRESULT RebuildCachedShadowCommandList(const SHADOW_DRAW_PACKET& shadowDrawPacket, LIGHT_TYPE lightType);
	HRESULT RebuildCachedCommandLists(const DRAW_PACKET& packet);

	HRESULT ResolveShadowDrawResources(LIGHT_TYPE lightType, SHADOW_DRAW_PACKET& outPacket) const;
	// Draw에 공통으로 필요한 셰이더, 샘플러, 상수 버퍼를 조회
	HRESULT ResolveDrawResources(DRAW_PACKET& outPacket) const;
	// 배치 전체 크기를 미리 계산해 상주 벡터의 재할당을 줄인다.
	void ReserveResidentDrawData();
	// 모델, 렌더 기능 단위 배치 하나를 GPU 컬링 입력과 메시별 Draw 정보로 펼친다
	HRESULT AppendInstanceBatch(const CMapMeshInstanceBatchCollector::MODEL_RENDER_KEY& key, const MAPMESH_INSTANCE_BATCH& batch, uint32_t& batchIndex);
	// 모든 배치를 병합하고 렌더 기능별 실행 순서까지 구성한다
	HRESULT BuildResidentDrawData(uint32_t& outBatchCount);

	HRESULT RunShadowGpuCulling(ID3D11DeviceContext* context, const RENDER_CTX& shadowContext, LIGHT_TYPE lightType, _bool uploadResidentData, SHADOW_DRAW_PACKET& outPacket);
	// GPU 컬링을 실행해 가시 인스턴스 버퍼와 간접 드로우 인자 버퍼를 만든다
	HRESULT RunGpuCulling(ID3D11DeviceContext* context, const RENDER_CTX& renderContext, uint32_t batchCount, _bool uploadResidentData, DRAW_PACKET& outPacket);
	
	HRESULT CaptureShadowPipelineState(ID3D11DeviceContext* context, SHADOW_DRAW_PACKET& outPacket) const;
	// Immediate Context의 현재 렌더 타깃과 파이프라인 상태를 패킷에 보관한다.
	HRESULT CapturePipelineState(ID3D11DeviceContext* context, DRAW_PACKET& outPacket) const;

	HRESULT RecordShadowDrawCommands(ID3D11DeviceContext* context, const SHADOW_DRAW_PACKET& shadowDrawPacket);
	// 지정된 Draw 명령 구간을 하나의 Deferred Context에 기록한다
	HRESULT RecordDrawCommands(ID3D11DeviceContext* context,const DRAW_PACKET& packet, uint32_t commandBegin, uint32_t commandEnd, uint32_t& outDrawCalls);
	// 메시별 머티리얼 값을 상수 버퍼에 기록하고 픽셀 셰이더에 바인딩한다
	static HRESULT BindMapMeshMaterial(ID3D11DeviceContext* context, const SPtr<CResCBuffer>& materialConstantBuffer, const MATERIAL_DESC& materialDesc);

private:
	// 현재 로드된 청크의 정적 인스턴스를 모델·렌더 기능별로 모은다.
	CMapMeshInstanceBatchCollector m_InstanceBatchCollector;
	// 청크 변경이 없는 프레임에는 이 데이터를 다시 만들거나 GPU에 보내지 않는다.
	std::unordered_map<MAPCHUNK_COORD, std::vector<RESIDENT_INSTANCE>, tagMapChunkCoordHash> m_ResidentChunks;
	// 모델의 메시별 텍스처 조회 결과를 프레임 사이에 재사용
	CMapMeshTextureCache m_TextureCache;

	UPtr<CMapMeshGpuCuller> m_pGpuCuller; // 프러스텀,오클루전 컬링과 간접 드로우 버퍼 생성을 담당
	UPtr<CMapMeshGpuCuller> m_pShadowGpuCuller; // 광원 프러스텀만 사용해 그림자 가시 인스턴스를 생성하는 GPU 컬러

	_bool m_IsMainGpuDataDirty = true;
	_bool m_IsShadowGpuDataDirty = true;

	// 청크 변경 시에만 다시 병합하고 GPU에 업로드하는 상주 입력 데이터다.
	// 세 배열은 같은 인스턴스 인덱스를 공유함
	std::vector<MAPMESH_INSTANCE_DATA> m_ResidentInstances;
	std::vector<MAPMESH_OCCLUSION_DATA> m_ResidentOcclusionData;
	std::vector<MAPMESH_CULL_META> m_ResidentCullMetadata;
	uint32_t m_ResidentBatchCount = 0;
	_bool m_IsResidentDrawDataDirty = true;

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

	// Shadow 커맨드리스트 캐싱
	std::unordered_map<ID3D11DepthStencilView*, SHADOW_COMMANDLIST_CACHE_ENTRY> m_CachedShadowCommandLists;
	// 청크 변화 없을 때 기록해놨던 드로우명령 캐싱해놓을 곳
	std::vector<ComPtr<ID3D11CommandList>> m_CachedCommandLists;
	_bool m_IsCommandListCacheDirty = true;

public:
	static UPtr<CMapMeshInstancingRenderer> Create();

public:
	void Free() override;
};

NS_END
