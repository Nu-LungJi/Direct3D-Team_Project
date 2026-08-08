#include "../ShaderHeader/SH_CommonFunction.hlsli"

Texture2D DepthTexture				: register(t0);
Texture2D SceneColorTexture			: register(t1);
Texture3D VoxelAccumulatedTexture	: register(t2);
Texture2D BlueNoiseTexture			: register(t3);

const static float	MaxFroxelZDistance = { 100.f };
const static float2 NoiseResolution = { 256.f, 256.f };
const static float	DepthThreshold = { 25.f };

cbuffer CB_FroxelConfig : register(b10)
{
	float3 FroxelGridSize;
	float NearZ;
	float FarZ;
	float2 ScreenResolution;
	
	float Padding;
};
cbuffer CB_VLFOG : register(b11)
{
	float3	FogColor;
	float	FogIntensity;
	   
	float3	FogCenterPos;
	float	FogHeight;
	
	float	FogStartPos;
	float	FogEndPos;
	float	FogDensity;
	float	FogNoiseScale;
	
	float3	FogLightDirection;
	float	FogAnisotropyGA; // 전방 산란도
	
	float3	FogLightColor;
	float	FogAnisotropyGB; // 후방 산란도
	
	float	FogScatteringWeight; // 전방/후방 가중치
	float3	FogPadding;
};


float ViewDepthToFroxelZ(float _Depth, float _Near, float _Far)
{
	return log(max(_Depth, _Near) / _Near) / log(_Far / _Near);
}

float4 CalculateAnalyticFog(float ViewDepth, float3 WorldPos)
{
	float HeightDiffer = max(WorldPos.y - FogCenterPos.y, 0.f);
	float HeightFactor = exp(-HeightDiffer * 0.02f);
	
	float FogAmount = 1.f - exp(-ViewDepth * FogDensity * 0.1f * HeightFactor);
	
	return float4(FogColor * FogIntensity * FogAmount, 1.f - FogAmount);
}

float4 PSMain(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
	float4	SceneColor = SceneColorTexture.Sample(PointClamp, TexCoord);
	float	Depth = DepthTexture.Sample(PointClamp, TexCoord).r;
	
	float3	WorldPos = Convert_WorldPosByDepth(Depth, TexCoord);
	float	ViewDepth = Convert_ViewZPosByDepth(Depth);
	
	float3	FinalScattering = float3(0.f, 0.f, 0.f);
	float	FinalTransmittance = 1.f;
	
	if (Depth >= 1.f)
	{
		float4 SkyFog = CalculateAnalyticFog(MaxFroxelZDistance * 2.0f, g_vCamPos + float3(0, 0, MaxFroxelZDistance));
		FinalScattering = SkyFog.rgb;
		FinalTransmittance = SkyFog.a;

		float3 FinalColor = SceneColor.rgb * FinalTransmittance + FinalScattering;
		return float4(FinalColor, SceneColor.a);
	}
	
	float2	TexelSize = float2(1.f / ScreenResolution.x, 1.f / ScreenResolution.y);
	
	float4	AccumulatedFog = float4(0.f, 0.f, 0.f, 0.f);
	float	TotalWeight = 0.0001f;
	
	[unroll]
	for (int x = -1; x <= 1; ++x)
	{
		[unroll]
		for (int y = -1; y <= 1; ++y)
		{
			float2 OffsetUV = TexCoord + float2(x, y) * TexelSize;
			float SampleDepth = DepthTexture.SampleLevel(PointClamp, OffsetUV, 0).r;
			float SampleViewZ = Convert_ViewZPosByDepth(SampleDepth);
			
			float SampleFroxelZ = ViewDepthToFroxelZ(SampleViewZ, NearZ, MaxFroxelZDistance);
			SampleFroxelZ -= (0.5f / FroxelGridSize.z);

			float3 SampleUVW = float3(OffsetUV, saturate(SampleFroxelZ));
			float4 SampleFog = VoxelAccumulatedTexture.SampleLevel(LinearClamp, SampleUVW, 0);
			
			float SpatialWeight = exp(-(x * x + y * y) / 2.f);
			
			float DepthDiff = abs(ViewDepth - SampleViewZ);
			float DepthWeight = exp(-DepthDiff * 20.0f);

			float Weight = SpatialWeight * DepthWeight;

			AccumulatedFog += SampleFog * Weight;
			TotalWeight += Weight;
		}
	}
	
	float4	FinalFog = AccumulatedFog / TotalWeight;
	
	//float	FroxelTexCoordZ = ViewDepthToFroxelZ(ViewDepth, NearZ, MaxFroxelZDistance);
	//float	CorrectedZ = FroxelTexCoordZ - (0.5f / FroxelGridSize.z);
	//
	//float2 NoiseUV = Position.xy / NoiseResolution;
	//float Jitter = BlueNoiseTexture.Sample(PointWrap, NoiseUV).r - 0.5f;
	//CorrectedZ += Jitter / FroxelGridSize.z;
	//
	//float3 FroxelTexCoord = float3(TexCoord, saturate(CorrectedZ));
	//
	//float4	FogTex = VoxelAccumulatedTexture.Sample(LinearClamp, FroxelTexCoord);

	//float4	GodRayTex = GodRayTexture.Sample(LinearClamp, TexCoord);

	if (ViewDepth > 200.0f)
	{
		float4 AnalyticFog = CalculateAnalyticFog(ViewDepth, WorldPos);
		float BlendWeight = saturate((ViewDepth - 200.0f) / (MaxFroxelZDistance - 200.0f));

		FinalScattering = lerp(FinalFog.rgb, AnalyticFog.rgb, BlendWeight);
		FinalTransmittance = lerp(FinalFog.a, AnalyticFog.a, BlendWeight);
	}
	else
	{
		FinalScattering = FinalFog.rgb;
		FinalTransmittance = FinalFog.a;
	}
	
	//float3	FinalScattering = GodRayTex.rgb;
	//float		FinalTransmittance = GodRayTex.a;
	//float3 FinalScattering = FinalFog.rgb;
	//float FinalTransmittance = FinalFog.a;
	//
	//float DistanceFade = saturate((MaxFroxelZDistance - ViewDepth) / (MaxFroxelZDistance - 80.f));
	//FinalScattering *= DistanceFade;
	//FinalTransmittance = lerp(1.f, FinalTransmittance, DistanceFade);
	
	float3	FinalColor = SceneColor.rgb * FinalTransmittance + FinalScattering;

	return float4(FinalColor, SceneColor.a);
}
