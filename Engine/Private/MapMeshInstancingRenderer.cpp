#include "pch.h"
#include "MapMeshInstancingRenderer.h"
#include "MapMeshGpuCuller.h"
#include "MapMeshObject.h"

NS_USING(Engine)

namespace
{
	struct MAPMESH_COMMAND_LIST_RESULT
	{
		HRESULT result = E_FAIL;
		ComPtr<ID3D11CommandList> commandList{};
		uint32_t drawCalls = 0;
	};

	SPtr<CResTexture2D> GetMapMeshTexture(const SPtr<CResStaticModel>& pModel, uint32_t meshIndex, AI_TEXTURE_TYPE materialType)
	{
		if (pModel == nullptr)
		{
			return nullptr;
		}

		auto& meshes = pModel->GetMeshes();
		if (meshIndex >= meshes.size() || meshes[meshIndex] == nullptr)
		{
			return nullptr;
		}

		auto& materials = pModel->GetMaterials();
		const uint32_t materialIndex = meshes[meshIndex]->Get_MaterialIndex();
		if (materialIndex >= materials.size() || materials[materialIndex] == nullptr)
		{
			return nullptr;
		}

		auto textures = materials[materialIndex]->GetTextures();
		if (textures[materialType].empty())
		{
			return nullptr;
		}

		return textures[materialType].front();
	}
}

const std::vector<CMapMeshInstancingRenderer::MAPMESH_TEXTURE_SET>*
CMapMeshInstancingRenderer::GetOrCreateMapMeshTextureCache(const SPtr<CResStaticModel>& pModel)
	{
		if (pModel == nullptr)
		{
			return nullptr;
		}

		if (const auto iter = m_MapMeshTextureCache.find(pModel); iter != m_MapMeshTextureCache.end())
		{
			return &iter->second;
		}

		MAPMESH_TEXTURE_SET defaultTextures{
			CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_DIFFUSE"),
			CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_NORMAL"),
			CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_SMRO"),
			CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_EMISSIVE")
		};
		if (std::ranges::any_of(defaultTextures, [](const auto& texture) { return texture == nullptr; }))
		{
			return nullptr;
		}

		std::vector<MAPMESH_TEXTURE_SET> textureSets(pModel->Get_NumMeshes(), defaultTextures);
		for (uint32_t meshIndex = 0; meshIndex < textureSets.size(); ++meshIndex)
		{
			auto& textures = textureSets[meshIndex];
			if (auto texture = GetMapMeshTexture(pModel, meshIndex, AI_TEXTURE_TYPE::aiTextureType_DIFFUSE))
				textures[0] = std::move(texture);
			if (auto texture = GetMapMeshTexture(pModel, meshIndex, AI_TEXTURE_TYPE::aiTextureType_NORMALS))
				textures[1] = std::move(texture);
			if (auto texture = GetMapMeshTexture(pModel, meshIndex, AI_TEXTURE_TYPE::aiTextureType_METALNESS))
				textures[2] = std::move(texture);
			if (auto texture = GetMapMeshTexture(pModel, meshIndex, AI_TEXTURE_TYPE::aiTextureType_EMISSIVE))
				textures[3] = std::move(texture);
		}

		auto [iter, inserted] = m_MapMeshTextureCache.emplace(pModel, std::move(textureSets));
		return &iter->second;
	}

HRESULT CMapMeshInstancingRenderer::BindMapMeshTextures(
	ID3D11DeviceContext* pContext,
	const std::vector<MAPMESH_TEXTURE_SET>& textureCache,
	uint32_t meshIndex) const
	{
		if (pContext == nullptr || meshIndex >= textureCache.size())
		{
			return E_FAIL;
		}

		ID3D11ShaderResourceView* srvs[MAPMESH_TEXTURE_COUNT]{};
		for (size_t i = 0; i < MAPMESH_TEXTURE_COUNT; ++i)
		{
			if (textureCache[meshIndex][i] == nullptr)
				return E_FAIL;
			srvs[i] = textureCache[meshIndex][i]->GetSRV().Get();
		}
		pContext->PSSetShaderResources(0, MAPMESH_TEXTURE_COUNT, srvs);

		return S_OK;
	}

HRESULT BindMapMeshMaterial(
	ID3D11DeviceContext* pContext,
	const SPtr<CResCBuffer>& materialConstantBuffer,
	_float3 emissiveColor,
	_float emissiveIntensity,
	_float objectAlpha)
	{
		if (pContext == nullptr || materialConstantBuffer == nullptr)
		{
			return E_FAIL;
		}

		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(pContext->Map(materialConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			return E_FAIL;
		}

		CB_MATERIAL material{};
		material.EmissiveColor = emissiveColor;
		material.EmissiveIntensity = emissiveIntensity;
		material.ObjectAlpha = objectAlpha;

		memcpy(mapped.pData, &material, sizeof(CB_MATERIAL));
		pContext->Unmap(materialConstantBuffer->GetCBuffer().Get(), 0);
		pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::MATERIAL), 1, materialConstantBuffer->GetCBuffer().GetAddressOf());

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
	s_pGpuCuller = CMapMeshGpuCuller::Create();

	if (s_pGpuCuller == nullptr)
		return E_FAIL;


	return S_OK;
}

HRESULT CMapMeshInstancingRenderer::PushMapObjectInstance(const SPtr<CResStaticModel>& pModel, EMapMeshRenderFeature renderFeature, const MAPMESH_INSTANCE_DATA& instanceData, MAPMESH_OCCLUSION_DATA& occlusionData)
{
	if (pModel == nullptr)
	{
		return E_FAIL;
	}
	MAPOBJECTKEY key{ pModel, renderFeature };
	auto& batch = s_InstanceBatches[key];

	occlusionData.instanceIndex = static_cast<uint32_t>(batch.instances.size());

	batch.instances.push_back(instanceData);
	batch.occlusionData.push_back(occlusionData);

	if(s_bInstancingEnabled)
		++s_FrameStats.iInstances;

	return S_OK;
}

void CMapMeshInstancingRenderer::Update()
{
	// 인스턴싱 ON 일 때
	if (s_bInstancingEnabled)
		CGameInstance::Get().AddRenderObject(RENDERGROUP::MAPMESH, this);
}

void CMapMeshInstancingRenderer::FrameEnd()
{
	ClearInstancingData();
	ClearFrameScratchBuffers();
}

void CMapMeshInstancingRenderer::ClearTextureCache()
{
	m_MapMeshTextureCache.clear();
}

void CMapMeshInstancingRenderer::EraseTextureCache(const SPtr<CResStaticModel>& model)
{
	//auto iter = m_MapMeshTextureCache.find(model);
	//if (iter == m_MapMeshTextureCache.end())
	//{
	//	return;
	//}
	m_MapMeshTextureCache.erase(model);
}

HRESULT CMapMeshInstancingRenderer::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	ZoneScopedN("MapMeshInstancingRender");

	DRAW_PACKET packet{};
	// 배치 병합, 캐시 준비, GPU 컬링
	if (FAILED(PrepareDrawPacket(pContext, ctx, packet)))
		return E_FAIL;
	if (!packet.bReady)
		return S_OK;

	const uint32_t commandCount = static_cast<uint32_t>(m_DrawCommandIndices.size());
	const uint32_t availableWorkers = CGameInstance::Get().GetRenderWorkerCount();
	const uint32_t workerCount = std::min({ 4u, availableWorkers, commandCount });
	if (workerCount == 0)
		return S_OK;

	std::vector<std::future<MAPMESH_COMMAND_LIST_RESULT>> commandListFutures{};
	commandListFutures.reserve(workerCount);

	const uint32_t commandsPerWorker = commandCount / workerCount;
	const uint32_t remainder = commandCount % workerCount;
	uint32_t commandBegin = 0;
	
	// 워커에게 commandList 기록 작업 분배
	for (uint32_t workerIndex = 0; workerIndex < workerCount; ++workerIndex)
	{
		const uint32_t commandEnd = commandBegin + commandsPerWorker + (workerIndex < remainder ? 1u : 0u);
		const std::string taskName = "MapMeshRecordDrawCommands_" + std::to_string(workerIndex);

		commandListFutures.push_back(
			CGameInstance::Get().RenderWorkerEnqueueWithFuture(
				taskName,
				[this, packet, commandBegin, commandEnd](ID3D11DeviceContext* pDeferredContext)
				{
					MAPMESH_COMMAND_LIST_RESULT result{};
					result.result = RecordDrawCommands(
						pDeferredContext, packet,
						commandBegin, commandEnd, result.drawCalls);

					if (FAILED(result.result))
						return result;

					result.result = pDeferredContext->FinishCommandList(FALSE, result.commandList.GetAddressOf());

					return result;
				}));

		commandBegin = commandEnd;
	}

	std::vector<MAPMESH_COMMAND_LIST_RESULT> commandListResults(workerCount);
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
		}
	}
	catch (...)
	{
		return E_FAIL;
	}

	// ImmediateContext가 gpu에게 commandList 제출
	{
		ZoneScopedN("MapMeshExecuteCommandLists");
		for (const auto& commandListResult : commandListResults)
		{
			s_FrameStats.iDrawCalls += commandListResult.drawCalls;
			pContext->ExecuteCommandList(commandListResult.commandList.Get(), TRUE);
		}
	}
	return S_OK;
}

HRESULT CMapMeshInstancingRenderer::PrepareDrawPacket(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx, DRAW_PACKET& outPacket)
{
	ZoneScopedN("MapMeshPrepareDrawPacket");
	outPacket = {};
	ClearFrameScratchBuffers();
	if (pContext == nullptr || s_InstanceBatches.empty())
		return S_OK;

	auto& gameInstance = CGameInstance::Get();

	outPacket.vertexStaticShader = gameInstance.GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim_Instanced");
	outPacket.vertexFoliageShader = gameInstance.GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim_Instanced_Foliage");
	outPacket.pixelShader = gameInstance.GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim_Instanced");
	outPacket.sampler = gameInstance.GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
	outPacket.materialConstantBuffer = gameInstance.GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_MATERIAL");

	if (!outPacket.vertexStaticShader || !outPacket.vertexFoliageShader ||
		!outPacket.pixelShader || !outPacket.sampler ||
		!outPacket.materialConstantBuffer)
		return E_FAIL;

	if (s_pGpuCuller == nullptr)
	{
		s_pGpuCuller = CMapMeshGpuCuller::Create();
		if (s_pGpuCuller == nullptr)
			return E_FAIL;
	}

	size_t totalInstances = 0;
	size_t totalDraws = 0;
	for (const auto& [pair, batch] : s_InstanceBatches)
	{
		const auto& model = pair.first;
		if (model && !batch.instances.empty())
		{
			totalInstances += batch.instances.size();
			totalDraws += model->Get_NumMeshes();
		}
	}

	m_Instances.reserve(totalInstances);
	m_OcclusionData.reserve(totalInstances);
	m_CullMeta.reserve(totalInstances);
	m_DrawBatchIndices.reserve(totalDraws);
	m_IndirectArgs.reserve(totalDraws);
	m_DrawItems.reserve(totalDraws);
	m_DrawCommandIndices.reserve(totalDraws);

	uint32_t batchIndex = 0;
	for (const auto& [pair, batch] : s_InstanceBatches)
	{
		const auto& model = pair.first;
		const auto& renderFeature = pair.second;
		if (!model || batch.instances.empty())
			continue;
		if (batch.occlusionData.size() != batch.instances.size())
			return E_FAIL;

		const auto* textureCache = GetOrCreateMapMeshTextureCache(model);
		if (textureCache == nullptr)
			return E_FAIL;

		const uint32_t instanceOffset = static_cast<uint32_t>(m_Instances.size());
		m_Instances.insert(m_Instances.end(), batch.instances.begin(), batch.instances.end());
		m_OcclusionData.insert(m_OcclusionData.end(), batch.occlusionData.begin(), batch.occlusionData.end());
		m_CullMeta.insert(m_CullMeta.end(), batch.instances.size(), MAPMESH_CULL_META{ instanceOffset, batchIndex });

		for (uint32_t meshIndex = 0; meshIndex < model->Get_NumMeshes(); ++meshIndex)
		{
			const auto& mesh = model->GetMeshes()[meshIndex];

			if (mesh == nullptr)
				continue;

			const uint32_t drawIndex = static_cast<uint32_t>(m_DrawItems.size());

			m_DrawBatchIndices.push_back(batchIndex);
			D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS args{};
			args.IndexCountPerInstance = static_cast<uint32_t>(mesh->GetNumIndices());
			m_IndirectArgs.push_back(args);
			m_DrawItems.push_back({ model, renderFeature, textureCache, meshIndex, instanceOffset });

			const size_t featureIndex = static_cast<size_t>(renderFeature);

			if (featureIndex >= RENDER_FEATURE_COUNT)
				return E_FAIL;

			m_DrawIndicesByFeature[featureIndex].push_back(drawIndex);
		}
		++batchIndex;
	}

	if (m_Instances.empty() || m_DrawItems.empty())
		return S_OK;

	for (const auto& drawIndices : m_DrawIndicesByFeature)
	{
		m_DrawCommandIndices.insert(
			m_DrawCommandIndices.end(), drawIndices.begin(), drawIndices.end());
	}

	if (m_DrawCommandIndices.size() != m_DrawItems.size())
		return E_FAIL;

	if (FAILED(BindMapMeshMaterial(
		pContext, outPacket.materialConstantBuffer,
		{ 1.f, 1.f, 1.f }, 0.f, 1.f)))
	{
		return E_FAIL;
	}

	if (FAILED(s_pGpuCuller->BuildVisibleInstancesAndIndirectArgs(
		pContext, m_Instances, m_OcclusionData, m_CullMeta, batchIndex,
		m_DrawBatchIndices, m_IndirectArgs,
		gameInstance.GetPrevHizBuffer(), ctx.matViewProj,
		gameInstance.GetClientScreenSize())))
	{
		return E_FAIL;
	}

	outPacket.visibleInstanceBuffer = s_pGpuCuller->GetVisibleInstanceBuffer();
	outPacket.indirectArgsBuffer = s_pGpuCuller->GetIndirectArgsBuffer();
	if (!outPacket.visibleInstanceBuffer || !outPacket.indirectArgsBuffer)
		return E_FAIL;

	ID3D11RenderTargetView* renderTargets[DRAW_PACKET::RENDER_TARGET_COUNT]{};
	ID3D11DepthStencilView* depthStencilView = nullptr;
	pContext->OMGetRenderTargets(DRAW_PACKET::RENDER_TARGET_COUNT, renderTargets, &depthStencilView);
	for (uint32_t i = 0; i < DRAW_PACKET::RENDER_TARGET_COUNT; ++i)
	{
		outPacket.renderTargets[i].Attach(renderTargets[i]);
	}
	outPacket.depthStencilView.Attach(depthStencilView);

	ID3D11DepthStencilState* depthStencilState = nullptr;
	pContext->OMGetDepthStencilState(&depthStencilState, &outPacket.stencilRef);
	outPacket.depthStencilState.Attach(depthStencilState);

	ID3D11RasterizerState* rasterizerState = nullptr;
	pContext->RSGetState(&rasterizerState);
	outPacket.rasterizerState.Attach(rasterizerState);

	ID3D11BlendState* blendState = nullptr;
	pContext->OMGetBlendState(&blendState, outPacket.blendFactor.data(), &outPacket.sampleMask);
	outPacket.blendState.Attach(blendState);

	UINT viewportCount = 1;
	pContext->RSGetViewports(&viewportCount, &outPacket.viewport);

	if (viewportCount != 1)
		return E_FAIL;

	ID3D11Buffer* perPassConstantBuffer = nullptr;
	pContext->VSGetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_PASS), 1, &perPassConstantBuffer);
	outPacket.perPassConstantBuffer.Attach(perPassConstantBuffer);

	ID3D11ShaderResourceView* noiseShaderResourceView = nullptr;
	pContext->PSGetShaderResources(13, 1, &noiseShaderResourceView);
	outPacket.noiseShaderResourceView.Attach(noiseShaderResourceView);

	if (!outPacket.depthStencilView || !outPacket.depthStencilState ||
		!outPacket.perPassConstantBuffer || !outPacket.noiseShaderResourceView ||
		std::ranges::any_of(outPacket.renderTargets,
			[](const auto& renderTarget) { return renderTarget == nullptr; }))
	{
		return E_FAIL;
	}

	outPacket.bReady = true;
	return S_OK;
}

HRESULT CMapMeshInstancingRenderer::RecordDrawCommands(
	ID3D11DeviceContext* pContext,
	const DRAW_PACKET& packet,
	uint32_t commandBegin,
	uint32_t commandEnd,
	uint32_t& outDrawCalls)
{
	ZoneScopedN("MapMeshRecordDrawCommands");
	outDrawCalls = 0;
	if (pContext == nullptr || !packet.bReady ||
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

	pContext->OMSetRenderTargets(DRAW_PACKET::RENDER_TARGET_COUNT, renderTargets, packet.depthStencilView.Get());
	pContext->OMSetDepthStencilState(packet.depthStencilState.Get(), packet.stencilRef);
	pContext->OMSetBlendState(packet.blendState.Get(), packet.blendFactor.data(), packet.sampleMask);
	pContext->RSSetState(packet.rasterizerState.Get());
	pContext->RSSetViewports(1, &packet.viewport);

	ID3D11Buffer* perPassConstantBuffer = packet.perPassConstantBuffer.Get();
	pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_PASS), 1, &perPassConstantBuffer);
	pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_PASS), 1, &perPassConstantBuffer);

	ID3D11ShaderResourceView* noiseShaderResourceView = packet.noiseShaderResourceView.Get();
	pContext->PSSetShaderResources(13, 1, &noiseShaderResourceView);

	pContext->PSSetShader(packet.pixelShader->GetPixelShader().Get(), nullptr, 0);
	pContext->PSSetSamplers(0, 1, packet.sampler->GetSamplerState().GetAddressOf());

	ID3D11Buffer* materialConstantBuffer = packet.materialConstantBuffer->GetCBuffer().Get();
	pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::MATERIAL), 1, &materialConstantBuffer);

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

			pContext->IASetInputLayout((*vertexShader)->GetInputLayout().Get());
			pContext->VSSetShader((*vertexShader)->GetVertexShader().Get(), nullptr, 0);
			currentFeature = item.renderFeature;
		}

		const auto& mesh = item.model->GetMeshes()[item.meshIndex];
		ID3D11Buffer* vertexBuffers[] = { mesh->GetVertexBuffer().Get(), packet.visibleInstanceBuffer.Get() };
		uint32_t strides[] = { mesh->GetVertexStride(), static_cast<uint32_t>(sizeof(MAPMESH_INSTANCE_DATA)) };
		uint32_t offsets[] = { 0, item.instanceOffset * static_cast<uint32_t>(sizeof(MAPMESH_INSTANCE_DATA)) };

		pContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
		pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());

		if (FAILED(BindMapMeshTextures(pContext, *item.textureCache, item.meshIndex)))
			return E_FAIL;

		pContext->DrawIndexedInstancedIndirect(packet.indirectArgsBuffer.Get(),
			drawIndex * static_cast<uint32_t>(sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS)));
		++outDrawCalls;
	}

	return S_OK;
}

bool CMapMeshInstancingRenderer::HasRenderPass(RENDERPASS ePass) const
{
	return ePass == RENDERPASS::DEFAULT;
}

void CMapMeshInstancingRenderer::SetInstancingEnabled(_bool bEnabled)
{
	if (s_bInstancingEnabled == bEnabled)
	{
		return;
	}

	s_bInstancingEnabled = bEnabled;
	ClearInstancingData();
}

void CMapMeshInstancingRenderer::ClearInstancingData()
{
	s_FrameStats.bEnabled = s_bInstancingEnabled;
	s_FrameStats.iInstances = 0;
	for (const auto& [pModel, instancesBatch] : s_InstanceBatches)
	{
		s_FrameStats.iInstances += static_cast<uint32_t>(instancesBatch.instances.size());
	}
	s_FrameStats.iBatches = static_cast<uint32_t>(s_InstanceBatches.size());
	s_LastStats = s_FrameStats;
	s_FrameStats = {};
	s_FrameStats.bEnabled = s_bInstancingEnabled;

	s_InstanceBatches.clear();
}

void CMapMeshInstancingRenderer::ClearFrameScratchBuffers()
{
	m_Instances.clear();
	m_OcclusionData.clear();
	m_CullMeta.clear();
	m_DrawBatchIndices.clear();
	m_IndirectArgs.clear();
	m_DrawItems.clear();
	m_DrawCommandIndices.clear();
	for (auto& drawIndices : m_DrawIndicesByFeature)
		drawIndices.clear();
}

void CMapMeshInstancingRenderer::ReleaseInstancingResources()
{
	s_InstanceBatches.clear();
	ClearFrameScratchBuffers();
	ClearTextureCache();
	s_pGpuCuller.reset();
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
