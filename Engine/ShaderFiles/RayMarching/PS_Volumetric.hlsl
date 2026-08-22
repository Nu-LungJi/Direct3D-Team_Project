#include "../ShaderHeader/SH_CommonFunction.hlsli"

Texture2D DepthTexture				: register(t0);
Texture2D SceneColorTexture			: register(t1);
Texture3D AccumulatedVolumeTexture  : register(t2);

//Texture2D GodRayTexture				: register(t2);

static const float FroxelDepthExponent = 1.5f;

cbuffer CB_FroxelConfig : register(b10)
{
	float3	FroxelGridSize;
	float	SliceDepthRatio;
	
	float2	FullScreenResolution;
	float2	HalfScreenResolution;
	
	float	NearZ;
	float	FarZ;
	float	AnalyticBlendStart;
	float	AnalyticBlendEnd;
	
	float3	JitterOffset;
	float	CB_FROXELPADDING;
	
	matrix	PreviousViewProj;
};
cbuffer CB_VLFOG : register(b11)
{
	float3 FogColor;
	float	FogIntensity;
	float	FogDensity;
	float	FogNoiseScale;
	float	FogScattering; // 전방/후방 가중치
	float	FogBaseBrightness;
	
	float3	FogLightColor;
	float3	FogLightDirection;
	
	float	FogBaseHeight;
	float	FogMaxHeight;
	float	FogHeightFallOff;
	
	float	FogStartDistance;
	float	FogEndDistance;
	
	float	FogTime;
};

float ViewDepthToFroxelZ(float _Depth, float _Near, float _Far)
{
	float LinearDepth = saturate((_Depth - _Near) / max(_Far - _Near, 0.0001f));
	return pow(LinearDepth, 1.f / FroxelDepthExponent);
}

float4 Compute_AnalyticFog(float ViewDepth, float3 WorldPos)
{
	if (FogMaxHeight <= 0.0001f) return float4(0.f, 0.f, 0.f, 1.f);
	
	float HeightDistance = max(WorldPos.y - FogBaseHeight, 0.f);
	float HeightFactor	 = exp(-HeightDistance * FogHeightFallOff);
	float HeightLimit = 1.f - smoothstep(FogMaxHeight * 0.8f, FogMaxHeight, HeightDistance);

	float FogAmount = 1.f - exp(-ViewDepth * FogDensity * 0.1f * HeightFactor * HeightLimit);
	
	float FogDistanceFactor = smoothstep(FogStartDistance, max(FogEndDistance, FogStartDistance + 0.0001f), ViewDepth);
	
	FogAmount *= FogDistanceFactor;
	
	return float4(FogColor * FogIntensity * FogAmount, 1.f - FogAmount);
}

float4 Compute_AccumulatedFog(float _Depth, float2 _TexCoord)
{
	float SampleViewDepth;
	
	SampleViewDepth = _Depth >= 1.f ? FarZ : min(abs(Convert_ViewZPosByDepth(_Depth)), FarZ);
	
	float FroxelZ = ViewDepthToFroxelZ(SampleViewDepth, NearZ, FarZ);
	
	float AccumulatedTexZ = (FroxelZ * FroxelGridSize.z + 0.5f) / (FroxelGridSize.z + 1.f);

	return AccumulatedVolumeTexture.SampleLevel(LinearClamp, float3(_TexCoord, AccumulatedTexZ), 0);
}

float4 PSMain(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
	float4	SceneColor	= SceneColorTexture.Sample(PointClamp, TexCoord);
	float	DepthTex	= DepthTexture.Sample(PointClamp, TexCoord).r;
	
	float4  RayMarchedFog = Compute_AccumulatedFog(DepthTex, TexCoord);
	
	float3	WorldPos	= Convert_WorldPosByDepth(DepthTex, TexCoord).xyz;
	float	ViewDepth	= Convert_ViewZPosByDepth(DepthTex);
	
	if (DepthTex >= 1.f)	// 백버퍼 빈 공간
	{
		float3 FinalColor = SceneColor.rgb * RayMarchedFog.a + RayMarchedFog.rgb;
		return float4(FinalColor, SceneColor.a);
	}

	float4	AnalyticFog		= Compute_AnalyticFog(ViewDepth, WorldPos);
	float	AnalyticWeight	= smoothstep(AnalyticBlendStart, AnalyticBlendEnd, ViewDepth);
	
	float3	FinalScattering		= lerp(RayMarchedFog.rgb, AnalyticFog.rgb, AnalyticWeight);
	float	FinalTransmittance	= lerp(RayMarchedFog.a, AnalyticFog.a, AnalyticWeight);
	
	return float4(SceneColor.rgb * FinalTransmittance + FinalScattering, SceneColor.a);
}
