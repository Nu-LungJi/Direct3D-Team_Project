#include "pch.h"
#include "TerrainChunk.h"
#include "GameInstance.h"

NS_USING(Engine)

CTerrainChunk::CTerrainChunk(
	ComPtr<ID3D11Device> device,
	ComPtr<ID3D11DeviceContext> context)
	: m_pDevice{ std::move(device) }
	, m_pContext{ std::move(context) }
{
}

HRESULT CTerrainChunk::Initialize(const TERRAIN_CHUNK_DESC& desc)
{
	if (!m_pDevice || !m_pContext ||
		desc.vertexCountX < 2 || desc.vertexCountZ < 2 ||
		desc.vertexSpacing <= 0.f ||
		desc.maskResolution < 2 ||
		desc.vertices.size() != static_cast<size_t>(desc.vertexCountX) * desc.vertexCountZ ||
		desc.indices.empty())
	{
		return E_INVALIDARG;
	}

	for (const uint32_t index : desc.indices)
	{
		if (index >= desc.vertices.size())
			return E_INVALIDARG;
	}

	m_Coord = desc.coord;
	m_iVertexCountX = desc.vertexCountX;
	m_iVertexCountZ = desc.vertexCountZ;
	m_fVertexSpacing = desc.vertexSpacing;
	m_iMaskResolution = desc.maskResolution;
	m_Vertices = desc.vertices;
	m_Indices = desc.indices;

	D3D11_BUFFER_DESC vertexBufferDesc{};
	vertexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(VTX_NORMAL_TEX) * m_Vertices.size());
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexInitialData{};
	vertexInitialData.pSysMem = m_Vertices.data();
	if (FAILED(m_pDevice->CreateBuffer(&vertexBufferDesc, &vertexInitialData, m_pVertexBuffer.GetAddressOf())))
	{
		return E_FAIL;
	}

	m_BlendMask.assign(static_cast<size_t>(m_iMaskResolution) * m_iMaskResolution * 4, 0);
	for (size_t pixel = 0; pixel < m_BlendMask.size(); pixel += 4)
		m_BlendMask[pixel] = 255;
	D3D11_TEXTURE2D_DESC maskDesc{};
	maskDesc.Width = m_iMaskResolution;
	maskDesc.Height = m_iMaskResolution;
	maskDesc.MipLevels = 1;
	maskDesc.ArraySize = 1;
	maskDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	maskDesc.SampleDesc.Count = 1;
	maskDesc.Usage = D3D11_USAGE_DEFAULT;
	maskDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	D3D11_SUBRESOURCE_DATA maskData{};
	maskData.pSysMem = m_BlendMask.data();
	maskData.SysMemPitch = m_iMaskResolution * 4;
	if (FAILED(m_pDevice->CreateTexture2D(&maskDesc, &maskData, m_pBlendMaskTexture.GetAddressOf())) ||
		FAILED(m_pDevice->CreateShaderResourceView(m_pBlendMaskTexture.Get(), nullptr, m_pBlendMaskSRV.GetAddressOf())))
		return E_FAIL;

	D3D11_BUFFER_DESC indexBufferDesc{};
	indexBufferDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * m_Indices.size());
	indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA indexInitialData{};
	indexInitialData.pSysMem = m_Indices.data();
	if (FAILED(m_pDevice->CreateBuffer(&indexBufferDesc, &indexInitialData, m_pIndexBuffer.GetAddressOf())))
	{
		return E_FAIL;
	}

	RecalculateBounds();
	return S_OK;
}

bool CTerrainChunk::PaintBlendMask(const _float2& center, const _float2& radius,
	uint32_t layer, _float opacity, _float falloff, TERRAIN_MASK_DIRTY_RECT& dirtyRect)
{
	dirtyRect = {};
	if (layer >= 4 || radius.x <= 0.f || radius.y <= 0.f || opacity <= 0.f)
		return false;

	const int32_t startX = std::max(0, static_cast<int32_t>(std::floor((center.x - radius.x) * m_iMaskResolution)));
	const int32_t startY = std::max(0, static_cast<int32_t>(std::floor((center.y - radius.y) * m_iMaskResolution)));

	const int32_t endX = std::min(static_cast<int32_t>(m_iMaskResolution) - 1, static_cast<int32_t>(std::ceil((center.x + radius.x) * m_iMaskResolution)));
	const int32_t endY = std::min(static_cast<int32_t>(m_iMaskResolution) - 1, static_cast<int32_t>(std::ceil((center.y + radius.y) * m_iMaskResolution)));

	if (startX > endX || startY > endY) 
		return false;

	bool changed = false;
	for (int32_t y = startY; y <= endY; ++y)
	{
		const float v = (static_cast<float>(y) + 0.5f) / m_iMaskResolution;
		for (int32_t x = startX; x <= endX; ++x)
		{
			const float u = (static_cast<float>(x) + 0.5f) / m_iMaskResolution;
			const float dx = (u - center.x) / radius.x;
			const float dy = (v - center.y) / radius.y;
			const float distance = std::sqrt(dx * dx + dy * dy);

			if (distance > 1.f) 
				continue;

			const float amount = std::clamp(opacity * std::pow(1.f - distance, std::max(falloff, 0.01f)), 0.f, 1.f);
			const size_t offset = (static_cast<size_t>(y) * m_iMaskResolution + x) * 4;
			float weights[4]{};

			for (uint32_t channel = 0; channel < 4; ++channel)
				weights[channel] = m_BlendMask[offset + channel] / 255.f;

			for (uint32_t channel = 0; channel < 4; ++channel)
				weights[channel] *= 1.f - amount;

			weights[layer] += amount;
			for (uint32_t channel = 0; channel < 4; ++channel)
				m_BlendMask[offset + channel] = static_cast<uint8_t>(std::clamp(weights[channel] * 255.f, 0.f, 255.f));

			changed = true;
			if (!dirtyRect.valid)
			{
				dirtyRect = { static_cast<uint32_t>(x), static_cast<uint32_t>(y), static_cast<uint32_t>(x + 1), static_cast<uint32_t>(y + 1), true };
			}
			else
			{
				dirtyRect.left = std::min(dirtyRect.left, static_cast<uint32_t>(x));
				dirtyRect.top = std::min(dirtyRect.top, static_cast<uint32_t>(y));
				dirtyRect.right = std::max(dirtyRect.right, static_cast<uint32_t>(x + 1));
				dirtyRect.bottom = std::max(dirtyRect.bottom, static_cast<uint32_t>(y + 1));
			}
		}
	}
	return changed;
}

HRESULT CTerrainChunk::UploadBlendMask(const TERRAIN_MASK_DIRTY_RECT* dirtyRect)
{
	if (!m_pContext || !m_pBlendMaskTexture || m_BlendMask.empty()) 
		return E_FAIL;

	if (dirtyRect && !dirtyRect->valid) 
		return S_FALSE;

	D3D11_BOX box{};
	const D3D11_BOX* boxPtr = nullptr;
	const uint8_t* source = m_BlendMask.data();
	if (dirtyRect)
	{
		box = { dirtyRect->left, dirtyRect->top, 0,
			dirtyRect->right, dirtyRect->bottom, 1 };
		boxPtr = &box;
		source += (static_cast<size_t>(dirtyRect->top) * m_iMaskResolution + dirtyRect->left) * 4;
	}

	m_pContext->UpdateSubresource(m_pBlendMaskTexture.Get(), 0, boxPtr, source, m_iMaskResolution * 4, 0);

	return S_OK;
}

HRESULT CTerrainChunk::SetBlendMask(const std::vector<uint8_t>& mask)
{
	if (mask.size() != m_BlendMask.size()) 
		return E_INVALIDARG;

	m_BlendMask = mask;

	return UploadBlendMask();
}

HRESULT CTerrainChunk::UpdateVertices(const std::vector<VTX_NORMAL_TEX>& vertices)
{
	if (!m_pContext || !m_pVertexBuffer || vertices.size() != m_Vertices.size())
		return E_INVALIDARG;

	m_Vertices = vertices;
	m_pContext->UpdateSubresource(m_pVertexBuffer.Get(), 0, nullptr, m_Vertices.data(), 0, 0);

	RecalculateBounds();

	return S_OK;
}

void CTerrainChunk::Bind(ID3D11DeviceContext* context) const
{
	if (!context || !m_pVertexBuffer || !m_pIndexBuffer)
		return;

	ID3D11Buffer* vertexBuffer = m_pVertexBuffer.Get();
	constexpr UINT stride = sizeof(VTX_NORMAL_TEX);
	constexpr UINT offset = 0;
	context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	context->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void CTerrainChunk::Draw(ID3D11DeviceContext* context) const
{
	if (context && m_pIndexBuffer)
		context->DrawIndexed(static_cast<UINT>(m_Indices.size()), 0, 0);
}

void CTerrainChunk::RecalculateBounds()
{
	if (m_Vertices.empty())
	{
		m_LocalBounds = {};
		return;
	}

	_float3 minPosition = m_Vertices.front().pos;
	_float3 maxPosition = m_Vertices.front().pos;

	for (const auto& vertex : m_Vertices)
	{
		minPosition.x = std::min(minPosition.x, vertex.pos.x);
		minPosition.y = std::min(minPosition.y, vertex.pos.y);
		minPosition.z = std::min(minPosition.z, vertex.pos.z);
		maxPosition.x = std::max(maxPosition.x, vertex.pos.x);
		maxPosition.y = std::max(maxPosition.y, vertex.pos.y);
		maxPosition.z = std::max(maxPosition.z, vertex.pos.z);
	}

	const _float3 center
	{
		(minPosition.x + maxPosition.x) * 0.5f,
		(minPosition.y + maxPosition.y) * 0.5f,
		(minPosition.z + maxPosition.z) * 0.5f
	};
	const _float3 extents
	{
		(maxPosition.x - minPosition.x) * 0.5f,
		(maxPosition.y - minPosition.y) * 0.5f,
		(maxPosition.z - minPosition.z) * 0.5f
	};

	m_LocalBounds = BoundingBox{ center, extents };
}

UPtr<CTerrainChunk> CTerrainChunk::Create(const TERRAIN_CHUNK_DESC& desc)
{
	auto instance = ToUPtr(new CTerrainChunk{
		CGameInstance::Get().GetGraphicDevice(),
		CGameInstance::Get().GetGraphicDeviceContext() });

	if (FAILED(instance->Initialize(desc)))
		return nullptr;

	return instance;
}
