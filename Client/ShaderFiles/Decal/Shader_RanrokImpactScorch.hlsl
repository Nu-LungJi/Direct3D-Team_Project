#include "DecalProjectionCommon.hlsli"

Texture2D g_DamageMask : register(t2);
Texture2D g_CrackMask : register(t3);
Texture2D g_DustMask : register(t4);
Texture2D g_Noise : register(t5);

struct PS_OUT { float4 diffuse : SV_TARGET0; float4 emissive : SV_TARGET1; };

float MaxChannel(float3 v) { return max(v.r, max(v.g, v.b)); }

PS_OUT PSMain(DECAL_VS_OUT input)
{
	DECAL_SURFACE_DATA s = GetDecalSurfaceData(input.position);
	float4 scorchColor = g_vDecalMaterialParams[0];
	float3 heatColor = g_vDecalMaterialParams[1].rgb;
	float scorchOpacity = saturate(g_vDecalMaterialParams[2].x);
	float heatIntensity = max(g_vDecalMaterialParams[2].y, 0.f);
	float flickerSpeed = max(g_vDecalMaterialParams[2].z, 0.f);
	float flickerStrength = saturate(g_vDecalMaterialParams[2].w);
	float edgeWidth = saturate(g_vDecalMaterialParams[3].x);
	float noiseTiling = max(g_vDecalMaterialParams[3].y, 0.01f);
	float crackStrength = max(g_vDecalMaterialParams[3].z, 0.f);
	float dustStrength = max(g_vDecalMaterialParams[3].w, 0.f);
	float time = g_vDecalProjectionParams.w;

	float4 damageSample = g_DamageMask.Sample(LinearClamp, s.uv);
	float damage = max(damageSample.a, MaxChannel(damageSample.rgb));
	float crack = MaxChannel(g_CrackMask.Sample(LinearClamp, s.uv).rgb) * crackStrength;
	float dust = g_DustMask.Sample(LinearClamp, s.uv).r * dustStrength;
	float shape = saturate(max(damage, crack) + dust * 0.35f);
	float alpha = saturate(shape * scorchOpacity * s.alphaFade);
	clip(alpha - 0.001f);
	float noise = g_Noise.Sample(LinearWrap, s.uv * noiseTiling).r;
	float hotEdge = smoothstep(0.05f, max(edgeWidth, 0.051f), crack + damage * noise);
	float flicker = 1.f + sin(time * flickerSpeed + noise * 6.f) * flickerStrength;
	float heatAlpha = saturate(hotEdge * s.alphaFade);

	PS_OUT o;
	o.diffuse = float4(scorchColor.rgb, alpha * scorchColor.a);
	o.emissive = float4(heatColor * heatIntensity * flicker, heatAlpha);
	return o;
}
