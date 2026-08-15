#include "DecalProjectionCommon.hlsli"

Texture2D g_Noise : register(t2);
Texture2D g_DetailMask : register(t3);

struct PS_OUT { float4 diffuse : SV_TARGET0; float4 emissive : SV_TARGET1; };

PS_OUT PSMain(DECAL_VS_OUT input)
{
	DECAL_SURFACE_DATA s = GetDecalSurfaceData(input.position);
	float4 tint = g_vDecalMaterialParams[0];
	float3 glowColor = g_vDecalMaterialParams[1].rgb;
	float glowIntensity = max(g_vDecalMaterialParams[2].x, 0.f);
	float pulseSpeed = max(g_vDecalMaterialParams[2].y, 0.f);
	float pulseStrength = saturate(g_vDecalMaterialParams[2].z);
	float ringRadius = saturate(g_vDecalMaterialParams[2].w);
	float ringWidth = saturate(g_vDecalMaterialParams[3].x);
	float noiseTiling = max(g_vDecalMaterialParams[3].y, 0.01f);
	float rotationSpeed = g_vDecalMaterialParams[3].z;
	float centerStrength = max(g_vDecalMaterialParams[3].w, 0.f);
	float time = g_vDecalProjectionParams.w;

	float2 centered = s.uv - 0.5f;
	float angle = time * rotationSpeed;
	float cs = cos(angle), sn = sin(angle);
	float2 rotated = float2(centered.x * cs - centered.y * sn, centered.x * sn + centered.y * cs);
	float noise = g_Noise.Sample(LinearWrap, rotated * noiseTiling + 0.5f).r;
	float detail = g_DetailMask.Sample(LinearClamp, s.uv).r;
	float radius = length(centered) * 2.f;
	float ring = 1.f - smoothstep(ringWidth, ringWidth + 0.08f, abs(radius - ringRadius + (noise - 0.5f) * 0.08f));
	float center = saturate(1.f - radius) * centerStrength * detail;
	float shape = saturate(ring + center);
	float alpha = saturate(shape * tint.a * s.alphaFade);
	clip(alpha - 0.001f);
	float pulse = 1.f + sin(time * pulseSpeed + noise * 4.f) * pulseStrength;

	PS_OUT o;
	o.diffuse = float4(tint.rgb, alpha);
	o.emissive = float4(glowColor * glowIntensity * pulse, alpha);
	return o;
}
