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

RWStructuredBuffer<ParticleData> g_ParticleBuffer : register(u0);

[numthreads(256, 1, 1)]
void CSMain(uint id : SV_DispatchThreadID)
{
	if (id >= g_iMaxParticles)
		return;

	ParticleData p = g_ParticleBuffer[id];

	if (p.alive == 0 || p.ownerID != g_iTargetOwnerID)
		return;
	p.emissive = vEmissive;
	p.originalEmissive = vEmissive;
	p.color = vColor;
	g_ParticleBuffer[id] = p;
}
