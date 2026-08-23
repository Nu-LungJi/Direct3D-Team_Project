#include "../ShaderDefines.hlsl"

Texture2D g_SceneTexture : register(t0);
Texture2D g_DepthTexture : register(t1);

float3 ReconstructWorldPosition(float2 screenUV, float depth)
{
	float4 ndcPosition = float4(screenUV.x * 2.f - 1.f,1.f - screenUV.y * 2.f, depth, 1.f);

	float4 worldPosition = mul(ndcPosition, g_matInvViewProj);
	return worldPosition.xyz / worldPosition.w;
}

float2 CalculateVelocity(float2 uv, float depth)
{
	float3 worldPosition = ReconstructWorldPosition(uv, depth);
	float4 prevClip = mul(float4(worldPosition, 1.f), g_matPrevViewProj);

	prevClip.xy /= prevClip.w;

	float2 prevUV;
	prevUV.x = prevClip.x * 0.5f + 0.5f;
	prevUV.y = 0.5f - prevClip.y * 0.5f;

	return uv - prevUV;
}

float4 PS_Main(
	float4 position : SV_POSITION,
	float2 texCoord : TEXCOORD0) : SV_TARGET
{
	float depth = g_DepthTexture.SampleLevel(PointClamp, texCoord, 0).r;
	float2 velocity = CalculateVelocity(texCoord, depth);

	uint width;
	uint height;
	g_SceneTexture.GetDimensions(width, height);

	float2 screenSize = float2(width, height);
	float2 velocityPixels = velocity * screenSize;
	float speedPixels = length(velocityPixels);

	// 작은 카메라 움직임 제거
	const float blurStartPixels = 8.f;
	const float blurFullPixels = 12.f;
	float blurFactor = smoothstep(blurStartPixels, blurFullPixels, speedPixels);

	velocity *= blurFactor;

	// 최대 블러 길이 제한
	const float maxBlurPixels = 32.f;
	velocityPixels = velocity * screenSize;
	velocity *= min(
		1.f,
		maxBlurPixels / max(length(velocityPixels), 0.000001f));

	const int sampleCount = 8;
	float4 color = 0.f;

	[unroll]
	for (int i = 0; i < sampleCount; ++i)
	{
		float ratio = (float)i / (sampleCount - 1);
		float2 sampleUV = texCoord - velocity * ratio;
		color += g_SceneTexture.SampleLevel(LinearClamp, sampleUV, 0);
	}

	return color / sampleCount;
}
