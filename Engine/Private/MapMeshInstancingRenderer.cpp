#include "pch.h"
#include "MapMeshInstancingRenderer.h"
#include "MapMeshGpuCuller.h"
#include "MapMeshObject.h"
#include "ComStaticModelInstance.h"

NS_USING(Engine)

namespace
{
	// 워커 하나가 기록한 명령 목록과 실행 결과를 메인 렌더 스레드로 전달한다.
	struct DRAW_COMMAND_LIST_RESULT
	{
		// 명령 기록 또는 FinishCommandList의 성공 여부다.
		HRESULT result = E_FAIL;
		// Immediate Context에서 실행할 완성된 명령 목록이다.
		ComPtr<ID3D11CommandList> commandList{};
		// 통계에 합산할 이 명령 목록의 Draw 호출 수다.
		uint32_t drawCalls = 0;
	};
}

HRESULT CMapMeshInstancingRenderer::BindMapMeshMaterial(ID3D11DeviceContext* context, const SPtr<CResCBuffer>& materialConstantBuffer, const MATERIAL_DESC& materialDesc)
{
	if (context == nullptr || materialConstantBuffer == nullptr)
	{
		return E_FAIL;
	}

	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (FAILED(context->Map(materialConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
	{
		return E_FAIL;
	}

	CB_MATERIAL material{};
	material.NormalIntensity = materialDesc.m_fNormalIntensity;
	material.MetallicIntensity = materialDesc.m_fMetallicIntensity;
	material.RoughnessIntensity = materialDesc.m_fRoughnessIntensity;
	material.AmbientIntensity = materialDesc.m_fAmbientIntensity;

	material.EmissiveColor = materialDesc.m_fEmissiveColor;
	material.EmissiveIntensity = materialDesc.m_fEmissiveIntensity;
	material.ObjectAlpha = materialDesc.m_fObjectAlpha;
	memcpy(mapped.pData, &material, sizeof(CB_MATERIAL));
	context->Unmap(materialConstantBuffer->GetCBuffer().Get(), 0);

	context->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::MATERIAL), 1, materialConstantBuffer->GetCBuffer().GetAddressOf());

	return S_OK;
}

CMapMeshInstancingRenderer::CMapMeshInstancingRenderer()
{
}

CMapMeshInstancingRenderer::~CMapMeshInstancingRenderer()
{
	ReleaseInstancingResources();
}

HRESULT CMapMeshInstancingRenderer::Initialize()
{
	m_pGpuCuller = CMapMeshGpuCuller::Create();

	if (m_pGpuCuller == nullptr)
		return E_FAIL;

	return S_OK;
}

void CMapMeshInstancingRenderer::Update()
{
	// 인스턴싱이 활성화된 프레임에만 렌더 큐에 등록한다.
	if (m_bInstancingEnabled) 
		CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND_MAPMESH, this);

	// 디버그 표시가 켜진 경우에만 상주 바운드를 순회
	if (!m_bDebugBoundsEnabled)
		return;

	auto* debugLine = CGameInstance::Get().GetDbgLineRender();
	if (debugLine == nullptr)
		return;

	debugLine->SetColor({ 1.f, 1.f, 0.f, 1.f });
	for (const auto& [coord, residentInstances] : m_ResidentChunks)
	{
		for (const auto& resident : residentInstances)
		{
			const _float3& center = resident.occlusionData.worldCenter;
			debugLine->AddBox(resident.occlusionData.worldExtents, XMMatrixTranslation(center.x, center.y, center.z));
		}
	}

	debugLine->SetColor();
}

void CMapMeshInstancingRenderer::FrameEnd()
{
	FinalizeFrameBatches();
}

void CMapMeshInstancingRenderer::ClearTextureCache()
{
	m_TextureCache.ClearAll();

	m_IsResidentSceneDirty = true;
	InvalidateCommandListCache();
}

void CMapMeshInstancingRenderer::EraseTextureCache(const SPtr<CResStaticModel>& model)
{
	m_TextureCache.EraseModel(model);

	m_IsResidentSceneDirty = true;
	InvalidateCommandListCache();
}

HRESULT CMapMeshInstancingRenderer::RegisterResidentChunk(const MAPCHUNK_COORD& coord, const std::vector<CHandle>& objectHandles)
{
	std::vector<RESIDENT_INSTANCE> residentInstances;
	residentInstances.reserve(objectHandles.size());

	for (const CHandle& objectHandle : objectHandles)
	{
		auto* mapObject = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(objectHandle);
		if (mapObject == nullptr || mapObject->GetStaticModelInstance() == nullptr)
			continue;

		auto model = mapObject->GetStaticModelInstance()->GetModel();
		if (model == nullptr)
			continue;

		// LateUpdate가 없어도 생성 직후의 Transform 행렬이 유효하도록 여기서 한 번 확정
		mapObject->GetTransform().Update();

		RESIDENT_INSTANCE resident{};
		resident.model = model;
		XMStoreFloat4x4(&resident.instanceData.world, mapObject->GetTransform().GetLoadedCombinedWorldMatrix());

		const WIND_DESC& windDesc = mapObject->GetWindDesc();
		resident.instanceData.windParams = {
			windDesc.strength,
			windDesc.speed,
			windDesc.frequency,
			windDesc.bendExponent
		};

		if (model->HasLocalBounds())
		{
			const BoundingBox& localBounds = model->GetLocalBounds();
			const _float modelMinY = localBounds.Center.y - localBounds.Extents.y;
			const _float modelHeight = localBounds.Extents.y * 2.f;
			const _float heightStart = std::clamp(windDesc.heightStart, 0.f, 1.f);
			const _float heightEnd = std::clamp(windDesc.heightEnd, heightStart, 1.f);
			const _float influenceHeight = modelHeight * (heightEnd - heightStart);
			if (influenceHeight > 0.0001f)
			{
				resident.instanceData.windHeightParams = { modelMinY + modelHeight * heightStart, 1.f / influenceHeight };
			}
		}

		resident.instanceData.windType = static_cast<uint32_t>(windDesc.type);
		resident.renderFeature = windDesc.type == EWindType::None ? EMapMeshRenderFeature::Static : EMapMeshRenderFeature::Foliage;

		BoundingBox worldBounds{};
		if (!mapObject->GetOcclusionBounds(worldBounds))
			continue;

		resident.occlusionData.worldCenter = worldBounds.Center;
		resident.occlusionData.worldExtents = worldBounds.Extents;
		residentInstances.push_back(std::move(resident));
	}

	m_ResidentChunks[coord] = std::move(residentInstances);
	m_IsResidentSceneDirty = true;
	InvalidateCommandListCache(); // 커맨드리스트 캐싱 무효화

	return S_OK;
}

void CMapMeshInstancingRenderer::UnregisterResidentChunk(const MAPCHUNK_COORD& coord)
{
	if (m_ResidentChunks.erase(coord) > 0)
	{
		m_IsResidentSceneDirty = true;
		InvalidateCommandListCache(); // 커맨드리스트 캐싱 무효화
	}
}

void CMapMeshInstancingRenderer::ClearResidentChunks()
{
	m_ResidentChunks.clear();
	m_InstanceBatchCollector.ClearBatches();
	ClearResidentDrawData();
	m_ResidentBatchCount = 0;
	m_IsResidentSceneDirty = false;
	InvalidateCommandListCache(); // 커맨드리스트 캐싱 무효화
}

HRESULT CMapMeshInstancingRenderer::Render(ID3D11DeviceContext* context, const RENDER_CTX& renderContext)
{
	ZoneScopedN("MapMeshInstancingRender");

	DRAW_PACKET packet{};
	// 수집한 인스턴스를 실제 Draw에 필요한 한 프레임 데이터로 변환
	if (FAILED(PrepareDrawPacket(context, renderContext, packet)))
		return E_FAIL;
	if (!packet.isReady)
		return S_OK;

	if (m_IsCommandListCacheDirty)
	{
		if (FAILED(RebuildCachedCommandLists(packet)))
			return E_FAIL;
	}

	// 완성된 명령 목록을 Immediate Context에서 순서대로 실행한다.
	{
		ZoneScopedN("MapMeshExecuteCommandLists");
		for (const auto& commandList : m_CachedCommandLists)
		{
			//m_CurrentFrameStats.iDrawCalls += commandList.drawCalls;
			context->ExecuteCommandList(commandList.Get(), TRUE);
		}
	}
	return S_OK;
}

HRESULT CMapMeshInstancingRenderer::PrepareDrawPacket(ID3D11DeviceContext* context, const RENDER_CTX& renderContext, DRAW_PACKET& outPacket)
{
	ZoneScopedN("MapMeshPrepareDrawPacket");
	outPacket = {};

	if (context == nullptr)
		return E_INVALIDARG;

	const _bool uploadResidentData = m_IsResidentSceneDirty;
	if (uploadResidentData && FAILED(RebuildResidentDrawData()))
		return E_FAIL;

	if (m_ResidentInstances.empty() || m_DrawItems.empty())
	{
		m_IsResidentSceneDirty = false;
		return S_OK;
	}

	if (FAILED(ResolveDrawResources(outPacket)))
		return E_FAIL;

	if (FAILED(RunGpuCulling(
		context, renderContext, m_ResidentBatchCount,
		uploadResidentData, outPacket)))
		return E_FAIL;

	m_IsResidentSceneDirty = false;
	if (FAILED(CapturePipelineState(context, outPacket)))
		return E_FAIL;

	outPacket.isReady = true;

	return S_OK;
}

HRESULT CMapMeshInstancingRenderer::RebuildCachedCommandLists(const DRAW_PACKET& packet)
{
	ZoneScopedN("MapMeshRebuildCachedCommandLists");

	const uint32_t commandCount = static_cast<uint32_t>(m_DrawCommandIndices.size());
	const uint32_t availableWorkers = CGameInstance::Get().GetRenderWorkerCount();
	// 작은 작업의 과도한 분할을 막기 위해 실제 명령 수와 최대 6개 워커로 제한
	const uint32_t workerCount = std::min({ 6u, availableWorkers, commandCount });
	if (workerCount == 0)
	{
		m_CachedCommandLists.clear();
		// 다음 프레임에 워커가 생기면 다시 시도하도록
		// Dirty는 true 상태로 유지
		return S_OK;
	}

	std::vector<ComPtr<ID3D11CommandList>> rebuiltCommandLists;
	rebuiltCommandLists.reserve(workerCount);

	std::vector<std::future<DRAW_COMMAND_LIST_RESULT>> commandListFutures{};
	commandListFutures.reserve(workerCount);

	const uint32_t commandsPerWorker = commandCount / workerCount;
	// 나누어떨어지지 않는 명령은 앞쪽 워커부터 하나씩 추가
	const uint32_t remainder = commandCount % workerCount;
	uint32_t commandBegin = 0;

	// Draw 명령을 여러 Deferred Context에 균등하게 분배
	for (uint32_t workerIndex = 0; workerIndex < workerCount; ++workerIndex)
	{
		const uint32_t commandEnd = commandBegin + commandsPerWorker + (workerIndex < remainder ? 1u : 0u);
		const std::string taskName = "MapMeshRecordDrawCommands_" + std::to_string(workerIndex);

		commandListFutures.push_back(
			CGameInstance::Get().RenderWorkerEnqueueWithFuture(
				taskName,
				[this, packet, commandBegin, commandEnd](ID3D11DeviceContext* deferredContext)
				{
					DRAW_COMMAND_LIST_RESULT result{};
					result.result = RecordDrawCommands(
						deferredContext, packet,
						commandBegin, commandEnd, result.drawCalls);

					if (FAILED(result.result))
						return result;

					result.result = deferredContext->FinishCommandList(FALSE, result.commandList.GetAddressOf());

					return result;
				}));

		commandBegin = commandEnd;
	}

	std::vector<DRAW_COMMAND_LIST_RESULT> commandListResults(workerCount);
	try
	{
		ZoneScopedN("MapMeshWaitForCommandLists");
		for (uint32_t i = 0; i < workerCount; ++i)
		{
			commandListResults[i] = commandListFutures[i].get();
			if (FAILED(commandListResults[i].result) || commandListResults[i].commandList == nullptr)
			{
				return E_FAIL;
			}

			rebuiltCommandLists.push_back(commandListResults[i].commandList);
		}
	}
	catch (...)
	{
		return E_FAIL;
	}

	// 캐싱 적용
	m_CachedCommandLists = std::move(rebuiltCommandLists);
	m_IsCommandListCacheDirty = false;

	return S_OK;
}

HRESULT CMapMeshInstancingRenderer::RebuildResidentDrawData()
{
	ZoneScopedN("MapMeshRebuildResidentDrawData");
	m_InstanceBatchCollector.ClearBatches();
	ClearResidentDrawData();

	for (auto& [coord, residentInstances] : m_ResidentChunks)
	{
		for (auto& resident : residentInstances)
		{
			if (FAILED(m_InstanceBatchCollector.AddInstance(
				resident.model,
				resident.renderFeature,
				resident.instanceData,
				resident.occlusionData)))
			{
				return E_FAIL;
			}
		}
	}

	m_ResidentBatchCount = 0;
	if (m_InstanceBatchCollector.IsEmpty())
		return S_OK;

	return BuildResidentDrawData(m_ResidentBatchCount);
}

HRESULT CMapMeshInstancingRenderer::ResolveDrawResources(DRAW_PACKET& outPacket) const
{
	ZoneScopedN("MapMeshResolveDrawResources");
	auto& gameInstance = CGameInstance::Get();

	outPacket.vertexStaticShader = gameInstance.GetResourceFirst<CResVertexShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim_Instanced");
	outPacket.vertexFoliageShader = gameInstance.GetResourceFirst<CResVertexShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim_Instanced_Foliage");
	outPacket.pixelShader = gameInstance.GetResourceFirst<CResPixelShader>(
		TAG_RES_GRP_PERMANENT_SHADER, TAG_RES_PERMANENT_NONBLENDSHADER);
	outPacket.sampler = gameInstance.GetResourceFirst<CResSamplerState>(
		TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
	outPacket.materialConstantBuffer = gameInstance.GetResourceFirst<CResCBuffer>(
		TAG_RES_GRP_PERMANENT_BUFFER, "CB_MATERIAL");

	if (!outPacket.vertexStaticShader || !outPacket.vertexFoliageShader ||
		!outPacket.pixelShader || !outPacket.sampler ||
		!outPacket.materialConstantBuffer)
	{
		return E_FAIL;
	}
	return S_OK;
}

void CMapMeshInstancingRenderer::ReserveResidentDrawData()
{
	// 실제 병합 전에 필요한 크기를 합산해 vector 확장과 복사를 최소화
	size_t instanceCapacity = 0;
	size_t drawCapacity = 0;
	for (const auto& [key, batch] : m_InstanceBatchCollector.GetBatches())
	{
		const auto& model = key.first;
		if (!model || batch.instances.empty())
			continue;

		instanceCapacity += batch.instances.size();
		drawCapacity += model->Get_NumMeshes();
	}

	m_ResidentInstances.reserve(instanceCapacity);
	m_ResidentOcclusionData.reserve(instanceCapacity);
	m_ResidentCullMetadata.reserve(instanceCapacity);
	m_BatchIndexByDraw.reserve(drawCapacity);
	m_IndirectDrawArguments.reserve(drawCapacity);
	m_DrawItems.reserve(drawCapacity);
	m_DrawCommandIndices.reserve(drawCapacity);
}

HRESULT CMapMeshInstancingRenderer::AppendInstanceBatch(
	const CMapMeshInstanceBatchCollector::MODEL_RENDER_KEY& key,
	const MAPMESH_INSTANCE_BATCH& batch,
	uint32_t& batchIndex)
{
	const auto& [model, renderFeature] = key;
	if (!model || batch.instances.empty())
		return S_OK;

	if (batch.occlusionData.size() != batch.instances.size())
		return E_FAIL;

	const auto* textureSets = m_TextureCache.GetOrCreateTextureSets(model);
	if (textureSets == nullptr)
		return E_FAIL;

	const MATERIAL_DESC materialDesc = model->GetMaterialDesc();
	// 이 배치가 병합된 전체 인스턴스 배열에서 시작하는 위치
	const uint32_t instanceOffset = static_cast<uint32_t>(m_ResidentInstances.size());
	m_ResidentInstances.insert(m_ResidentInstances.end(), batch.instances.begin(), batch.instances.end());
	m_ResidentOcclusionData.insert(m_ResidentOcclusionData.end(), batch.occlusionData.begin(), batch.occlusionData.end());
	m_ResidentCullMetadata.insert(m_ResidentCullMetadata.end(), batch.instances.size(), MAPMESH_CULL_META{ instanceOffset, batchIndex });

	// 렌더 기능별 Draw 버킷을 선택하는 배열 인덱스
	const size_t featureIndex = static_cast<size_t>(renderFeature);
	if (featureIndex >= RENDER_FEATURE_COUNT)
		return E_FAIL;

	for (uint32_t meshIndex = 0; meshIndex < model->Get_NumMeshes(); ++meshIndex)
	{
		const auto& mesh = model->GetMeshes()[meshIndex];
		if (mesh == nullptr)
			continue;

		const uint32_t drawIndex = static_cast<uint32_t>(m_DrawItems.size());
		m_BatchIndexByDraw.push_back(batchIndex);

		D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS args{};
		args.IndexCountPerInstance = static_cast<uint32_t>(mesh->GetNumIndices());
		m_IndirectDrawArguments.push_back(args);
		m_DrawItems.push_back({
			model, renderFeature, textureSets, materialDesc,
			meshIndex, instanceOffset });
		m_DrawIndicesByFeature[featureIndex].push_back(drawIndex);
	}

	++batchIndex;

	return S_OK;
}

HRESULT CMapMeshInstancingRenderer::BuildResidentDrawData(uint32_t& outBatchCount)
{
	ZoneScopedN("MapMeshBuildResidentDrawData");
	ReserveResidentDrawData();

	outBatchCount = 0;
	for (const auto& [key, batch] : m_InstanceBatchCollector.GetBatches())
	{
		if (FAILED(AppendInstanceBatch(key, batch, outBatchCount)))
			return E_FAIL;
	}

	for (const auto& drawIndices : m_DrawIndicesByFeature)
	{
		m_DrawCommandIndices.insert(m_DrawCommandIndices.end(), drawIndices.begin(), drawIndices.end());
	}

	return m_DrawCommandIndices.size() == m_DrawItems.size() ? S_OK : E_FAIL;
}

HRESULT CMapMeshInstancingRenderer::RunGpuCulling(
	ID3D11DeviceContext* context,
	const RENDER_CTX& renderContext,
	uint32_t batchCount,
	_bool uploadResidentData,
	DRAW_PACKET& outPacket)
{
	ZoneScopedN("MapMeshRunGpuCulling");
	if (m_pGpuCuller == nullptr)
	{
		m_pGpuCuller = CMapMeshGpuCuller::Create();
		if (m_pGpuCuller == nullptr)
			return E_FAIL;
	}

	auto& gameInstance = CGameInstance::Get();
	if (FAILED(m_pGpuCuller->BuildVisibleInstancesAndIndirectArgs(
		context, m_ResidentInstances, m_ResidentOcclusionData, m_ResidentCullMetadata, batchCount,
		m_BatchIndexByDraw, m_IndirectDrawArguments, uploadResidentData,
		gameInstance.GetPrevHizBuffer(), renderContext.matViewProj,
		gameInstance.GetClientScreenSize())))
	{
		return E_FAIL;
	}

	outPacket.visibleInstanceBuffer = m_pGpuCuller->GetVisibleInstanceBuffer();
	outPacket.indirectArgsBuffer = m_pGpuCuller->GetIndirectArgsBuffer();
	return outPacket.visibleInstanceBuffer && outPacket.indirectArgsBuffer ? S_OK : E_FAIL;
}

HRESULT CMapMeshInstancingRenderer::CapturePipelineState(
	ID3D11DeviceContext* context,
	DRAW_PACKET& outPacket) const
{
	ZoneScopedN("MapMeshCapturePipelineState");
	ID3D11RenderTargetView* renderTargets[DRAW_PACKET::RENDER_TARGET_COUNT]{};
	ID3D11DepthStencilView* depthStencilView = nullptr;
	context->OMGetRenderTargets(DRAW_PACKET::RENDER_TARGET_COUNT, renderTargets, &depthStencilView);
	for (uint32_t i = 0; i < DRAW_PACKET::RENDER_TARGET_COUNT; ++i)
		outPacket.renderTargets[i].Attach(renderTargets[i]);
	outPacket.depthStencilView.Attach(depthStencilView);

	ID3D11DepthStencilState* depthStencilState = nullptr;
	context->OMGetDepthStencilState(&depthStencilState, &outPacket.stencilRef);
	outPacket.depthStencilState.Attach(depthStencilState);

	ID3D11RasterizerState* rasterizerState = nullptr;
	context->RSGetState(&rasterizerState);
	outPacket.rasterizerState.Attach(rasterizerState);

	ID3D11BlendState* blendState = nullptr;
	context->OMGetBlendState(&blendState, outPacket.blendFactor.data(), &outPacket.sampleMask);
	outPacket.blendState.Attach(blendState);

	UINT viewportCount = 1;
	context->RSGetViewports(&viewportCount, &outPacket.viewport);
	if (viewportCount != 1)
		return E_FAIL;

	ID3D11Buffer* perPassConstantBuffer = nullptr;
	context->VSGetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_PASS), 1, &perPassConstantBuffer);
	outPacket.perPassConstantBuffer.Attach(perPassConstantBuffer);

	ID3D11ShaderResourceView* noiseShaderResourceView = nullptr;
	context->PSGetShaderResources(13, 1, &noiseShaderResourceView);
	outPacket.noiseShaderResourceView.Attach(noiseShaderResourceView);

	if (!outPacket.depthStencilView || !outPacket.depthStencilState ||
		!outPacket.perPassConstantBuffer || !outPacket.noiseShaderResourceView ||
		std::ranges::any_of(outPacket.renderTargets,
			[](const auto& renderTarget) { return renderTarget == nullptr; }))
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT CMapMeshInstancingRenderer::RecordDrawCommands(
	ID3D11DeviceContext* context,
	const DRAW_PACKET& packet,
	uint32_t commandBegin,
	uint32_t commandEnd,
	uint32_t& outDrawCalls)
{
	ZoneScopedN("MapMeshRecordDrawCommands");
	outDrawCalls = 0;
	if (context == nullptr || !packet.isReady ||
		!packet.vertexStaticShader || !packet.vertexFoliageShader ||
		!packet.pixelShader || !packet.sampler ||
		!packet.materialConstantBuffer ||
		!packet.visibleInstanceBuffer || !packet.indirectArgsBuffer ||
		!packet.depthStencilView || !packet.depthStencilState ||
		!packet.perPassConstantBuffer || !packet.noiseShaderResourceView ||
		commandBegin >= commandEnd || commandEnd > m_DrawCommandIndices.size())
	{
		return E_INVALIDARG;
	}

	ID3D11RenderTargetView* renderTargets[DRAW_PACKET::RENDER_TARGET_COUNT]{};
	for (uint32_t i = 0; i < DRAW_PACKET::RENDER_TARGET_COUNT; ++i)
	{
		renderTargets[i] = packet.renderTargets[i].Get();
	}

	context->OMSetRenderTargets(DRAW_PACKET::RENDER_TARGET_COUNT, renderTargets, packet.depthStencilView.Get());
	context->OMSetDepthStencilState(packet.depthStencilState.Get(), packet.stencilRef);
	context->OMSetBlendState(packet.blendState.Get(), packet.blendFactor.data(), packet.sampleMask);
	context->RSSetState(packet.rasterizerState.Get());
	context->RSSetViewports(1, &packet.viewport);

	ID3D11Buffer* perPassConstantBuffer = packet.perPassConstantBuffer.Get();
	context->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_PASS), 1, &perPassConstantBuffer);
	context->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_PASS), 1, &perPassConstantBuffer);

	ID3D11ShaderResourceView* noiseShaderResourceView = packet.noiseShaderResourceView.Get();
	// Shader_VtxMesh_Instanced.hlsl의 DefaultNoiseTexture register(t14)와 맞춘다.
	// t13은 CSMShadowTextures가 사용한다.
	context->PSSetShaderResources(14, 1, &noiseShaderResourceView);

	context->PSSetShader(packet.pixelShader->GetPixelShader().Get(), nullptr, 0);
	context->PSSetSamplers(0, 1, packet.sampler->GetSamplerState().GetAddressOf());

	ID3D11Buffer* materialConstantBuffer = packet.materialConstantBuffer->GetCBuffer().Get();
	context->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::MATERIAL), 1, &materialConstantBuffer);

	// 직전 Draw와 렌더 기능이 같으면 버텍스 셰이더 재바인딩을 생략한다.
	std::optional<EMapMeshRenderFeature> currentFeature{};
	for (uint32_t commandIndex = commandBegin; commandIndex < commandEnd; ++commandIndex)
	{
		const uint32_t drawIndex = m_DrawCommandIndices[commandIndex];
		if (drawIndex >= m_DrawItems.size())
			return E_FAIL;

		const auto& item = m_DrawItems[drawIndex];
		if (!currentFeature || *currentFeature != item.renderFeature)
		{
			const SPtr<CResVertexShader>* vertexShader = nullptr;
			switch (item.renderFeature)
			{
			case EMapMeshRenderFeature::Static:
				vertexShader = &packet.vertexStaticShader;
				break;
			case EMapMeshRenderFeature::Foliage:
				vertexShader = &packet.vertexFoliageShader;
				break;
			default:
				return E_FAIL;
			}

			context->IASetInputLayout((*vertexShader)->GetInputLayout().Get());
			context->VSSetShader((*vertexShader)->GetVertexShader().Get(), nullptr, 0);
			currentFeature = item.renderFeature;
		}

		const auto& mesh = item.model->GetMeshes()[item.meshIndex];
		ID3D11Buffer* vertexBuffers[] = { mesh->GetVertexBuffer().Get(), packet.visibleInstanceBuffer.Get() };
		uint32_t strides[] = { mesh->GetVertexStride(), static_cast<uint32_t>(sizeof(MAPMESH_INSTANCE_DATA)) };
		uint32_t offsets[] = { 0, item.instanceOffset * static_cast<uint32_t>(sizeof(MAPMESH_INSTANCE_DATA)) };

		context->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
		context->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		context->IASetPrimitiveTopology(mesh->GetPrimitiveType());
		if (FAILED(m_TextureCache.BindTextures(context, *item.textureSets, item.meshIndex)))
			return E_FAIL;

		// 모델의 머티리얼은 메시 Draw 직전에 갱신한다.
		if (FAILED(BindMapMeshMaterial(context, packet.materialConstantBuffer, item.materialDesc)))
			return E_FAIL;
		context->DrawIndexedInstancedIndirect(packet.indirectArgsBuffer.Get(),
			drawIndex * static_cast<uint32_t>(sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS)));
		++outDrawCalls;
	}

	return S_OK;
}

bool CMapMeshInstancingRenderer::HasRenderPass(RENDERPASS renderPass) const
{
	return renderPass == RENDERPASS::DEFAULT;
}

void CMapMeshInstancingRenderer::SetInstancingEnabled(_bool enabled)
{
	if (m_bInstancingEnabled == enabled)
	{
		return;
	}

	m_bInstancingEnabled = enabled;
	FinalizeFrameBatches();
}

void CMapMeshInstancingRenderer::FinalizeFrameBatches()
{
	// 상주 인스턴스 수는 청크가 바뀌지 않는 동안 동일하며 Draw 통계만 프레임마다 새로 누적한다.
	m_CurrentFrameStats.bEnabled = m_bInstancingEnabled;
	m_CurrentFrameStats.iInstances = m_InstanceBatchCollector.GetInstanceCount();
	m_CurrentFrameStats.iBatches = static_cast<uint32_t>(m_InstanceBatchCollector.GetBatchCount());
	m_PreviousFrameStats = m_CurrentFrameStats;
	m_CurrentFrameStats = {};
	m_CurrentFrameStats.bEnabled = m_bInstancingEnabled;

}

void CMapMeshInstancingRenderer::ClearResidentDrawData()
{
	m_ResidentInstances.clear();
	m_ResidentOcclusionData.clear();
	m_ResidentCullMetadata.clear();
	m_BatchIndexByDraw.clear();
	m_IndirectDrawArguments.clear();
	m_DrawItems.clear();
	m_DrawCommandIndices.clear();
	for (auto& drawIndices : m_DrawIndicesByFeature)
		drawIndices.clear();
}

void CMapMeshInstancingRenderer::ReleaseInstancingResources()
{
	InvalidateCommandListCache(); // 커맨드리스트 캐싱 무효화
	m_ResidentChunks.clear();
	m_InstanceBatchCollector.ClearBatches();
	ClearResidentDrawData();
	ClearTextureCache();
	m_pGpuCuller.reset();
}

void CMapMeshInstancingRenderer::InvalidateCommandListCache()
{
	m_CachedCommandLists.clear();
	m_IsCommandListCacheDirty = true;
}

UPtr<CMapMeshInstancingRenderer> CMapMeshInstancingRenderer::Create()
{
	auto pInstance = ToUPtr(new CMapMeshInstancingRenderer{});
	if (FAILED(pInstance->Initialize()))
	{
		return nullptr;
	}
	return pInstance;
}

void CMapMeshInstancingRenderer::Free()
{
	CEngineBase::Free();
}
