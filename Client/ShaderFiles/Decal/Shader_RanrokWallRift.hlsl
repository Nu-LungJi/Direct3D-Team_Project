#include "DecalProjectionCommon.hlsli"

Texture2D g_CrackMask : register(t2);
Texture2D g_GrowthMask : register(t3);
Texture2D g_CloudMask : register(t4);
Texture2D g_Noise : register(t5);

struct PS_OUT { float4 diffuse : SV_TARGET0; float4 emissive : SV_TARGET1; };

float MaxChannel(float3 v) { return max(v.r, max(v.g, v.b)); }

PS_OUT PSMain(DECAL_VS_OUT input)
{
	DECAL_SURFACE_DATA s = GetDecalSurfaceData(input.position);
	float4 stainColor = g_vDecalMaterialParams[0];
	float3 glowColor = g_vDecalMaterialParams[1].rgb;
	float stainIntensity = max(g_vDecalMaterialParams[2].x, 0.f);
	float glowIntensity = max(g_vDecalMaterialParams[2].y, 0.f);
	float pulseSpeed = max(g_vDecalMaterialParams[2].z, 0.f);
	float pulseStrength = saturate(g_vDecalMaterialParams[2].w);
	float hazeIntensity = max(g_vDecalMaterialParams[3].x, 0.f);
	float noiseTiling = max(g_vDecalMaterialParams[3].y, 0.01f);
	float maskContrast = max(g_vDecalMaterialParams[3].z, 0.01f);
	float hazeThreshold = saturate(g_vDecalMaterialParams[3].w);
	float time = g_vDecalProjectionParams.w;

	float crack = MaxChannel(g_CrackMask.Sample(LinearClamp, s.uv).rgb);
	float growth = MaxChannel(g_GrowthMask.Sample(LinearClamp, s.uv).rgb);
	crack = pow(saturate(crack + growth * 0.7f), maskContrast);
	float cloud = g_CloudMask.Sample(LinearWrap, s.uv * 1.35f + float2(time * 0.008f, -time * 0.012f)).r;
	float noise = g_Noise.Sample(LinearWrap, s.uv * noiseTiling - float2(0.f, time * 0.018f)).r;
	float haze = smoothstep(hazeThreshold, min(hazeThreshold + 0.22f, 1.f), cloud * noise);
	haze *= saturate(growth * 0.8f + crack * 0.35f);
	float diffuseAlpha = saturate(max(crack, haze * stainIntensity) * s.alphaFade);
	clip(diffuseAlpha - 0.001f);
	float pulse = 1.f + sin(time * pulseSpeed + noise * 4.f) * pulseStrength;
	float emissiveAlpha = saturate((crack + haze * 0.18f) * s.alphaFade);

	PS_OUT o;
	o.diffuse = float4(stainColor.rgb, diffuseAlpha * stainColor.a);
	o.emissive = float4(glowColor * (crack * glowIntensity * pulse + haze * hazeIntensity), emissiveAlpha);
	return o;
}
