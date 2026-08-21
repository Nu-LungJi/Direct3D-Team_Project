#include "../ShaderDefines.hlsl"

Texture2D g_PathTexture : register(t0);

struct VS_IN
{
	float3 posL : POSITION;
	float2 uv : TEXCOORD;
};

struct PS_IN
{
	float4 posH : SV_POSITION;
	float2 uv : TEXCOORD;
};

PS_IN VSMain(VS_IN input)
{
	PS_IN output;
	output.posH = mul(float4(input.posL, 1.f), g_matWVP);
	output.uv = input.uv;
	return output;
}

void TestSegment(
	float2 uv,
	float2 start,
	float2 end,
	float progressStart,
	float progressEnd,
	inout float bestDistanceSq,
	inout float bestProgress)
{
	const float2 segment = end - start;
	const float segmentLengthSq = max(dot(segment, segment), 1e-6f);
	const float ratio = saturate(dot(uv - start, segment) / segmentLengthSq);
	const float2 closest = start + segment * ratio;
	const float distanceSq = dot(uv - closest, uv - closest);

	if (distanceSq < bestDistanceSq)
	{
		bestDistanceSq = distanceSq;
		bestProgress = lerp(progressStart, progressEnd, ratio);
	}
}

float2 EvaluateCurve(float t)
{
	const float2 start = float2(0.852f, 0.928f);
	const float2 control = float2(0.700f, 0.848f);
	const float2 end = float2(0.472f, 0.853f);
	const float oneMinusT = 1.f - t;
	return oneMinusT * oneMinusT * start +
		2.f * oneMinusT * t * control +
		t * t * end;
}

float CalculateIncendioPathProgress(float2 uv)
{
	const float2 start = float2(0.122f, 0.883f);
	const float2 top = float2(0.480f, 0.086f);
	const float2 lowerRight = float2(0.852f, 0.928f);

	// Segment ratios match the arc-length table used by CSpellMiniGame.
	const float firstEnd = 0.400f;
	const float secondEnd = 0.821f;
	const uint curveSegmentCount = 16u;

	float bestDistanceSq = 1e10f;
	float bestProgress = 0.f;
	TestSegment(
		uv,
		start,
		top,
		0.f,
		firstEnd,
		bestDistanceSq,
		bestProgress);
	TestSegment(
		uv,
		top,
		lowerRight,
		firstEnd,
		secondEnd,
		bestDistanceSq,
		bestProgress);

	[unroll]
	for (uint i = 0u; i < curveSegmentCount; ++i)
	{
		const float t0 = (float)i / (float)curveSegmentCount;
		const float t1 = (float)(i + 1u) / (float)curveSegmentCount;
		TestSegment(
			uv,
			EvaluateCurve(t0),
			EvaluateCurve(t1),
			lerp(secondEnd, 1.f, t0),
			lerp(secondEnd, 1.f, t1),
			bestDistanceSq,
			bestProgress);
	}

	return bestProgress;
}

float CalculateFlipendoPathProgress(float2 uv)
{
	static const uint pointCount = 25u;
	static const float2 pathPoints[pointCount] =
	{
		float2(0.144f, 0.541f),
		float2(0.166f, 0.571f),
		float2(0.197f, 0.618f),
		float2(0.229f, 0.664f),
		float2(0.260f, 0.710f),
		float2(0.291f, 0.757f),
		float2(0.328f, 0.830f),
		float2(0.354f, 0.679f),
		float2(0.385f, 0.566f),
		float2(0.416f, 0.467f),
		float2(0.447f, 0.382f),
		float2(0.479f, 0.311f),
		float2(0.510f, 0.253f),
		float2(0.541f, 0.208f),
		float2(0.572f, 0.178f),
		float2(0.598f, 0.162f),
		float2(0.635f, 0.188f),
		float2(0.666f, 0.244f),
		float2(0.697f, 0.307f),
		float2(0.729f, 0.362f),
		float2(0.766f, 0.378f),
		float2(0.791f, 0.377f),
		float2(0.822f, 0.345f),
		float2(0.844f, 0.300f),
		float2(0.859f, 0.223f)
	};

	float totalDistance = 0.f;
	[unroll]
	for (uint i = 0u; i + 1u < pointCount; ++i)
		totalDistance += length(pathPoints[i + 1u] - pathPoints[i]);

	float bestDistanceSq = 1e10f;
	float bestProgress = 0.f;
	float accumulatedDistance = 0.f;
	[unroll]
	for (uint segmentIndex = 0u;
		segmentIndex + 1u < pointCount;
		++segmentIndex)
	{
		const float segmentDistance = length(
			pathPoints[segmentIndex + 1u] -
			pathPoints[segmentIndex]);
		TestSegment(
			uv,
			pathPoints[segmentIndex],
			pathPoints[segmentIndex + 1u],
			accumulatedDistance / totalDistance,
			(accumulatedDistance + segmentDistance) / totalDistance,
			bestDistanceSq,
			bestProgress);
		accumulatedDistance += segmentDistance;
	}

	return bestProgress;
}

float4 PSMain(PS_IN input) : SV_Target
{
	const float4 pathColor =
		g_PathTexture.Sample(LinearClamp, input.uv);
	if (pathColor.a < 0.01f)
		discard;

	const float pixelProgress = g_ui_texCoord.y > 0.5f
		? CalculateFlipendoPathProgress(input.uv)
		: CalculateIncendioPathProgress(input.uv);
	const float currentProgress = saturate(g_ui_texCoord.x);
	if (currentProgress <= 0.0001f)
		discard;

	const float edgeWidth = 0.004f;
	const float revealAlpha = 1.f - smoothstep(
		currentProgress,
		currentProgress + edgeWidth,
		pixelProgress);
	if (revealAlpha <= 0.001f)
		discard;

	const float brightness = dot(
		pathColor.rgb,
		float3(0.299f, 0.587f, 0.114f));
	const float3 blueColor = g_ui_color.rgb *
		lerp(0.75f, 1.25f, saturate(brightness));

	return float4(
		blueColor,
		pathColor.a * g_ui_color.a * revealAlpha);
}
