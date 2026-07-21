#include "pch.h"
#include "Terrain.h"

#include "ComConstantBuffer.h"
#include "GameInstance.h"
#include "Resources.h"

NS_USING(Engine)

namespace
{
	void AddNormal(VTX_NORMAL_TEX& vertex, FXMVECTOR normal)
	{
		XMStoreFloat3(&vertex.normal, XMLoadFloat3(&vertex.normal) + normal);
	}
}

CTerrain::CTerrain() : CGameObject{} {}
CTerrain::CTerrain(const CTerrain& prototype) : CGameObject{ prototype } {}

HRESULT CTerrain::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CTerrain::Initialize(void* pArg)
{
	const auto* desc = static_cast<const DESC*>(pArg);
	if (!desc || desc->textureGroup.empty() || desc->textureTag.empty() ||
		desc->shaderGroup.empty() || desc->vertexShaderTag.empty() ||
		desc->pixelShaderTag.empty() || desc->chunkQuadCount == 0 ||
		desc->vertexCountX < 2 || desc->vertexCountZ < 2 ||
		desc->vertexSpacing <= 0.f || desc->heightScale < 0.f)
		return E_INVALIDARG;

	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	m_pTerrainTextures[0] = CGameInstance::Get().GetResourceFirst<CResTexture2D>(desc->textureGroup, desc->textureTag);
	for (uint32_t layer = 1; layer < m_pTerrainTextures.size(); ++layer)
		m_pTerrainTextures[layer] = m_pTerrainTextures[0];
	m_pVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(desc->shaderGroup, desc->vertexShaderTag);
	m_pPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(desc->shaderGroup, desc->pixelShaderTag);
	if (!m_pTerrainTextures[0] || !m_pVertexShader || !m_pPixelShader ||
		FAILED(m_pVertexShader->Load()) || FAILED(m_pPixelShader->Load()))
		return E_FAIL;
	D3D11_BUFFER_DESC chunkBufferDesc{};
	chunkBufferDesc.ByteWidth = 16;
	chunkBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	chunkBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	chunkBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if (FAILED(CGameInstance::Get().GetGraphicDevice()->CreateBuffer(
		&chunkBufferDesc, nullptr, m_pChunkCBuffer.GetAddressOf()))) return E_FAIL;

	CComConstantBuffer::DESC cbufferDesc{};
	cbufferDesc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
	if (FAILED(AddComponentFromProto(
		"PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject",
		&cbufferDesc, &m_pCBufferPerObject)))
		return E_FAIL;

	const HRESULT terrainDataResult = desc->heightMapPath.empty()
		? CreateFlatTerrain(*desc)
		: LoadHeightMap(*desc);
	if (FAILED(terrainDataResult) ||
		FAILED(BuildChunks(desc->chunkQuadCount, desc->vertexSpacing, desc->maskResolution)))
		return E_FAIL;

	return S_OK;
}

HRESULT CTerrain::SetTileTexture(uint32_t layer, SPtr<CResTexture2D> texture)
{
	if (layer >= m_pTerrainTextures.size() || !texture) return E_INVALIDARG;
	if (texture->GetState() != CResource::STATE::LOADED && FAILED(texture->Load())) return E_FAIL;
	m_pTerrainTextures[layer] = std::move(texture);
	return S_OK;
}

SPtr<CResTexture2D> CTerrain::GetTileTexture(uint32_t layer) const
{
	return layer < m_pTerrainTextures.size() ? m_pTerrainTextures[layer] : nullptr;
}

HRESULT CTerrain::PaintTileLocal(const _float2& center, const _float2& radius,
	uint32_t layer, _float opacity, _float falloff)
{
	if (layer >= 4 || radius.x <= 0.f || radius.y <= 0.f) return E_INVALIDARG;
	bool changed = false;
	for (const auto& chunk : m_Chunks)
	{
		const float startX = static_cast<float>(chunk->GetCoord().x * m_iChunkQuadCount) * m_fVertexSpacing;
		const float startZ = static_cast<float>(chunk->GetCoord().z * m_iChunkQuadCount) * m_fVertexSpacing;
		const float sizeX = static_cast<float>(chunk->GetVertexCountX() - 1) * m_fVertexSpacing;
		const float sizeZ = static_cast<float>(chunk->GetVertexCountZ() - 1) * m_fVertexSpacing;
		if (center.x + radius.x < startX || center.x - radius.x > startX + sizeX ||
			center.y + radius.y < startZ || center.y - radius.y > startZ + sizeZ) continue;
		const _float2 uvCenter{ (center.x - startX) / sizeX, (center.y - startZ) / sizeZ };
		const _float2 uvRadius{ radius.x / sizeX, radius.y / sizeZ };
		if (chunk->PaintBlendMask(uvCenter, uvRadius, layer, opacity, falloff))
		{
			if (FAILED(chunk->UploadBlendMask())) return E_FAIL;
			changed = true;
		}
	}
	return changed ? S_OK : S_FALSE;
}

HRESULT CTerrain::AddChunkPositiveX()
{
	return ExpandTerrain(true);
}

HRESULT CTerrain::AddChunkPositiveZ()
{
	return ExpandTerrain(false);
}

HRESULT CTerrain::AddChunkNegativeX()
{
	return PrependTerrain(true);
}

HRESULT CTerrain::AddChunkNegativeZ()
{
	return PrependTerrain(false);
}

HRESULT CTerrain::PrependTerrain(bool negativeX)
{
	if (m_iChunkQuadCount == 0 ||
		(m_iVertexCountX - 1) % m_iChunkQuadCount != 0 ||
		(m_iVertexCountZ - 1) % m_iChunkQuadCount != 0)
		return E_FAIL;

	const uint32_t oldCountX = m_iVertexCountX;
	const uint32_t oldCountZ = m_iVertexCountZ;
	const uint32_t oldChunkCountX = (oldCountX - 1) / m_iChunkQuadCount;
	const uint32_t oldChunkCountZ = (oldCountZ - 1) / m_iChunkQuadCount;
	const uint32_t offsetX = negativeX ? m_iChunkQuadCount : 0;
	const uint32_t offsetZ = negativeX ? 0 : m_iChunkQuadCount;
	m_iVertexCountX += offsetX;
	m_iVertexCountZ += offsetZ;

	std::vector<VTX_NORMAL_TEX> expanded(
		static_cast<size_t>(m_iVertexCountX) * m_iVertexCountZ);
	for (uint32_t z = 0; z < m_iVertexCountZ; ++z)
	{
		for (uint32_t x = 0; x < m_iVertexCountX; ++x)
		{
			const uint32_t sourceX = x >= offsetX ? x - offsetX : 0;
			const uint32_t sourceZ = z >= offsetZ ? z - offsetZ : 0;
			auto& vertex = expanded[static_cast<size_t>(z) * m_iVertexCountX + x];
			vertex = m_Vertices[static_cast<size_t>(std::min(sourceZ, oldCountZ - 1)) * oldCountX +
				std::min(sourceX, oldCountX - 1)];
			vertex.pos.x = static_cast<float>(x) * m_fVertexSpacing;
			vertex.pos.z = static_cast<float>(z) * m_fVertexSpacing;
			vertex.texCoord = {
				static_cast<float>(x) / static_cast<float>(m_iVertexCountX - 1),
				static_cast<float>(z) / static_cast<float>(m_iVertexCountZ - 1) };
		}
	}
	m_Vertices = std::move(expanded);

	for (auto& chunk : m_Chunks)
	{
		auto coord = chunk->GetCoord();
		if (negativeX) ++coord.x;
		else ++coord.z;
		chunk->SetCoord(coord);
	}

	const float localDistance = static_cast<float>(m_iChunkQuadCount) * m_fVertexSpacing;
	auto& transform = GetTransform();
	const float axisScale = negativeX ? transform.GetScale().x : transform.GetScale().z;
	const _vector axis = negativeX ? transform.GetState(STATE::RIGHT) : transform.GetState(STATE::LOOK);
	transform.SetPosition(transform.GetLoadedPostion() - axis * localDistance * axisScale);
	transform.Update();

	BuildGridIndices();
	RecalculateNormals();
	RecalculateBounds();
	if (FAILED(UpdateChunks(0, 0, m_iVertexCountX - 1, m_iVertexCountZ - 1)))
		return E_FAIL;

	if (negativeX)
	{
		for (uint32_t chunkZ = 0; chunkZ < oldChunkCountZ; ++chunkZ)
		{
			auto chunk = CreateChunk(0, chunkZ);
			if (!chunk) return E_FAIL;
			m_Chunks.push_back(std::move(chunk));
		}
	}
	else
	{
		for (uint32_t chunkX = 0; chunkX < oldChunkCountX; ++chunkX)
		{
			auto chunk = CreateChunk(chunkX, 0);
			if (!chunk) return E_FAIL;
			m_Chunks.push_back(std::move(chunk));
		}
	}
	UpdateChunkVisibility();
	return S_OK;
}

HRESULT CTerrain::ExpandTerrain(bool positiveX)
{
	if (m_iChunkQuadCount == 0 ||
		(m_iVertexCountX - 1) % m_iChunkQuadCount != 0 ||
		(m_iVertexCountZ - 1) % m_iChunkQuadCount != 0)
		return E_FAIL;

	const uint32_t oldCountX = m_iVertexCountX;
	const uint32_t oldCountZ = m_iVertexCountZ;
	const uint32_t oldChunkCountX = (oldCountX - 1) / m_iChunkQuadCount;
	const uint32_t oldChunkCountZ = (oldCountZ - 1) / m_iChunkQuadCount;
	m_iVertexCountX += positiveX ? m_iChunkQuadCount : 0;
	m_iVertexCountZ += positiveX ? 0 : m_iChunkQuadCount;

	std::vector<VTX_NORMAL_TEX> expanded(
		static_cast<size_t>(m_iVertexCountX) * m_iVertexCountZ);
	for (uint32_t z = 0; z < m_iVertexCountZ; ++z)
	{
		for (uint32_t x = 0; x < m_iVertexCountX; ++x)
		{
			auto& vertex = expanded[static_cast<size_t>(z) * m_iVertexCountX + x];
			if (x < oldCountX && z < oldCountZ)
				vertex = m_Vertices[static_cast<size_t>(z) * oldCountX + x];
			else
			{
				const uint32_t sourceX = std::min(x, oldCountX - 1);
				const uint32_t sourceZ = std::min(z, oldCountZ - 1);
				vertex.pos = { static_cast<float>(x) * m_fVertexSpacing,
					m_Vertices[static_cast<size_t>(sourceZ) * oldCountX + sourceX].pos.y,
					static_cast<float>(z) * m_fVertexSpacing };
				vertex.normal = { 0.f, 1.f, 0.f };
			}
			vertex.texCoord = {
				static_cast<float>(x) / static_cast<float>(m_iVertexCountX - 1),
				static_cast<float>(z) / static_cast<float>(m_iVertexCountZ - 1) };
		}
	}
	m_Vertices = std::move(expanded);
	BuildGridIndices();
	RecalculateNormals();
	RecalculateBounds();
	if (FAILED(UpdateChunks(0, 0, oldCountX - 1, oldCountZ - 1)))
		return E_FAIL;

	if (positiveX)
	{
		for (uint32_t chunkZ = 0; chunkZ < oldChunkCountZ; ++chunkZ)
		{
			auto chunk = CreateChunk(oldChunkCountX, chunkZ);
			if (!chunk) return E_FAIL;
			m_Chunks.push_back(std::move(chunk));
		}
	}
	else
	{
		for (uint32_t chunkX = 0; chunkX < oldChunkCountX; ++chunkX)
		{
			auto chunk = CreateChunk(chunkX, oldChunkCountZ);
			if (!chunk) return E_FAIL;
			m_Chunks.push_back(std::move(chunk));
		}
	}
	UpdateChunkVisibility();
	return S_OK;
}

_float CTerrain::GetVertexHeight(uint32_t x, uint32_t z) const
{
	if (x >= m_iVertexCountX || z >= m_iVertexCountZ)
		return 0.f;
	return m_Vertices[static_cast<size_t>(z) * m_iVertexCountX + x].pos.y;
}

bool CTerrain::TryGetLocalHeight(_float localX, _float localZ, _float& outHeight) const
{
	if (m_Vertices.empty() || localX < 0.f || localZ < 0.f)
		return false;

	const _float gridX = localX / m_fVertexSpacing;
	const _float gridZ = localZ / m_fVertexSpacing;
	if (gridX > static_cast<_float>(m_iVertexCountX - 1) ||
		gridZ > static_cast<_float>(m_iVertexCountZ - 1))
		return false;

	const uint32_t x0 = static_cast<uint32_t>(std::floor(gridX));
	const uint32_t z0 = static_cast<uint32_t>(std::floor(gridZ));
	const uint32_t x1 = std::min(x0 + 1, m_iVertexCountX - 1);
	const uint32_t z1 = std::min(z0 + 1, m_iVertexCountZ - 1);
	const _float tx = gridX - static_cast<_float>(x0);
	const _float tz = gridZ - static_cast<_float>(z0);
	const _float top = std::lerp(GetVertexHeight(x0, z0), GetVertexHeight(x1, z0), tx);
	const _float bottom = std::lerp(GetVertexHeight(x0, z1), GetVertexHeight(x1, z1), tx);
	outHeight = std::lerp(top, bottom, tz);
	return true;
}

HRESULT CTerrain::SetVertexHeight(uint32_t x, uint32_t z, _float height)
{
	if (x >= m_iVertexCountX || z >= m_iVertexCountZ || !std::isfinite(height))
		return E_INVALIDARG;
	m_Vertices[static_cast<size_t>(z) * m_iVertexCountX + x].pos.y = height;
	return S_OK;
}

HRESULT CTerrain::CommitHeightRegion(uint32_t minX, uint32_t minZ, uint32_t maxX, uint32_t maxZ)
{
	if (m_Vertices.empty() || minX > maxX || minZ > maxZ ||
		minX >= m_iVertexCountX || minZ >= m_iVertexCountZ)
		return E_INVALIDARG;

	maxX = std::min(maxX, m_iVertexCountX - 1);
	maxZ = std::min(maxZ, m_iVertexCountZ - 1);
	const uint32_t normalMinX = minX > 0 ? minX - 1 : 0;
	const uint32_t normalMinZ = minZ > 0 ? minZ - 1 : 0;
	const uint32_t normalMaxX = std::min(maxX + 1, m_iVertexCountX - 1);
	const uint32_t normalMaxZ = std::min(maxZ + 1, m_iVertexCountZ - 1);
	RecalculateNormals(normalMinX, normalMinZ, normalMaxX, normalMaxZ);
	RecalculateBounds();
	return UpdateChunks(normalMinX, normalMinZ, normalMaxX, normalMaxZ);
}

void CTerrain::LateUpdate(_float fTimeDelta)
{
	GetTransform().Update();
	UpdateChunkVisibility();
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
}

HRESULT CTerrain::Render(ID3D11DeviceContext* context, const RENDER_CTX& renderContext)
{
	if (!context || !m_pCBufferPerObject || !m_pVertexShader || !m_pPixelShader || !m_pTerrainTextures[0])
		return E_FAIL;

	CB_PER_OBJECT cbPerObject{};
	cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(&cbPerObject.matWVP,
		GetTransform().GetLoadedCombinedWorldMatrix() * renderContext.matViewProj);
	if (FAILED(m_pCBufferPerObject->MapDiscard(context, &cbPerObject, sizeof(cbPerObject))))
		return E_FAIL;

	context->VSSetConstantBuffers(0, 1, m_pCBufferPerObject->GetAdressOfBuffer());
	context->PSSetConstantBuffers(0, 1, m_pCBufferPerObject->GetAdressOfBuffer());
	context->IASetInputLayout(m_pVertexShader->GetInputLayout().Get());
	context->VSSetShader(m_pVertexShader->GetVertexShader().Get(), nullptr, 0);
	context->PSSetShader(
		renderContext.pass == RENDERPASS::DEPTH ? nullptr : m_pPixelShader->GetPixelShader().Get(),
		nullptr, 0);

	auto materialBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(
		TAG_RES_GRP_PERMANENT_BUFFER, "CB_MATERIAL");
	if (!materialBuffer)
		return E_FAIL;

	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (SUCCEEDED(context->Map(materialBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
	{
		CB_MATERIAL material{};
		material.EmissiveColor = { 1.f, 0.f, 0.f };
		material.EmissiveIntensity = 1.f;
		material.ObjectAlpha = 1.f;
		memcpy(mapped.pData, &material, sizeof(material));
		context->Unmap(materialBuffer->GetCBuffer().Get(), 0);
	}
	context->PSSetConstantBuffers(3, 1, materialBuffer->GetCBuffer().GetAddressOf());
	ID3D11ShaderResourceView* tileSrvs[4]{};
	for (uint32_t layer = 0; layer < 4; ++layer)
		tileSrvs[layer] = (m_pTerrainTextures[layer] ? m_pTerrainTextures[layer] : m_pTerrainTextures[0])->GetSRV().Get();
	context->PSSetShaderResources(0, 4, tileSrvs);

	for (const auto* chunk : m_VisibleChunks)
	{
		const float totalX = static_cast<float>(m_iVertexCountX - 1);
		const float totalZ = static_cast<float>(m_iVertexCountZ - 1);
		const float offsetX = static_cast<float>(chunk->GetCoord().x * m_iChunkQuadCount) / totalX;
		const float offsetZ = static_cast<float>(chunk->GetCoord().z * m_iChunkQuadCount) / totalZ;
		const float spanX = static_cast<float>(chunk->GetVertexCountX() - 1) / totalX;
		const float spanZ = static_cast<float>(chunk->GetVertexCountZ() - 1) / totalZ;
		const _float4 chunkUV{ offsetX, offsetZ, spanX, spanZ };
		D3D11_MAPPED_SUBRESOURCE chunkMapped{};
		if (FAILED(context->Map(m_pChunkCBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &chunkMapped))) return E_FAIL;
		memcpy(chunkMapped.pData, &chunkUV, sizeof(chunkUV));
		context->Unmap(m_pChunkCBuffer.Get(), 0);
		context->PSSetConstantBuffers(11, 1, m_pChunkCBuffer.GetAddressOf());
		ID3D11ShaderResourceView* maskSrv = chunk->GetBlendMaskSRV();
		context->PSSetShaderResources(4, 1, &maskSrv);
		chunk->Bind(context);
		chunk->Draw(context);
	}

	ID3D11ShaderResourceView* nullSrv = nullptr;
	ID3D11ShaderResourceView* nullSrvs[5]{};
	context->PSSetShaderResources(0, 5, nullSrvs);
	return S_OK;
}

bool CTerrain::IsOcclusionCullable() const
{
	return !m_Chunks.empty();
}

bool CTerrain::GetOcclusionBounds(BoundingBox& outBounds) const
{
	if (m_Vertices.empty())
		return false;
	m_LocalBounds.Transform(outBounds, GetTransform().GetLoadedCombinedWorldMatrix());
	return true;
}

HRESULT CTerrain::LoadHeightMap(const DESC& desc)
{
	TexMetadata metadata{};
	ScratchImage sourceImage{};
	if (FAILED(LoadFromWICFile(StringToWString(desc.heightMapPath).c_str(),
		WIC_FLAGS_NONE, &metadata, sourceImage)))
		return E_FAIL;

	ScratchImage convertedImage{};
	const ScratchImage* heightImage = &sourceImage;
	if (metadata.format != DXGI_FORMAT_R8G8B8A8_UNORM)
	{
		if (FAILED(Convert(sourceImage.GetImages(), sourceImage.GetImageCount(),
			sourceImage.GetMetadata(), DXGI_FORMAT_R8G8B8A8_UNORM,
			TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, convertedImage)))
			return E_FAIL;
		heightImage = &convertedImage;
		metadata = convertedImage.GetMetadata();
	}

	if (metadata.width < 2 || metadata.height < 2 ||
		metadata.width > std::numeric_limits<uint32_t>::max() ||
		metadata.height > std::numeric_limits<uint32_t>::max())
		return E_FAIL;

	const Image* image = heightImage->GetImage(0, 0, 0);
	if (!image || !image->pixels)
		return E_FAIL;

	m_iVertexCountX = static_cast<uint32_t>(metadata.width);
	m_iVertexCountZ = static_cast<uint32_t>(metadata.height);
	m_Vertices.assign(static_cast<size_t>(m_iVertexCountX) * m_iVertexCountZ, {});

	for (uint32_t z = 0; z < m_iVertexCountZ; ++z)
	{
		const uint8_t* row = image->pixels + static_cast<size_t>(z) * image->rowPitch;
		for (uint32_t x = 0; x < m_iVertexCountX; ++x)
		{
			const size_t index = static_cast<size_t>(z) * m_iVertexCountX + x;
			auto& vertex = m_Vertices[index];
			vertex.pos = {
				static_cast<_float>(x) * desc.vertexSpacing,
				static_cast<_float>(row[static_cast<size_t>(x) * 4]) * desc.heightScale,
				static_cast<_float>(z) * desc.vertexSpacing };
			vertex.texCoord = {
				static_cast<_float>(x) / static_cast<_float>(m_iVertexCountX - 1),
				static_cast<_float>(z) / static_cast<_float>(m_iVertexCountZ - 1) };
		}
	}

	BuildGridIndices();
	RecalculateNormals();
	RecalculateBounds();
	return S_OK;
}

HRESULT CTerrain::CreateFlatTerrain(const DESC& desc)
{
	m_iVertexCountX = desc.vertexCountX;
	m_iVertexCountZ = desc.vertexCountZ;
	m_Vertices.assign(static_cast<size_t>(m_iVertexCountX) * m_iVertexCountZ, {});

	for (uint32_t z = 0; z < m_iVertexCountZ; ++z)
	{
		for (uint32_t x = 0; x < m_iVertexCountX; ++x)
		{
			auto& vertex = m_Vertices[static_cast<size_t>(z) * m_iVertexCountX + x];
			vertex.pos = {
				static_cast<_float>(x) * desc.vertexSpacing,
				0.f,
				static_cast<_float>(z) * desc.vertexSpacing };
			vertex.normal = { 0.f, 1.f, 0.f };
			vertex.texCoord = {
				static_cast<_float>(x) / static_cast<_float>(m_iVertexCountX - 1),
				static_cast<_float>(z) / static_cast<_float>(m_iVertexCountZ - 1) };
		}
	}

	BuildGridIndices();
	RecalculateBounds();
	return S_OK;
}

void CTerrain::BuildGridIndices()
{
	m_Indices.clear();
	m_Indices.reserve(static_cast<size_t>(m_iVertexCountX - 1) * (m_iVertexCountZ - 1) * 6);
	for (uint32_t z = 0; z + 1 < m_iVertexCountZ; ++z)
	{
		for (uint32_t x = 0; x + 1 < m_iVertexCountX; ++x)
		{
			const uint32_t topLeft = z * m_iVertexCountX + x;
			const uint32_t bottomLeft = topLeft + m_iVertexCountX;
			m_Indices.insert(m_Indices.end(),
				{ bottomLeft, bottomLeft + 1, topLeft + 1,
				  bottomLeft, topLeft + 1, topLeft });
		}
	}
}

HRESULT CTerrain::BuildChunks(uint32_t chunkQuadCount, _float vertexSpacing, uint32_t maskResolution)
{
	m_Chunks.clear();
	m_iChunkQuadCount = chunkQuadCount;
	m_iMaskResolution = maskResolution;
	m_fVertexSpacing = vertexSpacing;
	const uint32_t totalQuadsX = m_iVertexCountX - 1;
	const uint32_t totalQuadsZ = m_iVertexCountZ - 1;
	const uint32_t chunkCountX = (totalQuadsX + chunkQuadCount - 1) / chunkQuadCount;
	const uint32_t chunkCountZ = (totalQuadsZ + chunkQuadCount - 1) / chunkQuadCount;

	for (uint32_t chunkZ = 0; chunkZ < chunkCountZ; ++chunkZ)
	{
		for (uint32_t chunkX = 0; chunkX < chunkCountX; ++chunkX)
		{
			auto chunk = CreateChunk(chunkX, chunkZ);
			if (!chunk)
				return E_FAIL;
			m_Chunks.push_back(std::move(chunk));
		}
	}
	return m_Chunks.empty() ? E_FAIL : S_OK;
}

UPtr<CTerrainChunk> CTerrain::CreateChunk(uint32_t chunkX, uint32_t chunkZ) const
{
	const uint32_t startX = chunkX * m_iChunkQuadCount;
	const uint32_t startZ = chunkZ * m_iChunkQuadCount;
	if (startX >= m_iVertexCountX - 1 || startZ >= m_iVertexCountZ - 1) return nullptr;
	const uint32_t quadCountX = std::min(m_iChunkQuadCount, m_iVertexCountX - 1 - startX);
	const uint32_t quadCountZ = std::min(m_iChunkQuadCount, m_iVertexCountZ - 1 - startZ);
	TERRAIN_CHUNK_DESC desc{};
	desc.coord = { static_cast<int64_t>(chunkX), static_cast<int64_t>(chunkZ) };
	desc.vertexCountX = quadCountX + 1;
	desc.vertexCountZ = quadCountZ + 1;
	desc.vertexSpacing = m_fVertexSpacing;
	desc.maskResolution = m_iMaskResolution;
	for (uint32_t localZ = 0; localZ < desc.vertexCountZ; ++localZ)
	{
		const size_t begin = static_cast<size_t>(startZ + localZ) * m_iVertexCountX + startX;
		desc.vertices.insert(desc.vertices.end(), m_Vertices.begin() + begin,
			m_Vertices.begin() + begin + desc.vertexCountX);
	}
	for (uint32_t z = 0; z < quadCountZ; ++z)
	{
		for (uint32_t x = 0; x < quadCountX; ++x)
		{
			const uint32_t topLeft = z * desc.vertexCountX + x;
			const uint32_t bottomLeft = topLeft + desc.vertexCountX;
			desc.indices.insert(desc.indices.end(), { bottomLeft, bottomLeft + 1, topLeft + 1,
				bottomLeft, topLeft + 1, topLeft });
		}
	}
	return CTerrainChunk::Create(desc);
}

void CTerrain::RecalculateNormals()
{
	for (auto& vertex : m_Vertices)
		vertex.normal = {};

	for (size_t index = 0; index + 2 < m_Indices.size(); index += 3)
	{
		auto& v0 = m_Vertices[m_Indices[index]];
		auto& v1 = m_Vertices[m_Indices[index + 1]];
		auto& v2 = m_Vertices[m_Indices[index + 2]];
		const _vector normal = XMVector3Normalize(XMVector3Cross(
			XMLoadFloat3(&v1.pos) - XMLoadFloat3(&v0.pos),
			XMLoadFloat3(&v2.pos) - XMLoadFloat3(&v0.pos)));
		AddNormal(v0, normal);
		AddNormal(v1, normal);
		AddNormal(v2, normal);
	}

	for (auto& vertex : m_Vertices)
		XMStoreFloat3(&vertex.normal, XMVector3Normalize(XMLoadFloat3(&vertex.normal)));
}

void CTerrain::RecalculateNormals(uint32_t minX, uint32_t minZ, uint32_t maxX, uint32_t maxZ)
{
	for (uint32_t z = minZ; z <= maxZ; ++z)
	{
		for (uint32_t x = minX; x <= maxX; ++x)
			m_Vertices[static_cast<size_t>(z) * m_iVertexCountX + x].normal = {};
	}

	const uint32_t quadMinX = minX > 0 ? minX - 1 : 0;
	const uint32_t quadMinZ = minZ > 0 ? minZ - 1 : 0;
	const uint32_t quadMaxX = std::min(maxX, m_iVertexCountX - 2);
	const uint32_t quadMaxZ = std::min(maxZ, m_iVertexCountZ - 2);
	auto addIfAffected = [&](uint32_t vertexIndex, FXMVECTOR normal)
	{
		const uint32_t x = vertexIndex % m_iVertexCountX;
		const uint32_t z = vertexIndex / m_iVertexCountX;
		if (x >= minX && x <= maxX && z >= minZ && z <= maxZ)
			AddNormal(m_Vertices[vertexIndex], normal);
	};

	for (uint32_t z = quadMinZ; z <= quadMaxZ; ++z)
	{
		for (uint32_t x = quadMinX; x <= quadMaxX; ++x)
		{
			const uint32_t topLeft = z * m_iVertexCountX + x;
			const uint32_t triangles[6]
			{
				topLeft + m_iVertexCountX, topLeft + m_iVertexCountX + 1, topLeft + 1,
				topLeft + m_iVertexCountX, topLeft + 1, topLeft
			};
			for (uint32_t triangle = 0; triangle < 2; ++triangle)
			{
				const uint32_t i0 = triangles[triangle * 3];
				const uint32_t i1 = triangles[triangle * 3 + 1];
				const uint32_t i2 = triangles[triangle * 3 + 2];
				const _vector normal = XMVector3Normalize(XMVector3Cross(
					XMLoadFloat3(&m_Vertices[i1].pos) - XMLoadFloat3(&m_Vertices[i0].pos),
					XMLoadFloat3(&m_Vertices[i2].pos) - XMLoadFloat3(&m_Vertices[i0].pos)));
				addIfAffected(i0, normal);
				addIfAffected(i1, normal);
				addIfAffected(i2, normal);
			}
		}
	}

	for (uint32_t z = minZ; z <= maxZ; ++z)
	{
		for (uint32_t x = minX; x <= maxX; ++x)
		{
			auto& normal = m_Vertices[static_cast<size_t>(z) * m_iVertexCountX + x].normal;
			XMStoreFloat3(&normal, XMVector3Normalize(XMLoadFloat3(&normal)));
		}
	}
}

void CTerrain::RecalculateBounds()
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

	m_LocalBounds = BoundingBox{
		{ (minPosition.x + maxPosition.x) * 0.5f,
		  (minPosition.y + maxPosition.y) * 0.5f,
		  (minPosition.z + maxPosition.z) * 0.5f },
		{ (maxPosition.x - minPosition.x) * 0.5f,
		  (maxPosition.y - minPosition.y) * 0.5f,
		  (maxPosition.z - minPosition.z) * 0.5f } };
}

void CTerrain::UpdateChunkVisibility()
{
	m_VisibleChunks.clear();
	m_VisibleChunks.reserve(m_Chunks.size());

	auto* camera = CGameInstance::Get().GetActiveCamera();
	if (!camera)
	{
		for (const auto& chunk : m_Chunks)
			m_VisibleChunks.push_back(chunk.get());
		return;
	}

	BoundingFrustum viewFrustum{ camera->GetProj() };
	BoundingFrustum worldFrustum{};
	const _matrix inverseView = XMMatrixInverse(nullptr, camera->GetView());
	viewFrustum.Transform(worldFrustum, inverseView);

	const _matrix terrainWorld = GetTransform().GetLoadedCombinedWorldMatrix();
	for (const auto& chunk : m_Chunks)
	{
		BoundingBox worldBounds{};
		chunk->GetLocalBounds().Transform(worldBounds, terrainWorld);
		if (worldFrustum.Intersects(worldBounds))
			m_VisibleChunks.push_back(chunk.get());
	}
}

HRESULT CTerrain::UpdateChunks(uint32_t minX, uint32_t minZ, uint32_t maxX, uint32_t maxZ)
{
	for (const auto& chunk : m_Chunks)
	{
		const auto& coord = chunk->GetCoord();
		const uint32_t chunkStartX = static_cast<uint32_t>(coord.x) * m_iChunkQuadCount;
		const uint32_t chunkStartZ = static_cast<uint32_t>(coord.z) * m_iChunkQuadCount;
		const uint32_t chunkEndX = chunkStartX + chunk->GetVertexCountX() - 1;
		const uint32_t chunkEndZ = chunkStartZ + chunk->GetVertexCountZ() - 1;
		if (maxX < chunkStartX || minX > chunkEndX || maxZ < chunkStartZ || minZ > chunkEndZ)
			continue;

		auto vertices = chunk->GetVertices();
		for (uint32_t localZ = 0; localZ < chunk->GetVertexCountZ(); ++localZ)
		{
			for (uint32_t localX = 0; localX < chunk->GetVertexCountX(); ++localX)
			{
				const uint32_t globalX = chunkStartX + localX;
				const uint32_t globalZ = chunkStartZ + localZ;
				vertices[static_cast<size_t>(localZ) * chunk->GetVertexCountX() + localX] =
					m_Vertices[static_cast<size_t>(globalZ) * m_iVertexCountX + globalX];
			}
		}
		if (FAILED(chunk->UpdateVertices(vertices)))
			return E_FAIL;
	}
	return S_OK;
}

UPtr<CTerrain> CTerrain::Create()
{
	auto instance = ToUPtr(new CTerrain{});
	if (FAILED(instance->InitializePrototype()))
		return nullptr;
	return instance;
}

UPtr<CPrototype> CTerrain::Clone(void* pArg)
{
	auto instance = ToUPtr(new CTerrain{ *this });
	if (FAILED(instance->Initialize(pArg)))
		return nullptr;
	return instance;
}
