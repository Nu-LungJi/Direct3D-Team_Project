#include "../ShaderDefines.hlsl"

Texture2D auraRingTexture : register(t0);
Texture2D auraCloudTexture : register(t1);
Texture2D smokeNoiseTexture : register(t2);
Texture2D smokeThinTexture : register(t3);
Texture2D smokeThickTexture : register(t4);

struct PS_IN
{
	float4 posH : SV_POSITION;
	float2 uv : TEXCOORD;
};

float Luminance(float3 color)
{
	return dot(color, float3(0.299f, 0.587f, 0.114f));
}

float4 PSMain(PS_IN input) : SV_Target
{
	const float time = g_ui_texCoord.x;
	const float2 centered = input.uv - 0.5f;
	const float radius = max(length(centered), 0.0001f);
	const float2 radial = centered / radius;
	const float2 tangent = float2(-radial.y, radial.x);

	const float noise0 = Luminance(smokeNoiseTexture.Sample(
		LinearWrap,
		input.uv * 1.65f + float2(time * 0.16f, -time * 0.11f)).rgb);
	const float noise1 = Luminance(smokeNoiseTexture.Sample(
		LinearWrap,
		input.uv.yx * 2.25f + float2(-time * 0.12f, time * 0.15f)).rgb);
	const float flowingNoise = noise0 * 0.62f + noise1 * 0.38f - 0.5f;
	const float wave =
		sin(atan2(centered.y, centered.x) * 5.0f + time * 4.2f) * 0.5f +
		sin(atan2(centered.y, centered.x) * 9.0f - time * 3.1f) * 0.5f;

	float2 warpedUV = input.uv;
	warpedUV += radial * (flowingNoise * 0.024f + wave * 0.007f);
	warpedUV += tangent * (noise1 - 0.5f) * 0.012f;
	if (any(warpedUV < 0.0f) || any(warpedUV > 1.0f))
		discard;

	const float ring = Luminance(auraRingTexture.Sample(
		LinearClamp, warpedUV).rgb);
	const float cloud = Luminance(auraCloudTexture.Sample(
		LinearClamp,
		0.5f + (warpedUV - 0.5f) * 1.025f).rgb);
	uint width = 0;
	uint height = 0;
	auraRingTexture.GetDimensions(width, height);
	const float2 texel = 2.5f / float2(max(width, 1u), max(height, 1u));
	float softRing = ring;
	softRing += Luminance(auraRingTexture.Sample(
		LinearClamp, warpedUV + float2(texel.x, 0.0f)).rgb);
	softRing += Luminance(auraRingTexture.Sample(
		LinearClamp, warpedUV - float2(texel.x, 0.0f)).rgb);
	softRing += Luminance(auraRingTexture.Sample(
		LinearClamp, warpedUV + float2(0.0f, texel.y)).rgb);
	softRing += Luminance(auraRingTexture.Sample(
		LinearClamp, warpedUV - float2(0.0f, texel.y)).rgb);
	softRing *= 0.2f;
	const float filteredRing = lerp(ring, softRing, 0.38f);

	const float angle = atan2(centered.y, centered.x);
	const float pulse = 0.88f + 0.12f * sin(time * 3.8f + noise0 * 2.0f);
	const float sharpEdge = smoothstep(0.16f, 0.64f, filteredRing);
	const float coreLine = smoothstep(0.38f, 0.70f, filteredRing);
	const float lineFlow = 0.84f + 0.16f * sin(
		angle * 5.0f - time * 4.2f + noise1 * 2.5f);
	const float filament = coreLine * lineFlow;
	const float outerGlow = pow(saturate(softRing), 0.72f);
	const float movingMist = pow(saturate(cloud), 1.25f) *
		lerp(0.42f, 1.0f, saturate(noise0 * 0.65f + noise1 * 0.55f));

	// Wrap the horizontal smoke ribbons around the score outline in polar UVs.
	const float angularUV = frac(angle * 0.15915494f + 0.5f);
	const float radialUV = saturate((radius - 0.27f) / 0.20f);
	const float smokeThin = Luminance(smokeThinTexture.Sample(
		LinearWrap,
		float2(angularUV * 1.25f + time * 0.035f, radialUV)).rgb);
	const float smokeThick = Luminance(smokeThickTexture.Sample(
		LinearWrap,
		float2(-angularUV * 1.1f + time * 0.024f, 1.0f - radialUV)).rgb);
	const float radialBand = smoothstep(0.28f, 0.32f, radius) *
		(1.0f - smoothstep(0.46f, 0.50f, radius));
	const float edgeSupport = saturate(
		pow(saturate(softRing), 0.55f) * 1.3f + sharpEdge * 0.28f);
	const float smokeBreakup = smoothstep(
		0.38f,
		0.72f,
		noise0 * 0.62f + noise1 * 0.38f +
			sin(angle * 3.0f - time * 0.8f) * 0.12f);
	const float smokeLines = saturate(
		(smokeThin * 0.68f + smokeThick * 0.32f) * radialBand *
		lerp(0.45f, 1.0f, edgeSupport) *
		lerp(0.12f, 1.0f, smokeBreakup) * 0.82f);
	const float smokeWisps = pow(smokeLines, 0.78f);
	const float aura = saturate(
		sharpEdge * 0.82f + filament * 0.92f +
		outerGlow * 0.38f + movingMist * 0.18f +
		smokeWisps * 0.32f) * pulse;

	const float edgeAA = max(fwidth(aura) * 1.35f, 0.006f);
	const float edgeCoverage = smoothstep(0.002f, 0.002f + edgeAA, aura);

	const float3 defaultColor = float3(0.08f, 0.65f, 0.18f);
	const float hasTint = step(0.0001f,
		max(g_ui_color.r, max(g_ui_color.g, g_ui_color.b)));
	const float3 auraColor = lerp(defaultColor, g_ui_color.rgb, hasTint);
	const float3 filamentColor = lerp(
		auraColor,
		float3(0.22f, 0.85f, 0.32f),
		saturate(filament * 0.58f + smokeWisps * 0.24f));
	const float brightness = 0.68f + sharpEdge * 0.92f +
		filament * 1.15f + movingMist * 0.24f + smokeWisps * 0.18f;
	const float finalAlpha = saturate(aura * 0.94f) *
		edgeCoverage * g_ui_color.a;
	if (finalAlpha < 0.001f)
		discard;
	return float4(filamentColor * brightness, finalAlpha);
}
