#include "../ShaderDefines.hlsl"

Texture2D OriginalTexture	: register(t0); // PostProcess 이전 텍스쳐
Texture2D BlurPassTexture   : register(t1); // BrightPass  이후 텍스쳐

static const float	CenterWeight		= { 0.227027f };
static const float  BlurOffsets[2]		= { 1.3846154f, 3.2307692f };
static const float	BlurWeights[2]		= { 0.3162162f, 0.0702703f };

static const float  HalfBloomWeight		= { 0.60f };
static const float	QuarterBloomWeight	= { 0.40f };

static const float	BrightThreshold		= { 0.60f };
static const float  BloomIntensity		= { 0.25f };

static const float	Min_Luminance		= { 0.00018442211f };
static const float	Max_Luminance		= { 16.f };

static const float	DistortionIntensity	= { 0.f };	// 왜곡 강도
static const float	ChromaticIntensity	= { 0.f };	// 색수차 강도
static const float	VignetteIntensity	= { 0.f };	// 비네팅 강도
static const float	VignetteSmoothness	= { 0.f }; // 비네팅

cbuffer CB_BLOOM : register(b10)
{
	float2 TexelSize;
	float2 _pad;
};

float3 DownSampling(float2 _TexCoord)
{
	float2 SamplingOffset = TexelSize * 0.5f; // Sampling Near Pixel
	
	float3 Color = 0.f;
	Color += OriginalTexture.Sample(LinearClamp, _TexCoord + float2(-SamplingOffset.x, -SamplingOffset.y)).rgb;
	Color += OriginalTexture.Sample(LinearClamp, _TexCoord + float2(+SamplingOffset.x, -SamplingOffset.y)).rgb;
	Color += OriginalTexture.Sample(LinearClamp, _TexCoord + float2(-SamplingOffset.x, +SamplingOffset.y)).rgb;
	Color += OriginalTexture.Sample(LinearClamp, _TexCoord + float2(+SamplingOffset.x, +SamplingOffset.y)).rgb;

	return Color * 0.25f; // Color / 4.f
}

float3 GaussianBlur(float2 _TexCoord, float2 _TexelDirection)
{
	float4 CenterPixel = BlurPassTexture.Sample(LinearClamp, _TexCoord) * CenterWeight;
	
	[unroll]
	for (int i = 0; i < 2; ++i)
	{
		const float2 Offset = _TexelDirection * BlurOffsets[i];
		CenterPixel += BlurPassTexture.Sample(LinearClamp, _TexCoord + Offset) * BlurWeights[i];
		CenterPixel += BlurPassTexture.Sample(LinearClamp, _TexCoord - Offset) * BlurWeights[i];
	}
	
	return CenterPixel.rgb;
}

float4 PSMain_BrightPass(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{ 
	float3	DownSampledColor = DownSampling(TexCoord);

	float	Luminance = dot(DownSampledColor, float3(0.2126f, 0.7152f, 0.0722f));
	float	Contribution = smoothstep(BrightThreshold - 0.2f, BrightThreshold + 0.2f, Luminance);

	return float4(DownSampledColor * Contribution, 1.f);
}

float4 PSMain_VerticalBlur(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
	return float4(GaussianBlur(TexCoord, float2(0.f, TexelSize.y)), 1.f);
}
float4 PSMain_HorizontalBlur(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
	return float4(GaussianBlur(TexCoord, float2(TexelSize.x, 0.f)), 1.f);
}

float4 PSMain(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
	float3 OriginalColor  = OriginalTexture.Sample(LinearClamp, TexCoord).rgb;
	float3 BloomBlurColor = BlurPassTexture.Sample(LinearClamp, TexCoord).rgb;
    
	return float4(OriginalColor + BloomBlurColor * BloomIntensity, 1.f);
}

float4 PSMain_UpSampling(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
	float3 HalfBloom = OriginalTexture.Sample(LinearClamp, TexCoord).rgb;
	float3 QuarterBloom = BlurPassTexture.Sample(LinearClamp, TexCoord).rgb;
	
	return float4(HalfBloom * HalfBloomWeight + QuarterBloom * QuarterBloomWeight, 1.f);
}
float4 PSMain_DownSampling(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
	return float4(DownSampling(TexCoord), 1.f);
}
