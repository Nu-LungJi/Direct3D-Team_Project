#include "DecalProjectionCommon.hlsli"

Texture2D g_DecalMaskTexture : register(t2);

struct PS_OUT
{
	float4 diffuse : SV_TARGET0;
	float4 emissive : SV_TARGET1;
};

DECAL_VS_OUT VSMain(DECAL_VS_IN input)
{
	return DecalVSMain(input);
}

PS_OUT PSMain(DECAL_VS_OUT input)
{
	DECAL_SURFACE_DATA surface = GetDecalSurfaceData(input.position);
	float3 maskSample = g_DecalMaskTexture.Sample(LinearClamp, surface.uv).rgb;
	float mask = max(maskSample.r, max(maskSample.g, maskSample.b));
	float alpha = saturate(mask * surface.alphaFade);
	clip(alpha - 0.001f);

	float4 albedo = g_vDecalMaterialParams[0];
	float3 emissiveColor = g_vDecalMaterialParams[1].rgb;
	float emissiveIntensity = max(g_vDecalMaterialParams[2].x, 0.f);

	PS_OUT output;
	output.diffuse = float4(albedo.rgb, alpha * albedo.a);
	output.emissive = float4(emissiveColor * emissiveIntensity, alpha);
	return output;
}

