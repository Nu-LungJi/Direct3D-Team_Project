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
AppendStructuredBuffer<uint> gDeadList : register(u1);

[numthreads(256, 1, 1)]
void CSMain(uint id : SV_DispatchThreadID)
{
	if (id >= g_iMaxParticles)
		return;

	ParticleData p = g_ParticleBuffer[id];

	if (p.alive == 1 && p.ownerID == g_iTargetOwnerID)
	{
		p.alive = 0;
		p.color = float4(0.f, 0.f, 0.f, 0.f);
		p.emissive = float4(0.f, 0.f, 0.f, 0.f);
		p.life = 0.f;
		p.size = 0.f;
		p.ownerID = 0;
		p.iBehaviorType = 0;
		p.loop = 0;

		g_ParticleBuffer[id] = p;
		gDeadList.Append(id);
	}
}
