#include "../ShaderHeader/SH_CommonFunction.hlsli"

Texture2D DepthTexture				: register(t0);
Texture2D SceneColorTexture			: register(t1);
Texture3D VoxelAccumulatedTexture	: register(t2);
Texture2D GodRayTexture				: register(t3);

cbuffer CB_FroxelConfig : register(b10)
{
	float3	FroxelGridSize;
	float	NearZ;
	float	FarZ;
	float3	Padding;
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
	float	FogLightIntensity;
	
	float	FogAnisotropyGA; // 전방 산란도
	float	FogAnisotropyGB; // 후방 산란도
	float	FogScatteringWeight; // 전방/후방 가중치
	
	float3	FogLightColor;

	float2	FogPadding;
};


float ViewDepthToFroxelZ(float _Depth, float _Near, float _Far, float _MaxSlice)
{
	float ZSlice = log(max(_Depth, _Near) / _Near) / log(_Far / _Near) * _MaxSlice;
	return ZSlice / _MaxSlice;
}

float4 PSMain(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
	float4	SceneColor = SceneColorTexture.Sample(PointClamp, TexCoord);
	float	Depth = DepthTexture.Sample(PointClamp, TexCoord).r;
	
	float	ViewDepth = Convert_ViewZPosByDepth(Depth);
	
	float	FroxelTexCoordZ = ViewDepthToFroxelZ(ViewDepth, NearZ, FarZ, FroxelGridSize.z);
	float3	FroxelTexCoord = float3(TexCoord, saturate(FroxelTexCoordZ));
	
	float4	FogData = VoxelAccumulatedTexture.Sample(LinearClamp, FroxelTexCoord);

	float4	GodRayTex = GodRayTexture.Sample(LinearClamp, TexCoord);
	
	float3	FinalScattering = FogData.rgb * GodRayTex.rgb * FogLightIntensity;
	float	FinalTransmittance = FogData.a * GodRayTex.a;

	float3	FinalColor = SceneColor.rgb * FinalTransmittance + FinalScattering * FogIntensity;

	return float4(FinalColor, SceneColor.a);
}
