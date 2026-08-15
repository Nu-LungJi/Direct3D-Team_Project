#include "DecalProjectionCommon.hlsli"

Texture2D g_SurfaceTexture : register(t2);
Texture2D g_DetailMask : register(t3);
Texture2D g_FlowMap : register(t4);
Texture2D g_Noise : register(t5);

struct PS_OUT { float4 diffuse : SV_TARGET0; float4 emissive : SV_TARGET1; };

PS_OUT PSMain(DECAL_VS_OUT input)
{
	DECAL_SURFACE_DATA s = GetDecalSurfaceData(input.position);
	float4 albedo = g_vDecalMaterialParams[0];
	float3 emissiveColor = g_vDecalMaterialParams[1].rgb;
	float surfaceOpacity = saturate(g_vDecalMaterialParams[2].x);
	float emissiveIntensity = max(g_vDecalMaterialParams[2].y, 0.f);
	float flowSpeed = g_vDecalMaterialParams[2].z;
	float flowTiling = max(g_vDecalMaterialParams[2].w, 0.01f);
	float threshold = saturate(g_vDecalMaterialParams[3].x);
	float softness = max(g_vDecalMaterialParams[3].y, 0.001f);
	float distortion = max(g_vDecalMaterialParams[3].z, 0.f);
	float detailStrength = max(g_vDecalMaterialParams[3].w, 0.f);
	float time = g_vDecalProjectionParams.w;

	float2 flow = g_FlowMap.Sample(LinearWrap, s.uv * flowTiling).rg * 2.f - 1.f;
	float2 animatedUV = s.uv + flow * distortion + float2(0.f, -time * flowSpeed);
	float4 surfaceSample = g_SurfaceTexture.Sample(LinearClamp, animatedUV);
	float detail = g_DetailMask.Sample(LinearClamp, s.uv).r;
	float noise = g_Noise.Sample(LinearWrap, s.uv * (flowTiling * 1.7f) + float2(time * flowSpeed * 0.2f, 0.f)).r;
	float sourceMask = max(surfaceSample.a, dot(surfaceSample.rgb, float3(0.299f, 0.587f, 0.114f)));
	float mask = smoothstep(threshold - softness, threshold + softness, sourceMask + detail * detailStrength + noise * 0.16f);
	float alpha = saturate(mask * surfaceOpacity * s.alphaFade);
	clip(alpha - 0.001f);
	float energy = smoothstep(0.68f, 0.94f, noise) * mask;

	PS_OUT o;
	o.diffuse = float4(albedo.rgb, alpha * albedo.a);
	o.emissive = float4(emissiveColor * emissiveIntensity * energy, saturate(energy * s.alphaFade));
	return o;
}
