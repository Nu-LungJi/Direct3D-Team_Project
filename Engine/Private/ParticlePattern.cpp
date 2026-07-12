#include "pch.h"
#include "ParticlePattern.h"
NS_USING(Engine)

std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeStairs(
    const _float3& vStartPos,
    uint32_t iStepCount,
    _float fStepWidth,
    _float fStepHeight,
    _float fStepDepth,
    _float fLife,
    const _float4& color,
    const _float4& emissive)
{
    std::vector<PARTICLE_SPAWN_DATA> spawnList(iStepCount);
    for (uint32_t i = 0; i < iStepCount; ++i)
    {
        PARTICLE_SPAWN_DATA& s = spawnList[i];
        s.position = _float3(
            vStartPos.x,
            vStartPos.y + fStepHeight * i,
            vStartPos.z + fStepDepth * i
        );
        s.velocity = _float3(0.f, 0.f, 0.f);
        s.life = fLife;
        s.fSize = fStepWidth;
        s.color = color;
        s.emissive = emissive;
    }
    return spawnList;
}

std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeCircle(
    const _float3& vCenter,
    _float fRadius,
    uint32_t iCount,
    _float fSize,
    _float fLife,
    const _float4& color,
    const _float4& emissive,
    _float fYOffset)
{
    std::vector<PARTICLE_SPAWN_DATA> spawnList(iCount);
    if (iCount == 0)
        return spawnList;

    const _float fAngleStep = XM_2PI / (_float)iCount;
    for (uint32_t i = 0; i < iCount; ++i)
    {
        PARTICLE_SPAWN_DATA& s = spawnList[i];
        _float fAngle = fAngleStep * (_float)i;
        s.position = _float3(
            vCenter.x + cosf(fAngle) * fRadius,
            vCenter.y + fYOffset,
            vCenter.z + sinf(fAngle) * fRadius
        );
        s.velocity = _float3(0.f, 0.f, 0.f);
        s.life = fLife;
        s.fSize = fSize;
        s.color = color;
        s.emissive = emissive;
    }
    return spawnList;
}

std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeGrid(
	const _float3& vOrigin,
	uint32_t iRows,
	uint32_t iCols,
	_float fSpacing,
	_float fSize,
	_float fLife,
	const _float4& color,
	const _float4& emissive)
{
	std::vector<PARTICLE_SPAWN_DATA> spawnList(iRows * iCols);
	for (uint32_t r = 0; r < iRows; ++r)
	{
		for (uint32_t c = 0; c < iCols; ++c)
		{
			PARTICLE_SPAWN_DATA& s = spawnList[r * iCols + c];
			s.position = _float3(
				vOrigin.x + fSpacing * (_float)c,
				vOrigin.y,
				vOrigin.z + fSpacing * (_float)r
			);
			s.velocity = _float3(0.f, 0.f, 0.f);
			s.life = fLife;
			s.fSize = fSize;
			s.color = color;
			s.emissive = emissive;
		}
	}
	return spawnList;
}

std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeStrightGround(const _float3& vStartPos, uint32_t row, uint32_t col, _float offSetX, _float offsetZ, 
	_float spawnDelay, _float fSize, _float fLife, const _float4& color, const _float4& emissive)
{
	std::vector<PARTICLE_SPAWN_DATA> spawnList(row * col);
	_float center = (_float)(col - 1) * 0.5f; // 짝수/홀수 모두 대칭

	for (uint32_t r = 0; r < row; ++r)
	{
		for (uint32_t c = 0; c < col; ++c)
		{
			PARTICLE_SPAWN_DATA& s = spawnList[r * col + c];

			_float dx = (_float)c - center;
			s.position = _float3(
				vStartPos.x + offSetX * dx,
				vStartPos.y,
				vStartPos.z + offsetZ * (_float)r
			);

			s.velocity = _float3(Randf(-2.f,2.f), Randf(1.f, 3.f), Randf(0.f, 2.f));
			s.life = fLife;
			s.fSize = fSize;
			s.color = color;
			s.emissive = emissive;
			s.spawnDelay = spawnDelay * (_float)r;
		}
	}
	return spawnList;
}
