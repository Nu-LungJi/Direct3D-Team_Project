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
	if ((p.iBehaviorType & BEHAVIOR_ORBIT) != 0)
	{
		float3 transformedAxis = mul(float4(p.roationAxis, 0.f), g_matDelta).xyz;
		if (dot(transformedAxis, transformedAxis) > 0.000001f)
			p.roationAxis = normalize(transformedAxis);
	}

	float3 rotatedRight = normalize(mul(float4(RotateXYZ(float3(1.f, 0.f, 0.f), p.rotation), 0.f), g_matDelta).xyz);
	float3 rotatedUp = normalize(mul(float4(RotateXYZ(float3(0.f, 1.f, 0.f), p.rotation), 0.f), g_matDelta).xyz);
	float3 rotatedForward = normalize(mul(float4(RotateXYZ(float3(0.f, 0.f, 1.f), p.rotation), 0.f), g_matDelta).xyz);
	float sinYaw = clamp(-rotatedRight.z, -1.f, 1.f);
	float cosYaw = sqrt(max(1.f - sinYaw * sinYaw, 0.f));
	float pitch = 0.f;
	float yaw = asin(sinYaw);
	float roll = 0.f;

	if (cosYaw > 0.00001f)
	{
		pitch = atan2(rotatedUp.z, rotatedForward.z);
		roll = atan2(rotatedRight.y, rotatedRight.x);
	}
	else
	{
		pitch = atan2(sinYaw * rotatedUp.x, rotatedUp.y);
	}

	p.rotation.x = atan2(sin(pitch), cos(pitch));
	p.rotation.y = atan2(sin(yaw), cos(yaw));
	p.rotation.z = atan2(sin(roll), cos(roll));

	g_ParticleBuffer[id] = p;
}
