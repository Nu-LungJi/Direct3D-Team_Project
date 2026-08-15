#include "DecalProjectionCommon.hlsli"

Texture2D g_CrackMask : register(t2);
Texture2D g_GrowthMask : register(t3);
Texture2D g_BlightMask : register(t4);
Texture2D g_CorruptionNoise : register(t5);
Texture2D g_FlowMap : register(t6);

struct PS_OUT
{
	float4 diffuse : SV_TARGET0;
	float4 emissive : SV_TARGET1;
};

DECAL_VS_OUT VSMain(DECAL_VS_IN input)
{
	return DecalVSMain(input);
}

float MaxChannel(float3 value)
{
	return max(value.r, max(value.g, value.b));
}

PS_OUT PSMain(DECAL_VS_OUT input)
{
	DECAL_SURFACE_DATA surface = GetDecalSurfaceData(input.position);
	float4 stainColor = g_vDecalMaterialParams[0];
	float3 crackColor = g_vDecalMaterialParams[1].rgb;
	float stainIntensity = max(g_vDecalMaterialParams[2].x, 0.f);
	float crackIntensity = max(g_vDecalMaterialParams[2].y, 0.f);
	float hotCoreIntensity = max(g_vDecalMaterialParams[2].z, 0.f);
	float pulseSpeed = max(g_vDecalMaterialParams[2].w, 0.f);
	float pulseStrength = saturate(g_vDecalMaterialParams[3].x);
	float travelSpeed = g_vDecalMaterialParams[3].y;
	float travelWidth = saturate(g_vDecalMaterialParams[3].z);
	float maskContrast = max(g_vDecalMaterialParams[3].w, 0.01f);
	float blightThreshold = saturate(g_vDecalMaterialParams[4].x);
	float blightSoftness = max(g_vDecalMaterialParams[4].y, 0.001f);
	float noiseTiling = max(g_vDecalMaterialParams[4].z, 0.01f);
	float flowDistortion = max(g_vDecalMaterialParams[4].w, 0.f);
	float time = g_vDecalProjectionParams.w;

	float crackSource = MaxChannel(g_CrackMask.Sample(LinearClamp, surface.uv).rgb);
	float growthSource = MaxChannel(g_GrowthMask.Sample(LinearClamp, surface.uv).rgb);
	float crack = pow(saturate(crackSource + growthSource * 0.72f), maskContrast);

	float packedBlight = g_BlightMask.Sample(LinearClamp, surface.uv).r;
	float noise = g_CorruptionNoise.Sample(LinearWrap, surface.uv * noiseTiling).r;
	float erosionShape = saturate((1.f - packedBlight) * 0.9f + growthSource * 0.42f);
	float erosionBreakup = smoothstep(
		blightThreshold - blightSoftness,
		blightThreshold + blightSoftness,
		noise + growthSource * 0.25f);
	float erosion = saturate(erosionShape * erosionBreakup * stainIntensity);

	float diffuseAlpha = saturate(max(erosion * 0.72f, crack) * surface.alphaFade);
	clip(diffuseAlpha - 0.001f);

	float2 flow = g_FlowMap.Sample(LinearWrap, surface.uv * 2.1f).rg * 2.f - 1.f;
	float travelCoord = frac(
		(surface.uv.y + surface.uv.x * 0.2f) * 2.2f -
		time * travelSpeed +
		flow.x * flowDistortion);
	float travelBand = 1.f - abs(travelCoord * 2.f - 1.f);
	travelBand = smoothstep(1.f - max(travelWidth, 0.01f), 1.f, travelBand);
	float pulse = 1.f + sin(time * pulseSpeed + noise * 3.f) * pulseStrength;
	float hotCore = smoothstep(0.68f, 0.94f, noise) * crack;
	float emissiveIntensity = crackIntensity * pulse + travelBand * hotCoreIntensity + hotCore * hotCoreIntensity * 0.45f;
	float emissiveAlpha = saturate(crack * surface.alphaFade);

	PS_OUT output;
	output.diffuse = float4(stainColor.rgb, diffuseAlpha * stainColor.a);
	output.emissive = float4(crackColor * emissiveIntensity, emissiveAlpha);
	return output;
}
