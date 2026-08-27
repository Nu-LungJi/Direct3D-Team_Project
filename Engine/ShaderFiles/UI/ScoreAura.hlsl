#include "../ShaderDefines.hlsl"

Texture2D auraRingTexture : register(t0);
Texture2D auraCloudTexture : register(t1);
Texture2D scrollingCloudsTexture : register(t2);

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

	const float noise0 = scrollingCloudsTexture.Sample(
		LinearWrap,
		input.uv * 1.65f + float2(time * 0.075f, -time * 0.048f)).r;
	const float noise1 = scrollingCloudsTexture.Sample(
		LinearWrap,
		input.uv.yx * 2.25f + float2(-time * 0.052f, time * 0.069f)).r;
	const float flowingNoise = noise0 * 0.62f + noise1 * 0.38f - 0.5f;
	const float wave =
		sin(atan2(centered.y, centered.x) * 5.0f + time * 2.6f) * 0.5f +
		sin(atan2(centered.y, centered.x) * 9.0f - time * 1.8f) * 0.5f;

	float2 warpedUV = input.uv;
	warpedUV += radial * (flowingNoise * 0.020f + wave * 0.006f);
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

	const float pulse = 0.88f + 0.12f * sin(time * 3.1f + noise0 * 4.0f);
	const float sharpEdge = smoothstep(0.10f, 0.72f, ring);
	const float outerGlow = pow(saturate(softRing), 0.72f);
	const float movingMist = pow(saturate(cloud), 1.25f) *
		lerp(0.42f, 1.0f, saturate(noise0 * 0.65f + noise1 * 0.55f));
	const float aura = saturate(
		sharpEdge * 0.82f + outerGlow * 0.58f + movingMist * 0.28f) * pulse;

	if (aura < 0.008f)
		discard;

	const float3 defaultColor = float3(0.075f, 0.82f, 0.46f);
	const float hasTint = step(0.0001f,
		max(g_ui_color.r, max(g_ui_color.g, g_ui_color.b)));
	const float3 auraColor = lerp(defaultColor, g_ui_color.rgb, hasTint);
	const float brightness = 0.72f + sharpEdge * 0.95f + movingMist * 0.30f;
	return float4(
		auraColor * brightness,
		saturate(aura * 0.82f) * g_ui_color.a);
}
