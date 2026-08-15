#include "DecalProjectionCommon.hlsli"

Texture2D g_GrowthMask : register(t2);
Texture2D g_SecondaryGrowthMask : register(t3);
Texture2D g_FlowMap : register(t4);
Texture2D g_CorruptionNoise : register(t5);

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
	float4 albedo = g_vDecalMaterialParams[0];
	float3 emissiveColor = g_vDecalMaterialParams[1].rgb;
	float emissiveIntensity = max(g_vDecalMaterialParams[2].x, 0.f);

	float2 flowDirection = g_vDecalMaterialParams[3].xy;
	float flowDirectionLength = length(flowDirection);
	flowDirection = flowDirectionLength > 0.0001f ? flowDirection / flowDirectionLength : float2(0.f, 1.f);
	float flowSpeed = g_vDecalMaterialParams[3].z;
	float flowTiling = max(g_vDecalMaterialParams[3].w, 0.01f);
	float flowStrength = max(g_vDecalMaterialParams[4].x, 0.f);
	float flowWidth = saturate(g_vDecalMaterialParams[4].y);
	float pulseSpeed = max(g_vDecalMaterialParams[4].z, 0.f);
	float pulseStrength = saturate(g_vDecalMaterialParams[4].w);
	float noiseTiling = max(g_vDecalMaterialParams[5].x, 0.01f);
	float flowDistortion = max(g_vDecalMaterialParams[5].y, 0.f);
	float secondaryStrength = max(g_vDecalMaterialParams[5].z, 0.f);
	float time = g_vDecalProjectionParams.w;

	float primaryMask = MaxChannel(g_GrowthMask.Sample(LinearClamp, surface.uv).rgb);
	float secondaryMask = MaxChannel(g_SecondaryGrowthMask.Sample(LinearClamp, surface.uv).rgb);
	float mask = saturate(primaryMask + secondaryMask * secondaryStrength);
	float alpha = saturate(mask * surface.alphaFade);
	clip(alpha - 0.001f);

	float2 flowVector = g_FlowMap.Sample(LinearWrap, surface.uv * flowTiling).rg * 2.f - 1.f;
	flowVector += flowDirection;
	float flowVectorLength = max(length(flowVector), 0.0001f);
	flowVector /= flowVectorLength;

	float phase0 = frac(time * flowSpeed);
	float phase1 = frac(time * flowSpeed + 0.5f);
	float weight0 = 1.f - abs(phase0 * 2.f - 1.f);
	float weight1 = 1.f - abs(phase1 * 2.f - 1.f);
	float2 noiseUV0 = surface.uv * noiseTiling - flowVector * phase0 * flowDistortion;
	float2 noiseUV1 = surface.uv * noiseTiling - flowVector * phase1 * flowDistortion;
	float noise0 = g_CorruptionNoise.Sample(LinearWrap, noiseUV0).r;
	float noise1 = g_CorruptionNoise.Sample(LinearWrap, noiseUV1).r;
	float flowingNoise = (noise0 * weight0 + noise1 * weight1) / max(weight0 + weight1, 0.0001f);
	float energyBand = smoothstep(1.f - flowWidth, 1.f, flowingNoise);
	float pulse = 1.f + sin(time * pulseSpeed) * pulseStrength;
	float finalEmissiveIntensity = emissiveIntensity * pulse + energyBand * flowStrength;

	PS_OUT output;
	output.diffuse = float4(albedo.rgb, alpha * albedo.a);
	output.emissive = float4(emissiveColor * finalEmissiveIntensity, alpha);
	return output;
}
