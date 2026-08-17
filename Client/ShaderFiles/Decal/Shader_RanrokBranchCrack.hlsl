#include "DecalProjectionCommon.hlsli"

Texture2D g_CrackMask : register(t2);

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
	float emissiveIntensity = max(g_vDecalMaterialParams[2].x, 0.f);
	float maskThreshold = saturate(g_vDecalMaterialParams[2].y);
	float maskSoftness = max(g_vDecalMaterialParams[2].z, 0.001f);
	float maskContrast = max(g_vDecalMaterialParams[2].w, 0.01f);
	float pulseStrength = saturate(g_vDecalMaterialParams[3].x);
	float pulseSpeed = g_vDecalMaterialParams[3].y;
	float pulseFrequency = max(g_vDecalMaterialParams[3].z, 0.01f);
	float time = g_vDecalProjectionParams.w;

	float rawMask = g_CrackMask.Sample(LinearClamp, surface.uv).r;
	rawMask = pow(saturate(rawMask), maskContrast);
	float crackMask = smoothstep(
		maskThreshold - maskSoftness,
		maskThreshold + maskSoftness,
		rawMask);
	float alpha = saturate(crackMask * surface.alphaFade);
	clip(alpha - 0.001f);

	float phase = (surface.uv.x + surface.uv.y * 0.63f) * pulseFrequency * 6.2831853f;
	float pulse = 0.5f + 0.5f * sin(phase - time * pulseSpeed);
	float intensity = emissiveIntensity * lerp(1.f - pulseStrength, 1.f + pulseStrength, pulse);

	PS_OUT output;
	output.diffuse = float4(albedo.rgb, alpha * albedo.a);
	output.emissive = float4(emissiveColor * intensity, alpha);
	return output;
}
