#include "../ShaderHeader/SH_CommonFunction.hlsli"

struct VS_IN
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
	float2 TexCoord : TEXCOORD0;

	float4 World0 : INSTANCE_WORLD0;
	float4 World1 : INSTANCE_WORLD1;
	float4 World2 : INSTANCE_WORLD2;
	float4 World3 : INSTANCE_WORLD3;
};

struct VS_OUT
{
	float4 Position : SV_POSITION;
};

// Point Shadow Pixel Shader가 거리 기반 깊이를 계산할 수 있도록 월드 위치도 함께 전달
struct VS_POINT_OUT
{
	float4 Position : SV_POSITION;
	float3 WorldPosition : TEXCOORD0;
};

cbuffer CB_SHADOW : register(b11)
{
	uint CurrentShadowLightIndex;
	uint CurrentPointFaceIndex;
	uint CurrentCascadeIndex;
	float ShadowPadding;
};

VS_OUT VSMain(VS_IN input)
{
	VS_OUT output;

	float4x4 world = float4x4(
		input.World0,
        input.World1,
        input.World2,
        input.World3);

	float4 worldPosition = mul(float4(input.Position, 1.f), world);
	
	// Spot Light는 g_LightViewProj[0]을 사용, Directional Light만 현재 CSM Cascade의 행렬을 사용
	uint lightViewProjIndex = 0;

	if (AffectedLight[CurrentShadowLightIndex].LightType ==LIGHT_DIRECTIONAL)
	{
		lightViewProjIndex = CurrentCascadeIndex;
	}

	output.Position = mul(worldPosition, AffectedLight[CurrentShadowLightIndex].g_LightViewProj[lightViewProjIndex]);

	return output;
}

VS_POINT_OUT VSMainPointFace(VS_IN input)
{
	VS_POINT_OUT output;

	float4x4 worldMatrix = float4x4(
        input.World0,
        input.World1,
        input.World2,
        input.World3);

	float4 worldPosition = mul(float4(input.Position, 1.f),worldMatrix);

	output.WorldPosition = worldPosition.xyz;

	output.Position = mul(worldPosition, AffectedLight[CurrentShadowLightIndex].g_LightViewProj[CurrentPointFaceIndex]);

	return output;
}
