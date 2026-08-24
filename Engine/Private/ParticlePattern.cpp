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
		const _float fAngle = fAngleStep * (_float)i;
		if (param.bStand) {
			s.position = _float3(
				param.vCenter.x + cosf(fAngle) * param.fRadius,
				param.vCenter.y + sinf(fAngle) * param.fRadius,
				param.vCenter.z
			);
			s.originalPosition = param.vCenter;
		}
		else {
			s.position = _float3(
				param.vCenter.x + cosf(fAngle) * param.fRadius,
				param.vCenter.y + param.fYOffset,
				param.vCenter.z + sinf(fAngle) * param.fRadius
			);
			s.originalPosition = _float3(param.vCenter.x, param.vCenter.y + param.fYOffset, param.vCenter.z);
		}
	

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
		s.rotation = param.bRandomRot
			? _float4(XMConvertToRadians(Randf(param.vMinRot.x, param.vMaxRot.x)),
				XMConvertToRadians(Randf(param.vMinRot.y, param.vMaxRot.y)),
				XMConvertToRadians(Randf(param.vMinRot.z, param.vMaxRot.z)),
				0.f)
			: _float4(XMConvertToRadians(param.vRotation.x),
				XMConvertToRadians(param.vRotation.y),
				XMConvertToRadians(param.vRotation.z),
				0.f);
		s.life = param.fLife;
		s.fEndSize = param.fEndSize;
		s.color = param.color;
		s.emissive = _float4(param.emissive.x, param.emissive.y, param.emissive.z, param.startIntensity);
		s.endEmissive = _float4(param.endEmissive.x, param.endEmissive.y, param.endEmissive.z, param.endIntensity);
		s.iBehaviorType = param.iBehaviorType;
		s.originalVelocity = s.velocity;
		s.rotationAxis = param.rotationAxis;
		s.fRotationSpeed = param.rotationSpeed;
	}
	return spawnList;
}
std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeCircleAndSpread(const SCircleSpreadParam& param)
{
	std::vector<PARTICLE_SPAWN_DATA> spawnList(param.iCount);
	if (param.iCount == 0)
		return spawnList;

	const _float spawnDuration = std::max(param.fSpawnDuration, 0.f);
	const _float moveDuration = std::max(param.fLife, 0.0001f);
	const _float radiusJitter = std::max(param.fRadiusJitter, 0.f);
	for (uint32_t i = 0; i < param.iCount; ++i)
	{
		PARTICLE_SPAWN_DATA& s = spawnList[i];
		const _float fAngle = Randf(0.f, XM_2PI);
		const _float radius = std::max(0.f, param.fRadius + Randf(-radiusJitter, radiusJitter));
		s.position = param.vCenter;
		s.velocity = _float3(
			cosf(fAngle) * radius / moveDuration,
			sinf(fAngle) * radius / moveDuration,
			0
		);
		s.life = param.fLife;
		s.fSize = param.fSize;
		s.fEndSize = param.fEndSize;
		s.color = param.color;
		s.emissive = _float4(param.emissive.x, param.emissive.y, param.emissive.z,param.startIntensity);
		s.endEmissive = _float4(param.endEmissive.x, param.endEmissive.y, param.endEmissive.z,param.endIntensity);
		s.iBehaviorType = param.iBehaviorType;
		s.originalEmissive = param.emissive;
		s.originalPosition = param.vCenter;
		s.originalVelocity = s.velocity;
		s.spawnDelay = param.fSpawnDelay + Randf(0.f, spawnDuration);
	}
	return spawnList;
}

std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeCircleToCenter(const SCircleToCenterParam& param)
{
	std::vector<PARTICLE_SPAWN_DATA> spawnList(param.iCount);
	if (param.iCount == 0)
		return spawnList;

	const _float moveDuration = std::max(param.fLife, 0.0001f);
	const _float spawnDuration = std::max(param.fSpawnDuration, 0.f);
	const _float spawnInterval = param.iCount > 1 ? spawnDuration / static_cast<_float>(param.iCount - 1) : 0.f;
	const _float radiusJitter = std::max(param.fRadiusJitter, 0.f);
	const _float3 targetCenter = param.bStand ? param.vCenter : _float3(param.vCenter.x, param.vCenter.y + param.fYOffset, param.vCenter.z);

	for (uint32_t i = 0; i < param.iCount; ++i)
	{
		PARTICLE_SPAWN_DATA& spawn = spawnList[i];
		const _float angle = Randf(0.f, XM_2PI);
		const _float radius = std::max(0.f, param.fRadius + Randf(-radiusJitter, radiusJitter));

		if (param.bStand)
			spawn.position = _float3(param.vCenter.x + cosf(angle) * radius, param.vCenter.y + sinf(angle) * radius, param.vCenter.z);
		else
			spawn.position = _float3(param.vCenter.x + cosf(angle) * radius, param.vCenter.y + param.fYOffset, param.vCenter.z + sinf(angle) * radius);

		const _vector startPosition = XMLoadFloat3(&spawn.position);
		const _vector centerPosition = XMLoadFloat3(&targetCenter);
		XMStoreFloat3(&spawn.velocity, (centerPosition - startPosition) / moveDuration);

		spawn.life = moveDuration;
		spawn.fSize = param.bRandomSize ? _float3(Randf(param.fSizeMin.x, param.fSizeMax.x), Randf(param.fSizeMin.y, param.fSizeMax.y), Randf(param.fSizeMin.z, param.fSizeMax.z)) : param.fSize;
		spawn.fEndSize = param.fEndSize;
		spawn.color = param.color;
		spawn.emissive = _float4(param.emissive.x, param.emissive.y, param.emissive.z, param.startIntensity);
		spawn.endEmissive = _float4(param.endEmissive.x, param.endEmissive.y, param.endEmissive.z, param.endIntensity);
		spawn.originalEmissive = spawn.emissive;
		spawn.iBehaviorType = param.iBehaviorType;
		spawn.spawnDelay = param.fSpawnDelay + static_cast<_float>(i) * spawnInterval;
		spawn.originalPosition = spawn.position;
		spawn.originalVelocity = spawn.velocity;
		spawn.rotationAxis = param.rotationAxis;
		spawn.fRotationSpeed = param.rotationSpeed;
		spawn.rotation = param.bRandomRot ? _float4(XMConvertToRadians(Randf(param.vMinRot.x, param.vMaxRot.x)), XMConvertToRadians(Randf(param.vMinRot.y, param.vMaxRot.y)), XMConvertToRadians(Randf(param.vMinRot.z, param.vMaxRot.z)), 0.f) : _float4(XMConvertToRadians(param.vRotation.x), XMConvertToRadians(param.vRotation.y), XMConvertToRadians(param.vRotation.z), 0.f);
	}

	return spawnList;
}

std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeCenterToCircle(const SCenterToCircleParam& param)
{
	std::vector<PARTICLE_SPAWN_DATA> spawnList(param.iCount);
	if (param.iCount == 0)
		return spawnList;

	const _float moveDuration = std::max(param.fLife, 0.0001f);
	const _float spawnDuration = std::max(param.fSpawnDuration, 0.f);
	const _float radiusJitter = std::max(param.fRadiusJitter, 0.f);
	const _float3 startCenter = param.bStand ? param.vCenter : _float3(param.vCenter.x, param.vCenter.y + param.fYOffset, param.vCenter.z);

	for (uint32_t i = 0; i < param.iCount; ++i)
	{
		PARTICLE_SPAWN_DATA& spawn = spawnList[i];
		const _float angle = Randf(0.f, XM_2PI);
		const _float radius = std::max(0.f, param.fRadius + Randf(-radiusJitter, radiusJitter));
		const _float3 targetPosition = param.bStand
			? _float3(param.vCenter.x + cosf(angle) * radius, param.vCenter.y + sinf(angle) * radius, param.vCenter.z)
			: _float3(param.vCenter.x + cosf(angle) * radius, param.vCenter.y + param.fYOffset, param.vCenter.z + sinf(angle) * radius);

		spawn.position = startCenter;
		XMStoreFloat3(&spawn.velocity, (XMLoadFloat3(&targetPosition) - XMLoadFloat3(&startCenter)) / moveDuration);

		spawn.life = moveDuration;
		spawn.fSize = param.bRandomSize ? _float3(Randf(param.fSizeMin.x, param.fSizeMax.x), Randf(param.fSizeMin.y, param.fSizeMax.y), Randf(param.fSizeMin.z, param.fSizeMax.z)) : param.fSize;
		spawn.fEndSize = param.fEndSize;
		spawn.color = param.color;
		spawn.emissive = _float4(param.emissive.x, param.emissive.y, param.emissive.z, param.startIntensity);
		spawn.endEmissive = _float4(param.endEmissive.x, param.endEmissive.y, param.endEmissive.z, param.endIntensity);
		spawn.originalEmissive = spawn.emissive;
		spawn.iBehaviorType = param.iBehaviorType;
		spawn.spawnDelay = param.fSpawnDelay + Randf(0.f, spawnDuration);
		spawn.originalPosition = spawn.position;
		spawn.originalVelocity = spawn.velocity;
		spawn.rotationAxis = param.rotationAxis;
		spawn.fRotationSpeed = param.rotationSpeed;
		spawn.rotation = param.bRandomRot ? _float4(XMConvertToRadians(Randf(param.vMinRot.x, param.vMaxRot.x)), XMConvertToRadians(Randf(param.vMinRot.y, param.vMaxRot.y)), XMConvertToRadians(Randf(param.vMinRot.z, param.vMaxRot.z)), 0.f) : _float4(XMConvertToRadians(param.vRotation.x), XMConvertToRadians(param.vRotation.y), XMConvertToRadians(param.vRotation.z), 0.f);
	}

	return spawnList;
}

std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeIrregularRing(const SIrregularRingParam& param)
{
	std::vector<PARTICLE_SPAWN_DATA> spawnList(param.iCount);
	if (param.iCount == 0)
		return spawnList;

	const _float angleStep = XM_2PI / static_cast<_float>(param.iCount);
	const _float angleJitter = XMConvertToRadians(param.fAngleJitterDegree);
	const _float lifeMin = std::min(param.fLifeMin, param.fLifeMax);
	const _float lifeMax = std::max(param.fLifeMin, param.fLifeMax);
	const _float coherentPhase = Randf(0.f, XM_2PI);

	for (uint32_t i = 0; i < param.iCount; ++i)
	{
		auto& spawn = spawnList[i];
		const _float angle = angleStep * static_cast<_float>(i) +
			Randf(-angleJitter, angleJitter);
		// Low-frequency harmonics keep neighbouring particles moving together.
		// The ring bends into broad lobes instead of dissolving into random noise.
		const _float coherentRadial =
			sinf(angle * 2.f + coherentPhase) * 0.62f +
			sinf(angle * 3.f - coherentPhase * 0.73f) * 0.28f +
			sinf(angle - coherentPhase * 0.31f) * 0.1f;
		const _float radialScale = std::max(0.05f,
			1.f + coherentRadial * param.fCoherentWarp +
			Randf(-param.fRadialJitter, param.fRadialJitter));
		const _float radialSpeed = param.fBaseSpeed * radialScale;
		const _float coherentTangent =
			sinf(angle * 2.f + coherentPhase + XM_PIDIV2) * 0.7f +
			sinf(angle * 4.f - coherentPhase) * 0.3f;
		const _float tangentSpeed = coherentTangent * param.fTangentialJitter +
			Randf(-param.fTangentialJitter * 0.08f,
				param.fTangentialJitter * 0.08f);
		const _float cosAngle = cosf(angle);
		const _float sinAngle = sinf(angle);

		spawn.position = param.vCenter;
		spawn.velocity = {
			cosAngle * radialSpeed - sinAngle * tangentSpeed + param.vWind.x,
			sinAngle * radialSpeed + cosAngle * tangentSpeed + param.vWind.y,
			0.f
		};
		spawn.life = Randf(lifeMin, lifeMax);
		spawn.spawnDelay = Randf(0.f, std::max(0.f, param.fSpawnDelayMax));
		spawn.fSize = param.fSize;
		spawn.fEndSize = param.fEndSize;
		spawn.color = param.color;
		spawn.emissive = { param.emissive.x, param.emissive.y,
			param.emissive.z, param.startIntensity };
		spawn.endEmissive = { param.endEmissive.x, param.endEmissive.y,
			param.endEmissive.z, param.endIntensity };
		spawn.iBehaviorType = param.iBehaviorType;
		spawn.originalPosition = param.vCenter;
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
			if (param.bRandomPos) {
				s.position = _float3(
					Randf(param.fVelMin.x, param.fVelMax.x) + param.fOffsetX * dx,
					Randf(param.fVelMin.y, param.fVelMax.y), 
					Randf(param.fVelMin.z, param.fVelMax.z) + param.fOffsetZ * (_float)r
				);
			}
			else {
				s.position = _float3(
					param.vStartPos.x + param.fOffsetX * dx,
					param.vStartPos.y,
					param.vStartPos.z + param.fOffsetZ * (_float)r
				);
			}

			s.velocity = param.bRandomVel ?
				_float3(Randf(param.fVelMin.x, param.fVelMax.x),
					Randf(param.fVelMin.y, param.fVelMax.y),
					Randf(param.fVelMin.z, param.fVelMax.z))
				: param.fVelocity;

			s.fSize = param.bRandomSize ?
				_float3(Randf(param.fSizeMin.x, param.fSizeMax.x),
					Randf(param.fSizeMin.y, param.fSizeMax.y),
					Randf(param.fSizeMin.z, param.fSizeMax.z))
				: param.fSize;

			s.life = param.fLife;
			s.fEndSize = param.fEndSize;
			s.color = param.color;

			s.emissive = _float4(param.emissive.x, param.emissive.y, param.emissive.z, param.startIntensity);
			s.endEmissive = _float4(param.endEmissive.x, param.endEmissive.y, param.endEmissive.z, param.endIntensity);

			s.spawnDelay = param.fSpawnDelay * (_float)r;
			s.iBehaviorType = param.iBehaviorType;
			s.originalEmissive = s.emissive;
			s.originalPosition = param.vStartPos;
			s.rotation = param.bRandomRot
				? _float4(XMConvertToRadians(Randf(param.vMinRot.x, param.vMaxRot.x)),
					XMConvertToRadians(Randf(param.vMinRot.y, param.vMaxRot.y)),
					XMConvertToRadians(Randf(param.vMinRot.z, param.vMaxRot.z)),
					0.f)
				: _float4(XMConvertToRadians(param.vRotation.x),
					XMConvertToRadians(param.vRotation.y),
					XMConvertToRadians(param.vRotation.z),
					0.f);
		}
	}
	return spawnList;
}

std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeSpikePattern(const SSpikeParam& param)
{
	std::vector<PARTICLE_SPAWN_DATA> spawnList(param.iRow * param.iCol);
	const _float center = static_cast<_float>(param.iCol - 1) * 0.5f;

	for (uint32_t r = 0; r < param.iRow; ++r)
	{
		const _float rowRatio = static_cast<_float>(r);
		const _float3 rowSizeIncrease = _float3(param.fSizeIncreasePerRow.x * rowRatio, param.fSizeIncreasePerRow.y * rowRatio, param.fSizeIncreasePerRow.z * rowRatio);

		for (uint32_t c = 0; c < param.iCol; ++c)
		{
			PARTICLE_SPAWN_DATA& s = spawnList[r * param.iCol + c];
			const _float dx = static_cast<_float>(c) - center;

			if (param.bRandomPos)
			{
				s.position = _float3(Randf(param.vMinPos.x, param.vMaxPos.x) + param.fOffsetX * dx, Randf(param.vMinPos.y, param.vMaxPos.y), Randf(param.vMinPos.z, param.vMaxPos.z) + param.fOffsetZ * rowRatio);
			}
			else
			{
				s.position = _float3(param.vCenter.x + param.fOffsetX * dx, param.vCenter.y, param.vCenter.z + param.fOffsetZ * rowRatio);
			}

			s.velocity = param.bRandomVel ? _float3(Randf(param.fVelMin.x, param.fVelMax.x), Randf(param.fVelMin.y, param.fVelMax.y), Randf(param.fVelMin.z, param.fVelMax.z)) : param.fVelocity;

			const _float3 startSize = param.bRandomSize ? _float3(Randf(param.fSizeMin.x, param.fSizeMax.x), Randf(param.fSizeMin.y, param.fSizeMax.y), Randf(param.fSizeMin.z, param.fSizeMax.z)) : param.fSize;
			const _float3 endSize = param.bRandomSize ? startSize : param.fEndSize;
			s.fSize = _float3(startSize.x + rowSizeIncrease.x, startSize.y + rowSizeIncrease.y, startSize.z + rowSizeIncrease.z);
			s.fEndSize = _float3(endSize.x + rowSizeIncrease.x, endSize.y + rowSizeIncrease.y, endSize.z + rowSizeIncrease.z);

			s.life = param.fLife;
			s.color = param.color;
			s.emissive = _float4(param.emissive.x, param.emissive.y, param.emissive.z, param.startIntensity);
			s.endEmissive = _float4(param.endEmissive.x, param.endEmissive.y, param.endEmissive.z, param.endIntensity);
			s.spawnDelay = param.fSpawnDelay * rowRatio;
			s.iBehaviorType = param.iBehaviorType;
			s.originalEmissive = s.emissive;
			s.originalPosition = s.position;
			s.originalVelocity = s.velocity;
			s.rotation = param.bRandomRot ? _float4(XMConvertToRadians(Randf(param.vMinRot.x, param.vMaxRot.x)), XMConvertToRadians(Randf(param.vMinRot.y, param.vMaxRot.y)), XMConvertToRadians(Randf(param.vMinRot.z, param.vMaxRot.z)), 0.f) : _float4(XMConvertToRadians(param.vRotation.x), XMConvertToRadians(param.vRotation.y), XMConvertToRadians(param.vRotation.z), 0.f);
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


std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeCone(const SConeParam& param)
{
	std::vector<PARTICLE_SPAWN_DATA> spawnList(param.iCount);
	const _float cosMaxAngle = cosf(XMConvertToRadians(param.fAngleDegree));

	for (uint32_t i = 0; i < param.iCount; ++i)
	{
		const _float randomAngle = E::Randf(0.f, XM_2PI);
		const _float randomRatio = E::Randf(0.f, 1.f);
		const _float cosTheta = 1.f - randomRatio * (1.f - cosMaxAngle);
		const _float sinTheta = sqrtf(std::max(0.f, 1.f - cosTheta * cosTheta));
		const _float speed = E::Randf(param.fSpeedMin, param.fSpeedMax);

		_vector vDirection = XMVectorSet(cosf(randomAngle) * sinTheta, sinf(randomAngle) * sinTheta, cosTheta, 0.f);

		PARTICLE_SPAWN_DATA& spawnData = spawnList[i];
		spawnData.position = param.vCenter;
		XMStoreFloat3(&spawnData.velocity, vDirection * speed);
		spawnData.life = param.fLife;
		
		if (param.bRandomSize) {
			spawnData.fSize = _float3(E::Randf(param.fSizeMin.x, param.fSizeMax.x), E::Randf(param.fSizeMin.y, param.fSizeMax.y), E::Randf(param.fSizeMin.z, param.fSizeMax.z));
		}
		else {
			spawnData.fSize = param.fSize;
		}
		spawnData.fEndSize = param.fEndSize;
		spawnData.color = param.color;

		spawnData.emissive = _float4(param.emissive.x, param.emissive.y, param.emissive.z, param.startIntensity);
		spawnData.endEmissive = _float4(param.endEmissive.x, param.endEmissive.y, param.endEmissive.z, param.endIntensity);
		spawnData.originalPosition = spawnData.position;
		spawnData.originalVelocity = spawnData.velocity;
		spawnData.iBehaviorType = param.iBehaviorType;
		// [LSY] Cone 파티클을 한 프레임에 모두 생성하지 않고 순차 분사할 수 있게 개별 지연을 계산한다.
		spawnData.spawnDelay = param.fSpawnDelay +
			static_cast<_float>(i) * param.fSpawnInterval;

		spawnData.rotation = param.bRandomRot
			? _float4(XMConvertToRadians(Randf(param.vMinRot.x, param.vMaxRot.x)),
				XMConvertToRadians(Randf(param.vMinRot.y, param.vMaxRot.y)),
				XMConvertToRadians(Randf(param.vMinRot.z, param.vMaxRot.z)),
				0.f)
			: _float4(XMConvertToRadians(param.vRotation.x),
				XMConvertToRadians(param.vRotation.y),
				XMConvertToRadians(param.vRotation.z),
				0.f);

	}

	return spawnList;
}

std::vector<PARTICLE_SPAWN_DATA> ParticlePattern::MakeEnergySphere(const SEnergySphere& param){
	std::vector<PARTICLE_SPAWN_DATA> spawnList(1);

	for (uint32_t i = 0; i < 1; ++i)
	{
		PARTICLE_SPAWN_DATA& spawnData = spawnList[i];
		spawnData.color		= param.color;
		spawnData.life		= param.fLife;

		spawnData.fSize		= param.fSize;
		spawnData.fEndSize	= param.fEndSize;

		spawnData.emissive			= _float4(param.emissive.x, param.emissive.y, param.emissive.z, param.startIntensity);
		spawnData.endEmissive		= _float4(param.endEmissive.x, param.endEmissive.y, param.endEmissive.z, param.endIntensity);
		spawnData.originalEmissive	= spawnData.emissive;

		spawnData.originalPosition	= spawnData.position;

		spawnData.iBehaviorType		= param.iBehaviorType;
	}

	return spawnList;
}
