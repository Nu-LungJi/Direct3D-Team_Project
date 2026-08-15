#include "DecalProjectionCommon.hlsli"

Texture2D g_PoolMask : register(t2);
Texture2D g_FlowMap : register(t3);
Texture2D g_Perlin : register(t4);
Texture2D g_Waves : register(t5);

struct PS_OUT { float4 diffuse : SV_TARGET0; float4 emissive : SV_TARGET1; };

float MaxChannel(float3 v) { return max(v.r, max(v.g, v.b)); }

PS_OUT PSMain(DECAL_VS_OUT input)
{
	DECAL_SURFACE_DATA s = GetDecalSurfaceData(input.position);
	float4 poolColor = g_vDecalMaterialParams[0];
	float3 glowColor = g_vDecalMaterialParams[1].rgb;
	float poolOpacity = saturate(g_vDecalMaterialParams[2].x);
	float glowIntensity = max(g_vDecalMaterialParams[2].y, 0.f);
	float flowSpeed = g_vDecalMaterialParams[2].z;
	float pulseSpeed = max(g_vDecalMaterialParams[2].w, 0.f);
	float pulseStrength = saturate(g_vDecalMaterialParams[3].x);
	float rimWidth = saturate(g_vDecalMaterialParams[3].y);
	float noiseTiling = max(g_vDecalMaterialParams[3].z, 0.01f);
	float distortion = max(g_vDecalMaterialParams[3].w, 0.f);
	float time = g_vDecalProjectionParams.w;

	float2 flow = g_FlowMap.Sample(LinearWrap, s.uv * 1.8f).rg * 2.f - 1.f;
	float2 uv = s.uv + flow * distortion + float2(time * flowSpeed, -time * flowSpeed * 0.6f);
	float poolMask = MaxChannel(g_PoolMask.Sample(LinearClamp, s.uv).rgb);
	float perlin = g_Perlin.Sample(LinearWrap, uv * noiseTiling).r;
	float waves = MaxChannel(g_Waves.Sample(LinearWrap, uv * 2.2f).rgb);
	float shape = saturate(poolMask * (0.72f + perlin * 0.45f));
	float alpha = saturate(shape * poolOpacity * s.alphaFade);
	clip(alpha - 0.001f);
	float rim = smoothstep(max(1.f - rimWidth, 0.f), 1.f, shape);
	float pulse = 1.f + sin(time * pulseSpeed + perlin * 5.f) * pulseStrength;
	float energy = saturate(rim + waves * shape * 0.35f);

	PS_OUT o;
	o.diffuse = float4(poolColor.rgb, alpha * poolColor.a);
	o.emissive = float4(glowColor * glowIntensity * pulse, saturate(energy * s.alphaFade));
	return o;
}
