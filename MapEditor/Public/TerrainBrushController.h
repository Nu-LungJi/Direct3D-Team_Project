#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CTerrain;
NS_END

NS_BEGIN(Client)

enum class ETerrainBrushMode : uint8_t
{
	Raise,
	Lower,
	Flatten,
	Smooth,
	Noise
};

struct TERRAIN_BRUSH_SETTINGS
{
	ETerrainBrushMode mode = ETerrainBrushMode::Raise;
	float radius = 5.f;
	float strength = 5.f;
	float falloff = 2.f;
	uint32_t tileLayer = 0;
};

// Picking된 Terrain 월드 좌표를 기준으로 높이 Sculpt와 Texture Paint를 처리
//
// 연속된 마우스 이동을 여러 Brush Stamp로 보간하며,
// 변경된 정점 범위만 Terrain에 Commit해 Normal과 Chunk Buffer를 갱신

class CTerrainBrushController final : public E::CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CTerrainBrushController, E::CEngineBase)

private:
	CTerrainBrushController() = default;
	~CTerrainBrushController() override = default;

public:
	TERRAIN_BRUSH_SETTINGS& GetSettings() { return m_Settings; }
	const TERRAIN_BRUSH_SETTINGS& GetSettings() const { return m_Settings; }
	HRESULT UpdateStroke(E::CTerrain& terrain, const E::_float3& worldHit, float timeDelta);
	HRESULT UpdateTextureStroke(E::CTerrain& terrain, const E::_float3& worldHit, float timeDelta);
	void EndStroke();
	void DrawPreview(const E::CTerrain& terrain, const E::_float3& worldHit) const;
	static E::UPtr<CTerrainBrushController> Create();

private:
	bool ApplyStamp(E::CTerrain& terrain, const E::_float3& worldCenter, float heightDelta,
		uint32_t& minX, uint32_t& minZ, uint32_t& maxX, uint32_t& maxZ);

private:
	TERRAIN_BRUSH_SETTINGS m_Settings{};
	std::optional<E::_float3> m_PreviousHit{};
	std::optional<float> m_FlattenTargetHeight{};
};

NS_END
