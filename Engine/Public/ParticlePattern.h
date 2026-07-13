// ParticlePattern.h
#pragma once
#include "Engine_Defines.h"
#include "Engine_Struct.h"
#include "ParticleParams.h"
NS_BEGIN(Engine)

namespace ParticlePattern
{
	std::vector<PARTICLE_SPAWN_DATA> MakeStairs(const SStairsParam& p);
	std::vector<PARTICLE_SPAWN_DATA> MakeCircle(const SCircleParam& p);
	std::vector<PARTICLE_SPAWN_DATA> MakeSpiral(const SSpiralParam& p);
	std::vector<PARTICLE_SPAWN_DATA> MakeStraightGround(const SStraightGroundParam& p);
}
NS_END
