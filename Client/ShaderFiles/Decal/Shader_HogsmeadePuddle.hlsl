#include "DecalProjectionCommon.hlsli"

Texture2D<float4> g_PuddleDecalMask : register(t2);
Texture2D<float4> g_PuddleDecalNormal : register(t3);

struct PS_OUT
{
	float4 diffuse : SV_TARGET0;
	float4 emissive : SV_TARGET1;
	float4 surface : SV_TARGET2;
};

DECAL_VS_OUT VSMain(DECAL_VS_IN input)
{
	return DecalVSMain(input);
}

float2 EncodeOctNormal(float3 _Normal)
{
	_Normal /= abs(_Normal.x) + abs(_Normal.y) + abs(_Normal.z);
	float2 e = _Normal.xy;

	if (_Normal.z < 0.f)
		e = (1.f - abs(e.yx)) * sign(e);

	return e * 0.5f + 0.5f;
}

PS_OUT PSMain(DECAL_VS_OUT input)
{
	PS_OUT OUT;
	
	DECAL_SURFACE_DATA decal = GetDecalSurfaceData(input.position);

	float PuddleMask = g_PuddleDecalMask.Sample(LinearClamp, decal.uv).b;
	float PuddleCoverage = saturate(PuddleMask * decal.alphaFade);
	clip(PuddleCoverage - 0.001f);
	
	float3 DecalMaterial = g_vDecalMaterialParams[1].rgb;
	
	float Roughness = DecalMaterial.x;
	float SurfaceWeight = PuddleCoverage * DecalMaterial.y;
	float Darkening = PuddleCoverage * DecalMaterial.z;
	
	float3 PuddleNormal = g_PuddleDecalNormal.Sample(LinearClamp, decal.uv).xyz * 2.f - 1.f;
	PuddleNormal.xy *= g_vDecalMaterialParams[1].w;
	PuddleNormal = normalize(PuddleNormal);
	
	float3 T = normalize(g_matWorld[0].xyz);
	float3 B = -normalize(g_matWorld[2].xyz);
	float3 N = normalize(g_matWorld[1].xyz);
	
	PuddleNormal = normalize(PuddleNormal.x * T + PuddleNormal.y * B + PuddleNormal.z * N);
	float2 PackedNormal = EncodeOctNormal(PuddleNormal);
	
	OUT.diffuse = float4(float3(0.03f, 0.035f, 0.03f), Darkening);
	OUT.emissive = float4(0.f, 0.f, 0.f, 0.f);

	OUT.surface = float4(Roughness, PackedNormal.x, PackedNormal.y, SurfaceWeight);
	return OUT;
}

