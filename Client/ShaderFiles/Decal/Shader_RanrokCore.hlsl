#include "DecalProjectionCommon.hlsli"

Texture2D g_CoreMask : register(t2);
Texture2D g_GrowthMask : register(t3);
Texture2D g_PerlinNoise : register(t4);
Texture2D g_CorruptionNoise : register(t5);

struct PS_OUT
{
	float4 diffuse : SV_TARGET0;
	float4 emissive : SV_TARGET1;
};

float MaxChannel(float3 value)
{
	return max(value.r, max(value.g, value.b));
}

PS_OUT PSMain(DECAL_VS_OUT input)
{
	DECAL_SURFACE_DATA surface = GetDecalSurfaceData(input.position);
	float4 albedo = g_vDecalMaterialParams[0];
	float3 emissiveColor = g_vDecalMaterialParams[1].rgb;
	float baseIntensity = max(g_vDecalMaterialParams[2].x, 0.f);
	float ringIntensity = max(g_vDecalMaterialParams[2].y, 0.f);
	float ringSpeed = g_vDecalMaterialParams[2].z;
	float ringFrequency = max(g_vDecalMaterialParams[2].w, 0.01f);
	float pulseSpeed = max(g_vDecalMaterialParams[3].x, 0.f);
	float pulseStrength = saturate(g_vDecalMaterialParams[3].y);
	float growthStrength = max(g_vDecalMaterialParams[3].z, 0.f);
	float noiseTiling = max(g_vDecalMaterialParams[3].w, 0.01f);
	float time = g_vDecalProjectionParams.w;

	float coreMask = MaxChannel(g_CoreMask.Sample(LinearClamp, surface.uv).rgb);
	float growthMask = MaxChannel(g_GrowthMask.Sample(LinearClamp, surface.uv).rgb);
	float shape = saturate(coreMask + growthMask * growthStrength);
	float alpha = saturate(shape * surface.alphaFade);
	clip(alpha - 0.001f);

	float2 centered = surface.uv - 0.5f;
	float radial = length(centered) * ringFrequency;
	float perlin = g_PerlinNoise.Sample(LinearWrap, surface.uv * noiseTiling).r;
	float corruption = g_CorruptionNoise.Sample(
		LinearWrap, surface.uv * noiseTiling + float2(time * 0.03f, -time * 0.02f)).r;
	float ring = 0.5f + 0.5f * sin(radial * 6.283185f - time * ringSpeed + perlin * 2.f);
	ring = smoothstep(0.72f, 1.f, ring) * shape;
	float pulse = 1.f + sin(time * pulseSpeed + corruption * 3.f) * pulseStrength;
	float intensity = baseIntensity * pulse + ring * ringIntensity;

	PS_OUT output;
	output.diffuse = float4(albedo.rgb, alpha * albedo.a);
	output.emissive = float4(emissiveColor * intensity, alpha);
	return output;
}
