#include "pch.h"
#include "Particle.h"
#include "ParticlePattern.h"
NS_USING(Engine)

std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeStairs(const SStairsParam& param)
{
	std::vector<PARTICLE_SPAWN_DATA> spawnList(param.iStepCount);
	for (uint32_t i = 0; i < param.iStepCount; ++i)
	{
		PARTICLE_SPAWN_DATA& s = spawnList[i];
		s.position = _float3(
			param.vStartPos.x,
			param.vStartPos.y + param.fStepHeight * i,
			param.vStartPos.z + param.fStepDepth * i
		);
		s.velocity = _float3(0.f, 0.f, 0.f);
		s.life = param.fLife;
		s.fSize = _float3(param.fStepWidth, param.fStepWidth, param.fStepWidth);
		s.color = param.color;
		s.emissive = param.emissive;
	}
	return spawnList;
}

std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeCircle(const SCircleParam& param)
{
	std::vector<PARTICLE_SPAWN_DATA> spawnList(param.iCount);
	if (param.iCount == 0)
		return spawnList;

	const _float fAngleStep = XM_2PI / (_float)param.iCount;
	for (uint32_t i = 0; i < param.iCount; ++i)
	{
		PARTICLE_SPAWN_DATA& s = spawnList[i];
		_float fAngle = fAngleStep * (_float)i;
		s.position = _float3(
			param.vCenter.x + cosf(fAngle) * param.fRadius,
			param.vCenter.y + param.fYOffset,
			param.vCenter.z + sinf(fAngle) * param.fRadius
		);
		s.velocity = _float3(0.f, 0.f, 0.f);
		s.life = param.fLife;
		s.fSize = param.fSize;
		s.fEndSize = param.fEndSize;
		s.color = param.color;
		s.emissive = param.emissive;
		s.velocity = param.fVelocity;
		s.iBehaviorType = param.iBehaviorType;
		
		uint32_t degree = 360 / param.iCount;
	}
	return spawnList;
}
std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeCircleAndSpread(const SCircleSpreadParam& param)
{
	std::vector<PARTICLE_SPAWN_DATA> spawnList(param.iCount);
	if (param.iCount == 0)
		return spawnList;

	const _float fAngleStep = XM_2PI / (_float)param.iCount;
	for (uint32_t i = 0; i < param.iCount; ++i)
	{
		PARTICLE_SPAWN_DATA& s = spawnList[i];
		_float fAngle = fAngleStep * (_float)i;
		s.position = param.vCenter;
		s.velocity = _float3(
			cosf(fAngle) * param.fRadius,
			sinf(fAngle) * param.fRadius,
			0
		);
		s.life = param.fLife;
		s.fSize = param.fSize;
		s.fEndSize = param.fEndSize;
		s.color = param.color;
		s.emissive = param.emissive;
		s.endEmissive = param.endEmissive;
		s.iBehaviorType = param.iBehaviorType;
		s.originalEmissive = param.emissive;
		s.originalPosition = param.vCenter;
		uint32_t degree = 360 / param.iCount;
	}
	return spawnList;
}

std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeSpiral(const SSpiralParam& param)
{
	return std::vector<PARTICLE_SPAWN_DATA>();
}
std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeStraightGround(const SStraightGroundParam& param)
{
	
	std::vector<PARTICLE_SPAWN_DATA> spawnList(param.iRow * param.iCol);
	_float center = (_float)(param.iCol - 1) * 0.5f; // 짝수/홀수 모두 대칭

	for (uint32_t r = 0; r < param.iRow; ++r)
	{
		for (uint32_t c = 0; c < param.iCol; ++c)
		{
			PARTICLE_SPAWN_DATA& s = spawnList[r * param.iCol + c];

			_float dx = (_float)c - center;
			s.position = _float3(
				param.vStartPos.x + param.fOffsetX * dx,
				param.vStartPos.y,
				param.vStartPos.z + param.fOffsetZ * (_float)r
			);

			s.velocity = _float3(Randf(-2.f, 2.f), Randf(1.f, 3.f), Randf(0.f, 2.f));
			s.life = param.fLife;
			s.fSize = param.fSize;
			s.color = param.color;
			s.emissive = param.startEmissive;
			s.endEmissive = param.endEmissive;
			s.spawnDelay = param.fSpawnDelay * (_float)r;

			s.rotation = param.bRandomRot
				? _float4(XMConvertToRadians(Randf(param.vMinRot.x, param.vMaxRot.x)),
					XMConvertToRadians(Randf(param.vMinRot.y, param.vMaxRot.y)),
					XMConvertToRadians(Randf(param.vMinRot.z, param.vMaxRot.z)),
					1.f)
				: _float4(XMConvertToRadians(param.vRotation.x),
					XMConvertToRadians(param.vRotation.y),
					XMConvertToRadians(param.vRotation.z),
					1.f);
		}
	}
	return spawnList;
}

std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeSmoke(const SMOKE& param)
{

	std::vector<PARTICLE_SPAWN_DATA> spawnList(param.iCount * param.iArray);
	if (param.iCount == 0)
		return spawnList;

	for (uint32_t j = 0; j < param.iArray; ++j)
	{
		for (uint32_t i = 0; i < param.iCount; ++i)
		{

			PARTICLE_SPAWN_DATA& s = spawnList[j * param.iCount + i];
			_float fAngle = XM_2PI * (_float)i / (_float)param.iCount;

			fAngle += E::Randf(param.vRandAngle.x, param.vRandAngle.y);

			_float fSpeed = param.fSpeed * E::Randf(param.vRandSpeed.x, param.vRandSpeed.y);
			_float fY = param.vRot.y;
			s.position = param.vCenter;
			if (ETOUI(CParticle::BEHAVIOR_SMOKEGW) & param.iFlag)
			{
				_float iOffset = _float(j+1.f) *param.fRadius * E::Randf(param.vRandRaidus.x, param.vRandRaidus.y);
				_vector radial = XMVectorSet(cosf(fAngle), 0.f, sinf(fAngle), 0.f);
				XMStoreFloat3(&s.velocity, radial * fSpeed );
				fY += XMConvertToDegrees(atan2f(XMVectorGetX(radial), XMVectorGetZ(radial)));
	
			}
			else
			{
				_vector radial = XMVectorSet(cosf(fAngle), 0.f, sinf(fAngle), 0.f);
				XMStoreFloat3(&s.position, XMLoadFloat3(&param.vCenter) + radial * param.fRadius);

				s.velocity = _float3(
					cosf(fAngle) * fSpeed,
					0,
					sinf(fAngle) * fSpeed
				);
			}

			if (ETOUI(CParticle::BEHAVIOR_SMOKEJUMP) & param.iFlag)
			{
				XMStoreFloat3(&s.velocity , XMLoadFloat3(&s.velocity)+ XMVectorSet(0, E::Randf(param.vRandSpeed.x, param.vRandSpeed.y),0.f,0.f) );
			}
			

			s.rotation = _float4(XMConvertToRadians(param.vRot.x), XMConvertToRadians(fY), XMConvertToRadians(param.vRot.z) , 0);
			s.life = param.fLife * E::Randf(param.vRandLife.x, param.vRandLife.y);
			XMStoreFloat3(&s.fSize, XMLoadFloat3(&param.fSize) * E::Randf(param.vRandSize.x, param.vRandSize.y));
			s.position.y += param.fYOffset;
			s.fEndSize = param.fEndSize;
			s.color = param.color;
			s.color.w = param.color.w * E::Randf(param.vRandAlpha.x, param.vRandAlpha.y);
			s.iBehaviorType = param.iBehaviorType;
			s.originalPosition = param.vCenter;
			s.spawnDelay = param.fSPawnDelay * E::Randf(param.vRandSpawn.x, param.vRandSpawn.y);
		}
	}

	return spawnList;
}
std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeLightning(const SLightning& param)
{
	std::vector<PARTICLE_SPAWN_DATA> spawnList(param.iCount);

	for (uint32_t i = 0; i < param.iCount; ++i)
	{
		PARTICLE_SPAWN_DATA& s = spawnList[i];
		s.position = param.vCenter;
		s.life = param.fLife;
		s.fSize = param.fSize;
		s.color = param.color;

		if (param.bRandomVel) {
			s.velocity = _float3(E::Randf(param.fVelMin.x, param.fVelMax.x), E::Randf(param.fVelMin.y, param.fVelMax.y), E::Randf(param.fVelMin.z, param.fVelMax.z));
		}
		else {
			s.velocity = param.fVelocity;
		}

		if (param.bRandomSize) {
			s.fSize = _float3(E::Randf(param.fSizeMin.x, param.fSizeMax.x), E::Randf(param.fSizeMin.y, param.fSizeMax.y), E::Randf(param.fSizeMin.z, param.fSizeMax.z));
		}
		else {
			s.fSize = param.fSize;
		}
		s.fEndSize = param.fEndSize;

		s.emissive = _float4(param.emissive.x, param.emissive.y, param.emissive.z, param.startIntensity);
		s.rotation.z = atan2f(s.velocity.y, s.velocity.x) + XM_PIDIV2;
		s.endEmissive = _float4(param.endEmissive.x, param.endEmissive.y, param.endEmissive.z, param.endIntensity);
		s.iBehaviorType = param.iBehaviorType;
		s.originalEmissive = s.emissive;
		s.originalPosition = param.vCenter;
		uint32_t degree = 360 / param.iCount;
	}
	return spawnList;
}

//std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeStairs(
//    const _float3& vStartPos,
//    uint32_t iStepCount,
//    _float fStepWidth,
//    _float fStepHeight,
//    _float fStepDepth,
//    _float fLife,
//    const _float4& color,
//    const _float4& emissive)
//{
//    std::vector<PARTICLE_SPAWN_DATA> spawnList(iStepCount);
//    for (uint32_t i = 0; i < iStepCount; ++i)
//    {
//        PARTICLE_SPAWN_DATA& s = spawnList[i];
//        s.position = _float3(
//            vStartPos.x,
//            vStartPos.y + fStepHeight * i,
//            vStartPos.z + fStepDepth * i
//        );
//        s.velocity = _float3(0.f, 0.f, 0.f);
//        s.life = fLife;
//        s.fSize = fStepWidth;
//        s.color = color;
//        s.emissive = emissive;
//    }
//    return spawnList;
//}

//std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeCircle(
//    const _float3& vCenter,
//    _float fRadius,
//    uint32_t iCount,
//    _float fSize,
//    _float fLife,
//    const _float4& color,
//    const _float4& emissive,
//    _float fYOffset)
//{
//    std::vector<PARTICLE_SPAWN_DATA> spawnList(iCount);
//    if (iCount == 0)
//        return spawnList;
//
//    const _float fAngleStep = XM_2PI / (_float)iCount;
//    for (uint32_t i = 0; i < iCount; ++i)
//    {
//        PARTICLE_SPAWN_DATA& s = spawnList[i];
//        _float fAngle = fAngleStep * (_float)i;
//        s.position = _float3(
//            vCenter.x + cosf(fAngle) * fRadius,
//            vCenter.y + fYOffset,
//            vCenter.z + sinf(fAngle) * fRadius
//        );
//        s.velocity = _float3(0.f, 0.f, 0.f);
//        s.life = fLife;
//        s.fSize = fSize;
//        s.color = color;
//        s.emissive = emissive;
//    }
//    return spawnList;
//}
//
//std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeStrightGround(const _float3& vStartPos, uint32_t row, uint32_t col, _float offSetX, _float offsetZ, 
//	_float spawnDelay, _float fSize, _float fLife, const _float4& color, const _float4& emissive)
//{
//	std::vector<PARTICLE_SPAWN_DATA> spawnList(row * col);
//	_float center = (_float)(col - 1) * 0.5f; // 짝수/홀수 모두 대칭
//
//	for (uint32_t r = 0; r < row; ++r)
//	{
//		for (uint32_t c = 0; c < col; ++c)
//		{
//			PARTICLE_SPAWN_DATA& s = spawnList[r * col + c];
//
//			_float dx = (_float)c - center;
//			s.position = _float3(
//				vStartPos.x + offSetX * dx,
//				vStartPos.y,
//				vStartPos.z + offsetZ * (_float)r
//			);
//
//			s.velocity = _float3(Randf(-2.f,2.f), Randf(1.f, 3.f), Randf(0.f, 2.f));
//			s.life = fLife;
//			s.fSize = fSize;
//			s.color = color;
//			s.emissive = emissive;
//			s.spawnDelay = spawnDelay * (_float)r;
//		}
//	}
//	return spawnList;
//}
