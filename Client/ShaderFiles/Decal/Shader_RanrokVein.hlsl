#include "DecalProjectionCommon.hlsli"

Texture2D g_CrackMask : register(t2);
Texture2D g_FlowMap : register(t3);
Texture2D g_CorruptionNoise : register(t4);

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
	float travelIntensity = max(g_vDecalMaterialParams[2].y, 0.f);
	float travelSpeed = g_vDecalMaterialParams[2].z;
	float travelFrequency = max(g_vDecalMaterialParams[2].w, 0.01f);
	float flowDistortion = max(g_vDecalMaterialParams[3].x, 0.f);
	float maskContrast = max(g_vDecalMaterialParams[3].y, 0.01f);
	float pulseWidth = saturate(g_vDecalMaterialParams[3].z);
	float noiseTiling = max(g_vDecalMaterialParams[3].w, 0.01f);
	float time = g_vDecalProjectionParams.w;

	float mask = pow(saturate(MaxChannel(g_CrackMask.Sample(LinearClamp, surface.uv).rgb)), maskContrast);
	float alpha = saturate(mask * surface.alphaFade);
	clip(alpha - 0.001f);

	float2 flow = g_FlowMap.Sample(LinearWrap, surface.uv * 2.f).rg * 2.f - 1.f;
	float noise = g_CorruptionNoise.Sample(
		LinearWrap, surface.uv * noiseTiling + flow * flowDistortion - time * travelSpeed * 0.05f).r;
	float travelCoord = frac((surface.uv.y + surface.uv.x * 0.18f) * travelFrequency -
		time * travelSpeed + noise * 0.22f);
	float band = 1.f - abs(travelCoord * 2.f - 1.f);
	band = smoothstep(1.f - max(pulseWidth, 0.01f), 1.f, band);
	float intensity = baseIntensity + band * travelIntensity;

	PS_OUT output;
	output.diffuse = float4(albedo.rgb, alpha * albedo.a);
	output.emissive = float4(emissiveColor * intensity, alpha);
	return output;
}
