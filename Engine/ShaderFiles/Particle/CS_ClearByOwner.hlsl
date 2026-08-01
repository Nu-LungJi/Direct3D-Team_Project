#include "../Particle/Particle_Common_Struct_Func.hlsl"

cbuffer CB_OWNER_OPERATION : register(b13)
{
	uint g_iTargetOwnerID;
	uint g_iMaxParticles;
	float2 g_vPadding;

	row_major float4x4 g_matDelta;
	float4 vColor;
	float4 vEmissive;
};

RWStructuredBuffer<ParticleData> gParticleBuffer : register(u0);
RWStructuredBuffer<uint> gDeadList : register(u1);
RWStructuredBuffer<uint> gDeadCount : register(u2);

[numthreads(256, 1, 1)]
void CSMain(uint id : SV_DispatchThreadID)
{
	if (id >= g_iMaxParticles)
		return;

	ParticleData p = gParticleBuffer[id];

	if (p.alive == 0 || p.ownerID != g_iTargetOwnerID)
		return;

	p.alive = 0;
	p.color = float4(0.f, 0.f, 0.f, 0.f);
	p.emissive = float4(0.f, 0.f, 0.f, 0.f);
	p.life = 0.f;
	p.size = float3(0.f, 0.f, 0.f);
	p.ownerID = 0;
	p.frameIndex = 0;
	p.originalPosition = float3(0.f, 0.f, 0.f);
	p.iBehaviorType = 0;
	p.velocity = float3(0.f, 0.f, 0.f);
	p.loop = 0;

	gParticleBuffer[id] = p;

	uint writeIndex;
	InterlockedAdd(gDeadCount[0], 1, writeIndex);

	if (writeIndex < g_iMaxParticles)
	{
		gDeadList[writeIndex] = id;
	}
	else
	{
		uint ignored;
		InterlockedAdd(gDeadCount[0], 0xffffffffu, ignored);
	}
}
