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

	p.position = mul(float4(p.position, 1.f), g_matDelta).xyz;
	p.originalPosition = mul(float4(p.originalPosition, 1.f), g_matDelta).xyz;

	p.velocity = mul(float4(p.velocity, 0.f), g_matDelta).xyz;
	p.originalVelocity = mul(float4(p.originalVelocity, 0.f), g_matDelta).xyz;

	float2 forwardXZ = float2(g_matDelta._31, g_matDelta._33);
	float forwardLength = length(forwardXZ);

	if (forwardLength > 0.000001f)
	{
		forwardXZ /= forwardLength;

		float deltaYaw = atan2(forwardXZ.x, forwardXZ.y);
		p.rotation.y += deltaYaw;

		if (p.rotation.y > PI)
			p.rotation.y -= 2.f * PI;
		else if (p.rotation.y < -PI)
			p.rotation.y += 2.f * PI;
	}

	g_ParticleBuffer[id] = p;
}
