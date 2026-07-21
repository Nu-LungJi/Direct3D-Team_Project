#include "pch.h"
#include "TerrainEditCommand.h"

#include "Terrain.h"

NS_USING(Client)

CTerrainEditCommand::CTerrainEditCommand(E::CTerrain* terrain) : m_pTerrain{ terrain } {}

void CTerrainEditCommand::CaptureBefore(const E::_float3& worldCenter, float radius)
{
	if (!m_pTerrain || radius <= 0.f) return;
	const E::_matrix inverseWorld = XMMatrixInverse(nullptr,
		m_pTerrain->GetTransform().GetLoadedCombinedWorldMatrix());
	E::_float3 local{};
	XMStoreFloat3(&local, XMVector3TransformCoord(XMLoadFloat3(&worldCenter), inverseWorld));
	const auto& scale = m_pTerrain->GetTransform().GetScale();
	const float radiusX = radius / std::max(std::abs(scale.x), 0.0001f);
	const float radiusZ = radius / std::max(std::abs(scale.z), 0.0001f);
	const float spacing = m_pTerrain->GetVertexSpacing();
	const int32_t minX = std::max(0, static_cast<int32_t>(std::floor((local.x - radiusX) / spacing)));
	const int32_t minZ = std::max(0, static_cast<int32_t>(std::floor((local.z - radiusZ) / spacing)));
	const int32_t maxX = std::min(static_cast<int32_t>(m_pTerrain->GetVertexCountX()) - 1,
		static_cast<int32_t>(std::ceil((local.x + radiusX) / spacing)));
	const int32_t maxZ = std::min(static_cast<int32_t>(m_pTerrain->GetVertexCountZ()) - 1,
		static_cast<int32_t>(std::ceil((local.z + radiusZ) / spacing)));
	for (int32_t z = minZ; z <= maxZ; ++z)
		for (int32_t x = minX; x <= maxX; ++x)
		{
			const uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) |
				static_cast<uint32_t>(z);
			m_HeightBefore.try_emplace(key, m_pTerrain->GetVertexHeight(x, z));
		}
	const float chunkSize = m_pTerrain->GetChunkQuadCount() * spacing;
	for (const auto& chunk : m_pTerrain->GetChunks())
	{
		const auto& coord = chunk->GetCoord();
		const float startX = static_cast<float>(coord.x) * chunkSize;
		const float startZ = static_cast<float>(coord.z) * chunkSize;
		const float endX = startX + (chunk->GetVertexCountX() - 1) * spacing;
		const float endZ = startZ + (chunk->GetVertexCountZ() - 1) * spacing;
		if (local.x + radiusX < startX || local.x - radiusX > endX ||
			local.z + radiusZ < startZ || local.z - radiusZ > endZ) continue;
		const uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(coord.x)) << 32) |
			static_cast<uint32_t>(coord.z);
		m_MaskBefore.try_emplace(key, chunk->GetBlendMask());
	}
}

bool CTerrainEditCommand::Finalize()
{
	if (!m_pTerrain) return false;
	for (const auto& [key, before] : m_HeightBefore)
	{
		const uint32_t x = static_cast<uint32_t>(key >> 32);
		const uint32_t z = static_cast<uint32_t>(key);
		const float after = m_pTerrain->GetVertexHeight(x, z);
		if (before != after) m_HeightChanges.push_back({ x, z, before, after });
	}
	for (const auto& [key, before] : m_MaskBefore)
	{
		const int64_t x = static_cast<int32_t>(key >> 32);
		const int64_t z = static_cast<int32_t>(key);
		for (const auto& chunk : m_pTerrain->GetChunks())
		{
			if (chunk->GetCoord().x == x && chunk->GetCoord().z == z && before != chunk->GetBlendMask())
				m_MaskChanges.push_back({ x, z, before, chunk->GetBlendMask() });
		}
	}
	m_HeightBefore.clear();
	m_MaskBefore.clear();
	return !m_HeightChanges.empty() || !m_MaskChanges.empty();
}

_bool CTerrainEditCommand::Apply(bool after)
{
	if (!m_pTerrain) return false;
	uint32_t minX = UINT32_MAX, minZ = UINT32_MAX, maxX = 0, maxZ = 0;
	for (const auto& change : m_HeightChanges)
	{
		m_pTerrain->SetVertexHeight(change.x, change.z, after ? change.after : change.before);
		minX = std::min(minX, change.x); minZ = std::min(minZ, change.z);
		maxX = std::max(maxX, change.x); maxZ = std::max(maxZ, change.z);
	}
	if (!m_HeightChanges.empty() && FAILED(m_pTerrain->CommitHeightRegion(minX, minZ, maxX, maxZ))) return false;
	for (const auto& change : m_MaskChanges)
	{
		for (const auto& chunk : m_pTerrain->GetChunks())
			if (chunk->GetCoord().x == change.x && chunk->GetCoord().z == change.z &&
				FAILED(chunk->SetBlendMask(after ? change.after : change.before))) return false;
	}
	return true;
}

_bool CTerrainEditCommand::Execute() { return Apply(true); }
_bool CTerrainEditCommand::Undo() { return Apply(false); }
