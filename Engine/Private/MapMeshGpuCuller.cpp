#include "pch.h"
#include "MapMeshGpuCuller.h"
#include "HizBuffer.h"

NS_USING(Engine)

namespace
{
	uint32_t GrowCapacity(uint32_t current, uint32_t required, uint32_t minimum)
	{
		uint32_t capacity = std::max(current, minimum);
		while (capacity < required)
			capacity *= 2;
		return capacity;
	}

	SPtr<CResStructuredBuffer> CreateStructuredBuffer(
		uint32_t elementCount,
		uint32_t stride,
		uint32_t bindFlags)
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
}

HRESULT CMapMeshGpuCuller::EnsureCapacity(uint32_t instanceCount, uint32_t batchCount, uint32_t drawCount)
{
	if (instanceCount == 0 || batchCount == 0 || drawCount == 0)
		return E_INVALIDARG;

	if (m_iCapacity >= instanceCount && m_iBatchCapacity >= batchCount && m_iDrawCapacity >= drawCount &&
		m_pInstanceInputBuffer && m_pOcclusionInputBuffer && m_pCullMetaInputBuffer &&
		m_pDrawBatchInputBuffer && m_pBatchVisibleCountBuffer && m_pVisibleInstanceBuffer &&
		m_pVisibleInstanceVertexBuffer && m_pIndirectArgsBuffer && m_pIndirectArgsUAV &&
		m_pCBuffer && m_pArgsCBuffer)
	{
		return S_OK;
	}

	const uint32_t newInstanceCapacity = GrowCapacity(m_iCapacity, instanceCount, 256);
	const uint32_t newBatchCapacity = GrowCapacity(m_iBatchCapacity, batchCount, 64);
	const uint32_t newDrawCapacity = GrowCapacity(m_iDrawCapacity, drawCount, 256);

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

	if (m_pCBuffer == nullptr)
	{
		D3D11_BUFFER_DESC cbDesc{};
		cbDesc.ByteWidth = sizeof(CB_MAPMESH_GPU_CULL);
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		if (FAILED(device->CreateBuffer(&cbDesc, nullptr, m_pCBuffer.GetAddressOf())))
			return E_FAIL;
	}

	if (m_pArgsCBuffer == nullptr)
	{
		D3D11_BUFFER_DESC cbDesc{};
		cbDesc.ByteWidth = 16;
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		if (FAILED(device->CreateBuffer(&cbDesc, nullptr, m_pArgsCBuffer.GetAddressOf())))
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
	m_iCapacity = newInstanceCapacity;
	m_iBatchCapacity = newBatchCapacity;
	m_iDrawCapacity = newDrawCapacity;
	return S_OK;
}

HRESULT CMapMeshGpuCuller::BuildVisibleInstancesAndIndirectArgs(
	ID3D11DeviceContext* pContext,
	const std::vector<MAPMESH_INSTANCE_DATA>& instances,
	const std::vector<MAPMESH_OCCLUSION_DATA>& occlusionData,
	const std::vector<MAPMESH_CULL_META>& cullMeta,
	uint32_t batchCount,
	const std::vector<uint32_t>& drawBatchIndices,
	const std::vector<D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS>& indirectArgs,
	const CHizBuffer* pPrevHizBuffer,
	_matrix matViewProj,
	const _float2& screenSize)
{
	ZoneScopedN("BuildVisibleInstancesAndIndirectArgs");
	if (pContext == nullptr || instances.empty() || instances.size() != occlusionData.size() ||
		instances.size() != cullMeta.size() || drawBatchIndices.empty() ||
		drawBatchIndices.size() != indirectArgs.size())
		return E_INVALIDARG;

	const uint32_t instanceCount = static_cast<uint32_t>(instances.size());
	const uint32_t drawCount = static_cast<uint32_t>(indirectArgs.size());
	if (FAILED(EnsureCapacity(instanceCount, batchCount, drawCount)))
		return E_FAIL;

	if (FAILED(m_pInstanceInputBuffer->UpdateData(instances.data(), sizeof(MAPMESH_INSTANCE_DATA) * instanceCount)) ||
		FAILED(m_pOcclusionInputBuffer->UpdateData(occlusionData.data(), sizeof(MAPMESH_OCCLUSION_DATA) * instanceCount)) ||
		FAILED(m_pCullMetaInputBuffer->UpdateData(cullMeta.data(), sizeof(MAPMESH_CULL_META) * instanceCount)) ||
		FAILED(m_pDrawBatchInputBuffer->UpdateData(drawBatchIndices.data(), sizeof(uint32_t) * drawCount)))
		return E_FAIL;

	D3D11_BOX argsBox{};
	argsBox.right = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS) * drawCount;
	argsBox.bottom = 1;
	argsBox.back = 1;
	pContext->UpdateSubresource(m_pIndirectArgsBuffer.Get(), 0, &argsBox, indirectArgs.data(), 0, 0);

	const UINT clearValues[4]{};
	pContext->ClearUnorderedAccessViewUint(m_pBatchVisibleCountBuffer->GetUAV().Get(), clearValues);

	auto cullShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "CS_MapMeshGpuCull");
	if (cullShader == nullptr)
		return E_FAIL;

	ComPtr<ID3D11ShaderResourceView> prevHizSRV = pPrevHizBuffer ? pPrevHizBuffer->GetSRV() : nullptr;
	ID3D11ShaderResourceView* cullSRVs[] = {
		m_pInstanceInputBuffer->GetSRV().Get(),
		m_pOcclusionInputBuffer->GetSRV().Get(),
		prevHizSRV.Get(),
		m_pCullMetaInputBuffer->GetSRV().Get()
	};
	ID3D11UnorderedAccessView* cullUAVs[] = {
		m_pVisibleInstanceBuffer->GetUAV().Get(),
		m_pBatchVisibleCountBuffer->GetUAV().Get()
	};
	pContext->CSSetShader(cullShader->GetComputeShader().Get(), nullptr, 0);
	pContext->CSSetShaderResources(0, 4, cullSRVs);
	pContext->CSSetUnorderedAccessViews(0, 2, cullUAVs, nullptr);

	CB_MAPMESH_GPU_CULL cb{};
	XMStoreFloat4x4(&cb.matViewProj, matViewProj);
	cb.screenSize = screenSize;
	cb.hizSize = pPrevHizBuffer
		? _float2{ static_cast<_float>(pPrevHizBuffer->GetWidth()), static_cast<_float>(pPrevHizBuffer->GetHeight()) }
		: _float2{};
	cb.instanceCount = instanceCount;
	cb.mipCount = pPrevHizBuffer ? pPrevHizBuffer->GetMipCount() : 0;
	cb.useHiz = pPrevHizBuffer && prevHizSRV && cb.mipCount > 0 && screenSize.x > 0.f && screenSize.y > 0.f;

	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (FAILED(pContext->Map(m_pCBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		return E_FAIL;
	memcpy(mapped.pData, &cb, sizeof(cb));
	pContext->Unmap(m_pCBuffer.Get(), 0);
	pContext->CSSetConstantBuffers(0, 1, m_pCBuffer.GetAddressOf());
	pContext->Dispatch((instanceCount + 63) / 64, 1, 1);

	ID3D11ShaderResourceView* nullCullSRVs[4]{};
	ID3D11UnorderedAccessView* nullCullUAVs[2]{};
	pContext->CSSetShaderResources(0, 4, nullCullSRVs);
	pContext->CSSetUnorderedAccessViews(0, 2, nullCullUAVs, nullptr);

	auto argsShader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "CS_MapMeshBuildIndirectArgs");
	if (argsShader == nullptr)
		return E_FAIL;

	ID3D11ShaderResourceView* argsSRVs[] = {
		m_pBatchVisibleCountBuffer->GetSRV().Get(),
		m_pDrawBatchInputBuffer->GetSRV().Get()
	};
	ID3D11UnorderedAccessView* argsUAVs[] = { m_pIndirectArgsUAV.Get() };
	pContext->CSSetShader(argsShader->GetComputeShader().Get(), nullptr, 0);
	pContext->CSSetShaderResources(0, 2, argsSRVs);
	pContext->CSSetUnorderedAccessViews(0, 1, argsUAVs, nullptr);

	if (FAILED(pContext->Map(m_pArgsCBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		return E_FAIL;
	memset(mapped.pData, 0, 16);
	memcpy(mapped.pData, &drawCount, sizeof(drawCount));
	pContext->Unmap(m_pArgsCBuffer.Get(), 0);
	pContext->CSSetConstantBuffers(0, 1, m_pArgsCBuffer.GetAddressOf());
	pContext->Dispatch((drawCount + 63) / 64, 1, 1);

	ID3D11ShaderResourceView* nullArgsSRVs[2]{};
	ID3D11UnorderedAccessView* nullArgsUAVs[1]{};
	ID3D11Buffer* nullCBuffers[1]{};
	pContext->CSSetShaderResources(0, 2, nullArgsSRVs);
	pContext->CSSetUnorderedAccessViews(0, 1, nullArgsUAVs, nullptr);
	pContext->CSSetConstantBuffers(0, 1, nullCBuffers);
	pContext->CSSetShader(nullptr, nullptr, 0);

	pContext->CopyResource(m_pVisibleInstanceVertexBuffer.Get(), m_pVisibleInstanceBuffer->GetBuffer().Get());
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
