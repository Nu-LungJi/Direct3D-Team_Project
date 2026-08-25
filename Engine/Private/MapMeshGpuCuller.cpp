#include "pch.h"
#include "MapMeshGpuCuller.h"
#include "HizBuffer.h"

NS_USING(Engine)

uint32_t CMapMeshGpuCuller::CalculateExpandedCapacity(uint32_t currentCapacity, uint32_t requiredCapacity, uint32_t minimumCapacity)
{
	uint32_t expandedCapacity = std::max(currentCapacity, minimumCapacity);

	while (expandedCapacity < requiredCapacity)
		expandedCapacity *= 2;

	return expandedCapacity;
}

SPtr<CResStructuredBuffer> CMapMeshGpuCuller::CreateStructuredBuffer(uint32_t elementCount, uint32_t stride, uint32_t bindFlags)
{
	auto buffer = CResStructuredBuffer::Create();
	if (buffer == nullptr)
		return nullptr;

	CResStructuredBuffer::DESC desc{};
	desc.iNumElements = elementCount;
	desc.iStructureByteStride = stride;
	desc.iBindFlags = bindFlags;

	if (FAILED(buffer->Load(desc)))
		return nullptr;

	return buffer;
}

HRESULT CMapMeshGpuCuller::EnsureCapacity(uint32_t instanceCount, uint32_t batchCount, uint32_t drawCount)
{
	if (instanceCount == 0 || batchCount == 0 || drawCount == 0)
		return E_INVALIDARG;

	if (m_InstanceCapacity >= instanceCount && m_BatchCapacity >= batchCount && m_DrawCapacity >= drawCount &&
		m_pInstanceInputBuffer && m_pOcclusionInputBuffer && m_pCullMetaInputBuffer &&
		m_pDrawBatchInputBuffer && m_pBatchVisibleCountBuffer && m_pVisibleInstanceBuffer &&
		m_pVisibleInstanceVertexBuffer && m_pIndirectArgsBuffer && m_pIndirectArgsUAV &&
		m_pCullConstantBuffer && m_pIndirectArgsConstantBuffer)
	{
		return S_OK;
	}

	const uint32_t newInstanceCapacity = CalculateExpandedCapacity(m_InstanceCapacity, instanceCount, 256);
	const uint32_t newBatchCapacity = CalculateExpandedCapacity(m_BatchCapacity, batchCount, 64);
	const uint32_t newDrawCapacity = CalculateExpandedCapacity(m_DrawCapacity, drawCount, 256);

	// 새 용량을 모두 준비한 뒤에만 기존 버퍼와 교체해 생성 실패 시 기존 상태를 보존한다
	auto instanceInput = CreateStructuredBuffer(newInstanceCapacity, sizeof(MAPMESH_INSTANCE_DATA), D3D11_BIND_SHADER_RESOURCE);
	auto occlusionInput = CreateStructuredBuffer(newInstanceCapacity, sizeof(MAPMESH_OCCLUSION_DATA), D3D11_BIND_SHADER_RESOURCE);
	auto cullMetaInput = CreateStructuredBuffer(newInstanceCapacity, sizeof(MAPMESH_CULL_META), D3D11_BIND_SHADER_RESOURCE);
	auto drawBatchInput = CreateStructuredBuffer(newDrawCapacity, sizeof(uint32_t), D3D11_BIND_SHADER_RESOURCE);
	auto batchVisibleCount = CreateStructuredBuffer(newBatchCapacity, sizeof(uint32_t), D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
	auto visibleInstances = CreateStructuredBuffer(newInstanceCapacity, sizeof(MAPMESH_INSTANCE_DATA), D3D11_BIND_UNORDERED_ACCESS);
	if (!instanceInput || !occlusionInput || !cullMetaInput || !drawBatchInput || !batchVisibleCount || !visibleInstances)
		return E_FAIL;

	auto device = CGameInstance::Get().GetGraphicDevice();
	if (device == nullptr)
		return E_FAIL;

	ComPtr<ID3D11Buffer> visibleVertexBuffer;
	D3D11_BUFFER_DESC vertexDesc{};
	vertexDesc.ByteWidth = sizeof(MAPMESH_INSTANCE_DATA) * newInstanceCapacity;
	vertexDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	if (FAILED(device->CreateBuffer(&vertexDesc, nullptr, visibleVertexBuffer.GetAddressOf())))
		return E_FAIL;

	ComPtr<ID3D11Buffer> indirectArgsBuffer;
	D3D11_BUFFER_DESC argsDesc{};
	argsDesc.ByteWidth = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS) * newDrawCapacity;
	argsDesc.Usage = D3D11_USAGE_DEFAULT;
	argsDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	argsDesc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS | D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	if (FAILED(device->CreateBuffer(&argsDesc, nullptr, indirectArgsBuffer.GetAddressOf())))
		return E_FAIL;

	ComPtr<ID3D11UnorderedAccessView> indirectArgsUAV;
	D3D11_UNORDERED_ACCESS_VIEW_DESC argsUAVDesc{};
	argsUAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	argsUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	argsUAVDesc.Buffer.NumElements = argsDesc.ByteWidth / sizeof(uint32_t);
	argsUAVDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
	if (FAILED(device->CreateUnorderedAccessView(indirectArgsBuffer.Get(), &argsUAVDesc, indirectArgsUAV.GetAddressOf())))
		return E_FAIL;

	if (m_pCullConstantBuffer == nullptr)
	{
		D3D11_BUFFER_DESC constantBufferDesc{};
		constantBufferDesc.ByteWidth = sizeof(CB_MAPMESH_GPU_CULL);
		constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		if (FAILED(device->CreateBuffer(&constantBufferDesc, nullptr, m_pCullConstantBuffer.GetAddressOf())))
			return E_FAIL;
	}

	if (m_pIndirectArgsConstantBuffer == nullptr)
	{
		D3D11_BUFFER_DESC constantBufferDesc{};
		constantBufferDesc.ByteWidth = 16;
		constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		if (FAILED(device->CreateBuffer(&constantBufferDesc, nullptr, m_pIndirectArgsConstantBuffer.GetAddressOf())))
			return E_FAIL;
	}

	m_pInstanceInputBuffer = std::move(instanceInput);
	m_pOcclusionInputBuffer = std::move(occlusionInput);
	m_pCullMetaInputBuffer = std::move(cullMetaInput);
	m_pDrawBatchInputBuffer = std::move(drawBatchInput);
	m_pBatchVisibleCountBuffer = std::move(batchVisibleCount);
	m_pVisibleInstanceBuffer = std::move(visibleInstances);
	m_pVisibleInstanceVertexBuffer = std::move(visibleVertexBuffer);
	m_pIndirectArgsBuffer = std::move(indirectArgsBuffer);
	m_pIndirectArgsUAV = std::move(indirectArgsUAV);
	m_InstanceCapacity = newInstanceCapacity;
	m_BatchCapacity = newBatchCapacity;
	m_DrawCapacity = newDrawCapacity;
	return S_OK;
}

HRESULT CMapMeshGpuCuller::BuildVisibleInstancesAndIndirectArgs(
	ID3D11DeviceContext* context,
	const std::vector<MAPMESH_INSTANCE_DATA>& instances,
	const std::vector<MAPMESH_OCCLUSION_DATA>& occlusionData,
	const std::vector<MAPMESH_CULL_META>& cullMeta,
	uint32_t batchCount,
	const std::vector<uint32_t>& drawBatchIndices,
	const std::vector<D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS>& indirectArgs,
	const CHizBuffer* previousHizBuffer,
	_matrix matViewProj,
	const _float2& screenSize)
{
	ZoneScopedN("BuildVisibleInstancesAndIndirectArgs");
	if (context == nullptr || instances.empty() || instances.size() != occlusionData.size() ||
		instances.size() != cullMeta.size() || drawBatchIndices.empty() ||
		drawBatchIndices.size() != indirectArgs.size())
		return E_INVALIDARG;

	const uint32_t instanceCount = static_cast<uint32_t>(instances.size());
	const uint32_t drawCount = static_cast<uint32_t>(indirectArgs.size());
	if (FAILED(EnsureCapacity(instanceCount, batchCount, drawCount)))
		return E_FAIL;

	// 1단계: 이번 프레임의 인스턴스와 Draw-배치 대응 정보를 GPU 입력 버퍼에 업로드
	if (FAILED(m_pInstanceInputBuffer->UpdateData(instances.data(), sizeof(MAPMESH_INSTANCE_DATA) * instanceCount)) ||
		FAILED(m_pOcclusionInputBuffer->UpdateData(occlusionData.data(), sizeof(MAPMESH_OCCLUSION_DATA) * instanceCount)) ||
		FAILED(m_pCullMetaInputBuffer->UpdateData(cullMeta.data(), sizeof(MAPMESH_CULL_META) * instanceCount)) ||
		FAILED(m_pDrawBatchInputBuffer->UpdateData(drawBatchIndices.data(), sizeof(uint32_t) * drawCount)))
		return E_FAIL;

	// IndexCount 등 CPU가 아는 고정값을 먼저 기록하고 GPU가 가시 개수만 덮어쓰게 한다.
	D3D11_BOX indirectArgsUploadRegion{};
	indirectArgsUploadRegion.right = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS) * drawCount;
	indirectArgsUploadRegion.bottom = 1;
	indirectArgsUploadRegion.back = 1;
	context->UpdateSubresource(m_pIndirectArgsBuffer.Get(), 0, &indirectArgsUploadRegion, indirectArgs.data(), 0, 0);

	// 배치별 가시 개수는 매 프레임 컬링 전에 0부터 다시 누적한다.
	const UINT clearValues[4]{};
	context->ClearUnorderedAccessViewUint(m_pBatchVisibleCountBuffer->GetUAV().Get(), clearValues);

	auto cullShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_MapMeshGpuCull");
	if (cullShader == nullptr)
		return E_FAIL;

	ComPtr<ID3D11ShaderResourceView> previousHizSRV = previousHizBuffer ? previousHizBuffer->GetSRV() : nullptr;
	// 2단계: 인스턴스별 프러스텀,Hi-Z 판정 후 가시 데이터와 배치별 개수를 기록한다.
	ID3D11ShaderResourceView* cullShaderResources[] = {
		m_pInstanceInputBuffer->GetSRV().Get(),
		m_pOcclusionInputBuffer->GetSRV().Get(),
		previousHizSRV.Get(),
		m_pCullMetaInputBuffer->GetSRV().Get()
	};
	ID3D11UnorderedAccessView* cullOutputs[] = {
		m_pVisibleInstanceBuffer->GetUAV().Get(),
		m_pBatchVisibleCountBuffer->GetUAV().Get()
	};
	context->CSSetShader(cullShader->GetComputeShader().Get(), nullptr, 0);
	context->CSSetShaderResources(0, 4, cullShaderResources);
	context->CSSetUnorderedAccessViews(0, 2, cullOutputs, nullptr);

	CB_MAPMESH_GPU_CULL cullConstants{};
	XMStoreFloat4x4(&cullConstants.matViewProj, matViewProj);
	cullConstants.screenSize = screenSize;
	cullConstants.hizSize = previousHizBuffer
		? _float2{ static_cast<_float>(previousHizBuffer->GetWidth()), static_cast<_float>(previousHizBuffer->GetHeight()) }
		: _float2{};
	cullConstants.instanceCount = instanceCount;
	cullConstants.mipCount = previousHizBuffer ? previousHizBuffer->GetMipCount() : 0;
	cullConstants.useHiz = previousHizBuffer && previousHizSRV && cullConstants.mipCount > 0 && screenSize.x > 0.f && screenSize.y > 0.f;

	D3D11_MAPPED_SUBRESOURCE mappedResource{};
	if (FAILED(context->Map(m_pCullConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
		return E_FAIL;
	memcpy(mappedResource.pData, &cullConstants, sizeof(cullConstants));
	context->Unmap(m_pCullConstantBuffer.Get(), 0);
	context->CSSetConstantBuffers(0, 1, m_pCullConstantBuffer.GetAddressOf());
	context->Dispatch((instanceCount + 63) / 64, 1, 1);

	// 같은 버퍼를 다음 단계에서 다른 용도로 바인딩할 수 있도록 기존 슬롯을 해제
	ID3D11ShaderResourceView* nullCullShaderResources[4]{};
	ID3D11UnorderedAccessView* nullCullOutputs[2]{};
	context->CSSetShaderResources(0, 4, nullCullShaderResources);
	context->CSSetUnorderedAccessViews(0, 2, nullCullOutputs, nullptr);

	auto argsShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "CS_MapMeshBuildIndirectArgs");
	if (argsShader == nullptr)
		return E_FAIL;

	// 3단계: 배치별 가시 개수를 각 Draw의 InstanceCount에 반영한다
	ID3D11ShaderResourceView* indirectArgsShaderResources[] = {
		m_pBatchVisibleCountBuffer->GetSRV().Get(),
		m_pDrawBatchInputBuffer->GetSRV().Get()
	};
	ID3D11UnorderedAccessView* indirectArgsOutputs[] = { m_pIndirectArgsUAV.Get() };
	context->CSSetShader(argsShader->GetComputeShader().Get(), nullptr, 0);
	context->CSSetShaderResources(0, 2, indirectArgsShaderResources);
	context->CSSetUnorderedAccessViews(0, 1, indirectArgsOutputs, nullptr);

	if (FAILED(context->Map(m_pIndirectArgsConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
		return E_FAIL;
	memset(mappedResource.pData, 0, 16);
	memcpy(mappedResource.pData, &drawCount, sizeof(drawCount));
	context->Unmap(m_pIndirectArgsConstantBuffer.Get(), 0);
	context->CSSetConstantBuffers(0, 1, m_pIndirectArgsConstantBuffer.GetAddressOf());
	context->Dispatch((drawCount + 63) / 64, 1, 1);

	ID3D11ShaderResourceView* nullIndirectArgsShaderResources[2]{};
	ID3D11UnorderedAccessView* nullIndirectArgsOutputs[1]{};
	ID3D11Buffer* nullConstantBuffers[1]{};
	context->CSSetShaderResources(0, 2, nullIndirectArgsShaderResources);
	context->CSSetUnorderedAccessViews(0, 1, nullIndirectArgsOutputs, nullptr);
	context->CSSetConstantBuffers(0, 1, nullConstantBuffers);
	context->CSSetShader(nullptr, nullptr, 0);

	// 컬링 결과 구조화 버퍼를 IA 단계에서 사용할 인스턴스 버텍스 버퍼로 복사한다.
	context->CopyResource(m_pVisibleInstanceVertexBuffer.Get(), m_pVisibleInstanceBuffer->GetBuffer().Get());
	return S_OK;
}

ID3D11Buffer* CMapMeshGpuCuller::GetVisibleInstanceBuffer() const
{
	return m_pVisibleInstanceVertexBuffer.Get();
}

ID3D11Buffer* CMapMeshGpuCuller::GetIndirectArgsBuffer() const
{
	return m_pIndirectArgsBuffer.Get();
}

UPtr<CMapMeshGpuCuller> CMapMeshGpuCuller::Create()
{
	return ToUPtr(new CMapMeshGpuCuller);
}
