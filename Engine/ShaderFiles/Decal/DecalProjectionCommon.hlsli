#ifndef DECAL_PROJECTION_COMMON_HLSLI
#define DECAL_PROJECTION_COMMON_HLSLI

#include "../ShaderDefines.hlsl"

Texture2D g_DecalDepthTexture : register(t0);
Texture2D g_DecalNormalTexture : register(t1);

cbuffer CB_DECAL_VOLUME : register(b11)
{
	matrix g_matInvDecalWorld;
	float4 g_vDecalProjectionParams;
	float4 g_vDecalMaterialParams[8];
};

struct DECAL_VS_IN
{
	float3 position : POSITION;
};

struct DECAL_VS_OUT
{
	float4 position : SV_POSITION;
};

struct DECAL_SURFACE_DATA
{
	float3 worldPosition;
	float3 localPosition;
	float3 surfaceNormal;
	float2 uv;
	float alphaFade;
};

DECAL_VS_OUT DecalVSMain(DECAL_VS_IN input)
{
	DECAL_VS_OUT output;
	output.position = mul(float4(input.position, 1.f), g_matWVP);
	return output;
}

float3 ReconstructDecalWorldPosition(float2 screenUV, float depth)
{
	float4 ndcPosition = float4(screenUV.x * 2.f - 1.f, 1.f - screenUV.y * 2.f, depth, 1.f);
	float4 worldPosition = mul(ndcPosition, g_matInvViewProj);
	return worldPosition.xyz / max(worldPosition.w, 0.000001f);
}

DECAL_SURFACE_DATA GetDecalSurfaceData(float4 screenPosition)
{
	uint width;
	uint height;
	g_DecalDepthTexture.GetDimensions(width, height);
	uint2 pixel = min(uint2(screenPosition.xy), uint2(max(width, 1u) - 1u, max(height, 1u) - 1u));
	float2 screenUV = (float2(pixel) + 0.5f) / float2(width, height);
	float depth = g_DecalDepthTexture.Load(int3(pixel, 0)).r;
	clip(0.999999f - depth);

	DECAL_SURFACE_DATA surface;
	surface.worldPosition = ReconstructDecalWorldPosition(screenUV, depth);
	surface.localPosition = mul(float4(surface.worldPosition, 1.f), g_matInvDecalWorld).xyz;
	clip(0.5f - abs(surface.localPosition));

	surface.surfaceNormal = normalize(g_DecalNormalTexture.Load(int3(pixel, 0)).xyz * 2.f - 1.f);
	float3 projectionAxis = normalize(g_matWorld[1].xyz);
	float normalThreshold = saturate(g_vDecalProjectionParams.y);
	float normalFade = smoothstep(
		normalThreshold,
		min(normalThreshold + 0.15f, 1.f),
		abs(dot(surface.surfaceNormal, projectionAxis)));

	surface.uv = float2(surface.localPosition.x + 0.5f, 0.5f - surface.localPosition.z);
	float edgeSoftness = clamp(g_vDecalProjectionParams.z, 0.001f, 0.49f);
	float sideDistance = max(abs(surface.localPosition.x), abs(surface.localPosition.z));
	float edgeFade = 1.f - smoothstep(0.5f - edgeSoftness, 0.5f, sideDistance);
	surface.alphaFade = g_vDecalProjectionParams.x * normalFade * edgeFade;
	return surface;
}

#endif
