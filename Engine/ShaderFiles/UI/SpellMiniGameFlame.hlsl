#include "../ShaderDefines.hlsl"

Texture2D g_FlameTexture : register(t0);
Texture2D g_FlameMask : register(t1);
Texture2D g_FlowMap : register(t2);
Texture2D g_DistortionNoise : register(t3);
Texture2D g_FlameTopClamp : register(t4);
Texture2D g_SparksTexture : register(t5);

struct VS_IN
{
	float3 Position : POSITION;
	float2 UV : TEXCOORD;
};

struct PS_IN
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD;
};

PS_IN VSMain(VS_IN input)
{
	PS_IN output;
	output.Position = mul(float4(input.Position, 1.f), g_matWVP);
	output.UV = lerp(input.UV, 1.f - input.UV, g_ui_uvFlip);
	return output;
}

float Luminance(float3 color)
{
	return dot(color, float3(0.299f, 0.587f, 0.114f));
}

float SampleFlameEnergy(float2 uv)
{
	const float4 sampleColor = g_FlameTexture.Sample(LinearClamp, uv);
	return sampleColor.a;
}

float SampleClampEnergy(float2 uv)
{
	const float4 sampleColor = g_FlameTopClamp.Sample(LinearClamp, uv);
	return sampleColor.a;
}

float SteadySway(float phase)
{
	const float sineWave = sin(phase);
	const float triangleWave = 0.636619772f * asin(clamp(sineWave, -1.f, 1.f));
	return lerp(sineWave, triangleWave, 0.82f);
}

float4 PSMain(PS_IN input) : SV_Target
{
	const float2 uv = input.UV;
	const float time = g_ui_margins.x * 2.25f;

	// Two differently moving fields avoid an obvious texture-loop seam.
	const float2 flowUV = uv * float2(1.15f, 1.35f) +
		float2(0.07f * time, 0.19f * time);
	const float2 flow = g_FlowMap.Sample(LinearWrap, flowUV).rg * 2.f - 1.f;
	const float2 noiseUV = uv * float2(1.7f, 1.25f) +
		float2(-0.045f * time, 0.13f * time);
	const float2 noise = g_DistortionNoise.Sample(LinearWrap, noiseUV).rg * 2.f - 1.f;

	float2 distortedUV = uv;
	const float tipWeight = pow(saturate(1.f - uv.y), 1.25f);
	const float broadSway =
		SteadySway(time * 2.f + uv.y * 5.4f) * 0.034f +
		sin(-uv.y * 8.2f + 1.3f) * 0.014f;
	distortedUV.x += flow.x * 0.040f + noise.x * 0.024f;
	distortedUV.x += broadSway * tipWeight;
	distortedUV.y += flow.y * 0.024f + noise.y * 0.015f;
	distortedUV.y += 0.016f * sin(uv.x * 12.f + time * 2.6f);

	const float maskSample = g_FlameMask.Sample(LinearClamp, uv).r;
	const float sideMask = 1.f - smoothstep(0.72f, 1.02f, abs(uv.x * 2.f - 1.f));
	const float softenedMask = lerp(
		0.22f,
		1.f,
		pow(saturate(maskSample), 0.72f));
	const float shapeMask = saturate(softenedMask * sideMask);

	const float2 innerUV = (distortedUV - 0.5f) * 1.06f + 0.5f;
	const float innerFlame = SampleFlameEnergy(saturate(innerUV));
	// Pull the UVs toward the center so the second PointFlame layer appears
	// slightly larger than the bright inner flame.
	float2 outerUV = (distortedUV - 0.5f) * 0.80f + 0.5f;
	outerUV.x += SteadySway(time * 2.f + uv.y * 4.4f + 0.9f) * 0.012f * tipWeight;
	outerUV.y += 0.035f * sin(time * 2.1f);
	const float outerFlame = SampleClampEnergy(saturate(outerUV));
	// A third, wider PointFlame layer forms the subdued blue silhouette at
	// the very outside of the effect.
	float2 farOuterUV = (distortedUV - 0.5f) * 0.65f + 0.5f;
	farOuterUV.x += SteadySway(time * 2.f + uv.y * 3.8f + 2.1f) * 0.020f * tipWeight;
	farOuterUV.y += 0.026f * sin(time * 1.8f + 0.7f);
	const float farOuterFlame = SampleClampEnergy(saturate(farOuterUV));

	// The extracted material uses Flame Size (Power) = 1.6785.
	const float innerRawEnergy = saturate(innerFlame * shapeMask);
	const float outerRawEnergy = saturate(outerFlame * shapeMask * 0.76f);
	const float farOuterRawEnergy = saturate(farOuterFlame * shapeMask * 0.56f);
	const float innerContour = smoothstep(0.018f, 0.10f, innerRawEnergy);
	const float outerContour = smoothstep(0.012f, 0.085f, outerRawEnergy);
	const float farOuterContour = smoothstep(0.010f, 0.075f, farOuterRawEnergy);
	const float innerDensity = pow(innerRawEnergy, 1.28f);
	const float outerDensity = pow(outerRawEnergy, 1.12f);
	const float farOuterDensity = pow(farOuterRawEnergy, 1.06f);
	const float flicker = 0.91f + 0.09f * sin(time * 8.2f + uv.y * 11.f);
	const float innerEnergy = saturate(innerDensity * flicker);
	const float outerEnergy = saturate(outerDensity * flicker);
	const float farOuterEnergy = saturate(farOuterDensity * flicker);

	const float3 innerColor = lerp(
		float3(0.10f, 0.32f, 0.80f),
		float3(0.38f, 0.96f, 1.10f),
		smoothstep(0.16f, 0.78f, innerEnergy));
	const float3 outerColor = lerp(
		float3(0.20f, 0.42f, 0.84f),
		float3(0.50f, 0.82f, 1.02f),
		smoothstep(0.10f, 0.72f, outerEnergy));
	const float3 farOuterColor = lerp(
		float3(0.035f, 0.09f, 0.30f),
		float3(0.14f, 0.34f, 0.68f),
		smoothstep(0.08f, 0.66f, farOuterEnergy));
	const float outerBlend = smoothstep(0.06f, 0.56f, outerEnergy);
	const float innerBlend = smoothstep(0.08f, 0.64f, innerEnergy);
	const float3 outerCombinedColor = lerp(farOuterColor, outerColor, outerBlend);
	float3 flameColor = lerp(outerCombinedColor, innerColor, innerBlend);
	flameColor *= 1.02f + innerEnergy * 0.88f;
	flameColor *= g_ui_color.rgb;

	const float farOuterAlpha = farOuterContour * lerp(0.14f, 0.50f, farOuterEnergy);
	const float outerAlpha = outerContour * lerp(0.26f, 0.76f, outerEnergy);
	const float innerAlpha = innerContour * lerp(0.32f, 0.96f, innerEnergy);
	const float flameAlpha = max(farOuterAlpha, max(outerAlpha, innerAlpha)) * g_ui_color.a;

	// White magic splatters rise vertically around the flame on two sparse layers.
	// Reuse the flame's broad sway so the sparks move with its silhouette.
	const float sparkFlameSway = broadSway * tipWeight;
	float2 sparkUV1 = uv * float2(1.12f, 1.28f);
	sparkUV1 += float2(
		sparkFlameSway + SteadySway(time * 1.15f + uv.y * 3.2f) * 0.006f,
		time * 0.09375f);
	sparkUV1 += float2(flow.x * 0.018f + noise.x * 0.011f,
		flow.y * 0.009f + noise.y * 0.006f);

	float2 sparkUV2 = uv * float2(1.58f, 1.72f) + float2(0.37f, 0.21f);
	sparkUV2 += float2(
		sparkFlameSway * 1.12f - SteadySway(time * 0.92f + uv.y * 4.1f + 1.7f) * 0.005f,
		time * 0.13125f);
	sparkUV2 += float2(flow.x * 0.012f - noise.x * 0.009f,
		flow.y * 0.006f - noise.y * 0.0045f);

	const float sparkSample1 = pow(saturate(g_SparksTexture.Sample(LinearWrap, sparkUV1).a), 3.2f);
	const float sparkSample2 = pow(saturate(g_SparksTexture.Sample(LinearWrap, sparkUV2).a), 3.2f);
	// Keep the particles inside a soft envelope matching the flame's size.
	// The far-outer contour boosts particles nearest to the actual silhouette,
	// while still allowing a small amount just outside the flame edge.
	const float2 sparkEnvelopeUV =
		(uv - float2(0.54f, 0.49f)) / float2(0.34f, 0.33f);
	const float sparkProximityMask =
		(1.f - smoothstep(0.80f, 1.05f, length(sparkEnvelopeUV))) *
		lerp(0.62f, 1.f, farOuterContour);
	const float sparkVerticalMask =
		smoothstep(0.15f, 0.25f, uv.y) *
		(1.f - smoothstep(0.72f, 0.84f, uv.y));
	const float sparkTwinkle = 0.82f +
		0.18f * sin(time * 5.3f + uv.x * 17.f + uv.y * 23.f);
	const float sparkAlpha = saturate(max(sparkSample1, sparkSample2 * 0.48f) *
		sparkVerticalMask * sparkProximityMask * sparkTwinkle * 0.90f) *
		g_ui_color.a;

	const float alpha = max(flameAlpha, sparkAlpha);
	if (alpha < 0.004f)
		discard;

	const float3 sparkColor = float3(1.12f, 1.16f, 1.20f);
	const float3 combinedColor =
		(flameColor * flameAlpha + sparkColor * sparkAlpha) /
		max(alpha, 0.0001f);
	return float4(combinedColor, alpha);
}
