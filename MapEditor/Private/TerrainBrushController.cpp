#include "pch.h"
#include "TerrainBrushController.h"

#include "DbgLineRender.h"
#include "GameInstance.h"
#include "Terrain.h"

NS_USING(Client)

HRESULT CTerrainBrushController::UpdateStroke(E::CTerrain& terrain, const E::_float3& worldHit, float timeDelta)
{
	if (timeDelta <= 0.f || m_Settings.radius <= 0.f || m_Settings.strength <= 0.f)
		return S_FALSE;

	std::vector<E::_float3> stamps{};
	if (!m_PreviousHit)
	{
		stamps.push_back(worldHit);
	}
	else
	{
		const float dx = worldHit.x - m_PreviousHit->x;
		const float dz = worldHit.z - m_PreviousHit->z;
		const float distance = std::sqrt(dx * dx + dz * dz);
		const float spacing = std::max(m_Settings.radius * 0.2f, terrain.GetVertexSpacing());
		const uint32_t sampleCount = std::max(1u, static_cast<uint32_t>(std::ceil(distance / spacing)));
		stamps.reserve(sampleCount);
		for (uint32_t sample = 1; sample <= sampleCount; ++sample)
		{
			const float t = static_cast<float>(sample) / static_cast<float>(sampleCount);
			stamps.push_back({
				std::lerp(m_PreviousHit->x, worldHit.x, t),
				std::lerp(m_PreviousHit->y, worldHit.y, t),
				std::lerp(m_PreviousHit->z, worldHit.z, t) });
		}
	}
	m_PreviousHit = worldHit;

	const float direction = m_Settings.mode == ETerrainBrushMode::Lower ? -1.f : 1.f;
	const float heightDelta = direction * m_Settings.strength * timeDelta / static_cast<float>(stamps.size());

	uint32_t minX = std::numeric_limits<uint32_t>::max();
	uint32_t minZ = std::numeric_limits<uint32_t>::max();
	uint32_t maxX = 0;
	uint32_t maxZ = 0;
	bool changed = false;

	for (const auto& stamp : stamps)
		changed |= ApplyStamp(terrain, stamp, heightDelta, minX, minZ, maxX, maxZ);

	if (!changed)
		return S_FALSE;

	return terrain.CommitHeightRegion(minX, minZ, maxX, maxZ);
}

HRESULT CTerrainBrushController::UpdateTextureStroke(
	E::CTerrain& terrain, const E::_float3& worldHit, float timeDelta)
{
	if (timeDelta <= 0.f || m_Settings.radius <= 0.f || m_Settings.strength <= 0.f)
		return S_FALSE;

	const E::_matrix inverseWorld = XMMatrixInverse(nullptr, terrain.GetTransform().GetLoadedCombinedWorldMatrix());

	E::_float3 localHit{};
	XMStoreFloat3(&localHit, XMVector3TransformCoord(XMLoadFloat3(&worldHit), inverseWorld));

	const auto& scale = terrain.GetTransform().GetScale();
	const E::_float2 localRadius{
		m_Settings.radius / std::max(std::abs(scale.x), 0.0001f),
		m_Settings.radius / std::max(std::abs(scale.z), 0.0001f) };

	return terrain.PaintTileLocal({ localHit.x, localHit.z }, localRadius,
		m_Settings.tileLayer, std::clamp(m_Settings.strength * timeDelta, 0.f, 1.f),
		m_Settings.falloff);
}

void CTerrainBrushController::EndStroke()
{
	m_PreviousHit.reset();
	m_FlattenTargetHeight.reset();
}

bool CTerrainBrushController::ApplyStamp(E::CTerrain& terrain, const E::_float3& worldCenter, float heightDelta, uint32_t& minX, uint32_t& minZ, uint32_t& maxX, uint32_t& maxZ)
{
	const E::_matrix world = terrain.GetTransform().GetLoadedCombinedWorldMatrix();
	const E::_matrix inverseWorld = XMMatrixInverse(nullptr, world);

	E::_float3 localCenter{};
	XMStoreFloat3(&localCenter, XMVector3TransformCoord(XMLoadFloat3(&worldCenter), inverseWorld));
	const auto& scale = terrain.GetTransform().GetScale();
	const float scaleX = std::max(std::abs(scale.x), 0.0001f);
	const float scaleZ = std::max(std::abs(scale.z), 0.0001f);
	const float scaleY = std::max(std::abs(scale.y), 0.0001f);
	const float localRadiusX = m_Settings.radius / scaleX;
	const float localRadiusZ = m_Settings.radius / scaleZ;
	const float spacing = terrain.GetVertexSpacing();

	if (m_Settings.mode == ETerrainBrushMode::Flatten && !m_FlattenTargetHeight)
		m_FlattenTargetHeight = localCenter.y;

	const int32_t startX = std::max(0, static_cast<int32_t>(std::floor((localCenter.x - localRadiusX) / spacing)));
	const int32_t startZ = std::max(0, static_cast<int32_t>(std::floor((localCenter.z - localRadiusZ) / spacing)));
	const int32_t endX = std::min(static_cast<int32_t>(terrain.GetVertexCountX()) - 1,
		static_cast<int32_t>(std::ceil((localCenter.x + localRadiusX) / spacing)));
	const int32_t endZ = std::min(static_cast<int32_t>(terrain.GetVertexCountZ()) - 1,
		static_cast<int32_t>(std::ceil((localCenter.z + localRadiusZ) / spacing)));

	if (startX > endX || startZ > endZ)
		return false;

	struct HeightUpdate { uint32_t x; uint32_t z; float height; };
	std::vector<HeightUpdate> updates{};
	updates.reserve(static_cast<size_t>(endX - startX + 1) * (endZ - startZ + 1));

	for (int32_t z = startZ; z <= endZ; ++z)
	{
		for (int32_t x = startX; x <= endX; ++x)
		{
			E::_float3 localVertex{
				static_cast<float>(x) * spacing,
				terrain.GetVertexHeight(x, z),
				static_cast<float>(z) * spacing };

			E::_float3 worldVertex{};
			XMStoreFloat3(&worldVertex, XMVector3TransformCoord(XMLoadFloat3(&localVertex), world));
			const float dx = worldVertex.x - worldCenter.x;
			const float dz = worldVertex.z - worldCenter.z;
			const float distance = std::sqrt(dx * dx + dz * dz);

			if (distance > m_Settings.radius)
				continue;

			const float normalized = std::clamp(distance / m_Settings.radius, 0.f, 1.f);
			const float weight = std::pow(1.f - normalized, std::max(m_Settings.falloff, 0.01f));
			const uint32_t ux = static_cast<uint32_t>(x);
			const uint32_t uz = static_cast<uint32_t>(z);
			const float currentHeight = terrain.GetVertexHeight(ux, uz);
			float nextHeight = currentHeight;
			switch (m_Settings.mode)
			{
				case ETerrainBrushMode::Raise:
				case ETerrainBrushMode::Lower:
					nextHeight += heightDelta * weight / scaleY;
					break;
				case ETerrainBrushMode::Flatten:
					nextHeight = std::lerp(currentHeight, *m_FlattenTargetHeight,
						std::clamp(std::abs(heightDelta) * weight, 0.f, 1.f));
					break;
				case ETerrainBrushMode::Smooth:
				{
					float sum = 0.f;
					uint32_t count = 0;
					for (int32_t oz = -1; oz <= 1; ++oz)
						for (int32_t ox = -1; ox <= 1; ++ox)
						{
							const int32_t nx = x + ox, nz = z + oz;
							if (nx < 0 || nz < 0 || nx >= static_cast<int32_t>(terrain.GetVertexCountX()) ||
								nz >= static_cast<int32_t>(terrain.GetVertexCountZ())) 
								continue;

							sum += terrain.GetVertexHeight(static_cast<uint32_t>(nx), static_cast<uint32_t>(nz));
							++count;
						}
					const float average = count ? sum / static_cast<float>(count) : currentHeight;
					nextHeight = std::lerp(currentHeight, average, std::clamp(std::abs(heightDelta) * weight, 0.f, 1.f));
					break;
				}
				case ETerrainBrushMode::Noise:
				{
					uint32_t hash = ux * 374761393u + uz * 668265263u;
					hash = (hash ^ (hash >> 13u)) * 1274126177u;
					const float noise = static_cast<float>(hash & 0xffffu) / 32767.5f - 1.f;
					nextHeight += noise * std::abs(heightDelta) * weight / scaleY;
					break;
				}
			}

			if (std::abs(nextHeight - currentHeight) <= 0.000001f) 
				continue;

			updates.push_back({ ux, uz, nextHeight });
			minX = std::min(minX, ux);
			minZ = std::min(minZ, uz);
			maxX = std::max(maxX, ux);
			maxZ = std::max(maxZ, uz);
		}
	}
	for (const auto& update : updates)
		terrain.SetVertexHeight(update.x, update.z, update.height);

	return !updates.empty();
}

void CTerrainBrushController::DrawPreview(
	const E::CTerrain& terrain, const E::_float3& worldHit) const
{
	auto* debugDraw = E::CGameInstance::Get().GetDbgLineRender();
	if (!debugDraw)
		return;

	debugDraw->SetColor({ 0.1f, 0.9f, 0.2f, 1.f });
	constexpr uint32_t segmentCount = 64;
	constexpr float heightOffset = 0.08f;
	E::_float3 previous{};
	for (uint32_t segment = 0; segment <= segmentCount; ++segment)
	{
		const float angle = XM_2PI * static_cast<float>(segment) / static_cast<float>(segmentCount);
		E::_float3 point{
			worldHit.x + std::cos(angle) * m_Settings.radius,
			worldHit.y + heightOffset,
			worldHit.z + std::sin(angle) * m_Settings.radius };

		if (segment > 0)
			debugDraw->AddLine(previous, point);

		previous = point;
	}
}

E::UPtr<CTerrainBrushController> CTerrainBrushController::Create()
{
	return E::ToUPtr(new CTerrainBrushController{});
}
