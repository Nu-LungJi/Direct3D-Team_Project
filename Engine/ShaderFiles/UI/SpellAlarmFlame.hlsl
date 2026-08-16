#include "../ShaderDefines.hlsl"

Texture2D g_FlameTexture : register(t0);
Texture2D g_FlowMap : register(t1);
Texture2D g_DistortionNoise : register(t2);

struct PS_IN
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD;
};

float SteadySway(float phase)
{
	const float sineWave = sin(phase);
	const float triangleWave =
		0.636619772f * asin(clamp(sineWave, -1.f, 1.f));
	return lerp(sineWave, triangleWave, 0.68f);
}

float4 PSMain(PS_IN input) : SV_Target
{
	const float time = g_ui_margins.x * 2.15f;
	const float phase = g_ui_margins.y;
	const float swayScale = g_ui_margins.z;
	const float2 uv = input.UV;

	const float2 flowUV = uv * float2(1.18f, 1.36f) +
		float2(0.055f * time + phase * 0.07f, 0.18f * time);
	const float2 flow =
		g_FlowMap.Sample(LinearWrap, flowUV).rg * 2.f - 1.f;
	const float2 noiseUV = uv * float2(1.72f, 1.31f) +
		float2(-0.042f * time, 0.12f * time + phase * 0.11f);
	const float2 noise =
		g_DistortionNoise.Sample(LinearWrap, noiseUV).rg * 2.f - 1.f;

	const float tipWeight = pow(saturate(1.f - uv.y), 1.18f);
	// Keep the base attached to the straight UI edge. Motion fades through
	// the lower section and is completely removed at the bottom.
	const float baseAnchor = 1.f - smoothstep(0.60f, 0.92f, uv.y);
	const float broadSway =
		SteadySway(time * 1.85f + uv.y * 5.2f + phase) *
		0.042f * swayScale;

	float2 distortedUV = uv;
	distortedUV.x += (flow.x * 0.036f + noise.x * 0.021f) *
		baseAnchor;
	distortedUV.x += broadSway * tipWeight * baseAnchor;
	distortedUV.y += (flow.y * 0.021f + noise.y * 0.013f) *
		baseAnchor;
	distortedUV.y += 0.012f * sin(
		uv.x * 11.f + time * 2.25f + phase) * baseAnchor;

	if (any(distortedUV < 0.f) || any(distortedUV > 1.f))
		discard;

	const float4 source =
		g_FlameTexture.Sample(LinearClamp, distortedUV);
	// The source keeps bright RGB values even in fully transparent pixels.
	// Alpha alone defines the flame silhouette; including RGB reveals the quad.
	const float sourceEnergy = source.a;
	const float rawEnergy = saturate(sourceEnergy);
	const float sourceDetail = saturate(dot(
		source.rgb,
		float3(0.299f, 0.587f, 0.114f)));
	const float flicker = 0.91f + 0.09f * sin(
		time * 7.1f + uv.y * 10.5f + phase * 1.7f);

	// Preserve the source's pale strands and folds without adding another
	// flame layer. The alpha power keeps the outer wisps soft instead of
	// turning low-alpha pixels into a hard contour.
	const float strandDetail = smoothstep(0.24f, 0.92f, sourceDetail);
	const float3 bodyGold = float3(0.66f, 0.52f, 0.27f);
	const float3 strandGold = float3(1.12f, 0.98f, 0.66f);
	const float3 flameColor = lerp(
		bodyGold,
		strandGold,
		strandDetail) * g_ui_color.rgb;
	const float softSilhouette = pow(rawEnergy, 1.28f);
	const float alpha = softSilhouette *
		lerp(0.34f, 0.50f, strandDetail) *
		flicker * g_ui_color.a;
	if (alpha < 0.004f)
		discard;

	return float4(flameColor, alpha);
}
