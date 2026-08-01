#include "../Particle/Particle_Common_Struct_Func.hlsl"

cbuffer CB_SPAWN_COUNT : register(b12)
{
	uint g_iSpawnCount;
	uint g_iMaxParticles;
	float2 pad;
};

StructuredBuffer<SPAWN_DATA> gSpawnBuffer : register(t6);
RWStructuredBuffer<uint> gDeadList : register(u0);
RWStructuredBuffer<ParticleData> gParticles : register(u1);
RWStructuredBuffer<uint> gDeadCount : register(u2);

bool TryPopDeadIndex(out uint particleIndex)
{
	particleIndex = 0;

	[loop]
	while (true)
	{
		uint currentCount = gDeadCount[0];

		if (currentCount == 0)
			return false;

		uint originalCount;

		InterlockedCompareExchange(
			gDeadCount[0],
			currentCount,
			currentCount - 1,
			originalCount);

		if (originalCount == currentCount)
		{
			particleIndex = gDeadList[currentCount - 1];
			return particleIndex < g_iMaxParticles;
		}
	}
}

[numthreads(256, 1, 1)]
void CSMain(uint id : SV_DispatchThreadID)
{
	if (id >= g_iSpawnCount)
		return;

	uint index;

	if (!TryPopDeadIndex(index))
		return;

	SPAWN_DATA s = gSpawnBuffer[id];

	ParticleData p = (ParticleData) 0;
	p.position = s.position;
	p.velocity = s.velocity;
	p.life = s.life;
	p.maxLife = s.life;
	p.startSize = s.size;
	p.endSize = s.endSize;
	p.rotation = s.rotation;
	p.size = s.size;
	p.stopSizeTime = s.stopSizeTime;
	p.alive = 1;
	p.color = s.color;
	p.originalVelocity = s.originalVelocity;
	p.originalEmissive = s.originalEmissive;
	p.emissive = s.emissive;
	p.endEmissive = s.endEmissive;
	p.ownerID = s.ownerID;
	p.iBehaviorType = s.iBehaviorType;
	p.loop = s.loop;
	p.originalPosition = s.originalPosition;
	p.roationAxis = s.roationAxis;
	p.fRotationSpeed = s.fRotationSpeed;

	gParticles[index] = p;
}
