#include "DecalProjectionCommon.hlsli"

Texture2D g_BlightMask : register(t2);
Texture2D g_GrimeMask : register(t3);
Texture2D g_PerlinNoise : register(t4);
Texture2D g_CorruptionNoise : register(t5);

struct PS_OUT
{
	float4 diffuse : SV_TARGET0;
	float4 emissive : SV_TARGET1;
};

PS_OUT PSMain(DECAL_VS_OUT input)
{
	DECAL_SURFACE_DATA surface = GetDecalSurfaceData(input.position);
	float4 albedo = g_vDecalMaterialParams[0];
	float3 emissiveColor = g_vDecalMaterialParams[1].rgb;
	float baseIntensity = max(g_vDecalMaterialParams[2].x, 0.f);
	float veinIntensity = max(g_vDecalMaterialParams[2].y, 0.f);
	float breatheSpeed = max(g_vDecalMaterialParams[2].z, 0.f);
	float breatheStrength = saturate(g_vDecalMaterialParams[2].w);
	float maskThreshold = saturate(g_vDecalMaterialParams[3].x);
	float maskSoftness = max(g_vDecalMaterialParams[3].y, 0.001f);
	float noiseTiling = max(g_vDecalMaterialParams[3].z, 0.01f);
	float driftSpeed = g_vDecalMaterialParams[3].w;
	float time = g_vDecalProjectionParams.w;

	float packedMask = g_BlightMask.Sample(LinearClamp, surface.uv).r;
	float grime = g_GrimeMask.Sample(LinearClamp, surface.uv).r;
	float perlin = g_PerlinNoise.Sample(
		LinearWrap, surface.uv * noiseTiling + float2(time * driftSpeed, -time * driftSpeed * 0.37f)).r;
	float shape = saturate((1.f - packedMask) * 0.65f + grime * 0.55f);
	shape *= smoothstep(maskThreshold - maskSoftness, maskThreshold + maskSoftness, perlin);
	float alpha = saturate(shape * surface.alphaFade);
	clip(alpha - 0.001f);

	float corruption = g_CorruptionNoise.Sample(
		LinearWrap, surface.uv * (noiseTiling * 1.7f) - float2(time * driftSpeed * 0.4f, 0.f)).r;
	float veins = smoothstep(0.64f, 0.92f, corruption) * shape;
	float breathe = 1.f + sin(time * breatheSpeed + perlin * 4.f) * breatheStrength;
	float intensity = baseIntensity * breathe + veins * veinIntensity;

	PS_OUT output;
	output.diffuse = float4(albedo.rgb, alpha * albedo.a);
	output.emissive = float4(emissiveColor * intensity, alpha);
	return output;
}
