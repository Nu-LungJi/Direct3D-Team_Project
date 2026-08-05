#include "../ShaderHeader/SH_CommonFunction.hlsli"

Texture2D<float4>		SceneColorTexture : register(t0);
										  
Texture2D<float>		DepthTexture      : register(t1);
Texture2D<float>		ShadowMapTexture  : register(t2);
Texture2D<float>		BlueNoiseTexture  : register(t3);
Texture3D<float4>		VolumeTexture     : register(t4);

Texture3D<float4>		VoxelDensityColor : register(t5);
Texture3D<float4>		VoxelLightingColor : register(t6);

Texture2DArray<float>	ShadowMapArray : register(t7);
										  
RWTexture2D<float4>		OUTPUT            : register(u0);
RWTexture3D<float4>		OUTPUT3D		  : register(u1);

const static float2		ScreenResolution    = { 1280.f, 720.f };
const static float2		NoiseResolution     = { 256.f, 256.f };
const static uint		FogMaxStep          = { 32 };

cbuffer CB_FroxelConfig : register(b10)
{
	float3	FroxelGridSize; // float3(160, 90, 64)
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
	float	FogAnisotropyGA;		// 전방 산란도
	float	FogAnisotropyGB;		// 후방 산란도
	float	FogScatteringWeight;	// 전방/후방 가중치
	
	float3	FogLightColor;
	float3	FogPadding;
};

cbuffer CB_CSM : register(b12)
{
	matrix ShadowViewProj[4];
	float4 CascadeSplits;
	float2 ShadowMapSize;
	float2 ShadowBias;
};

float FroxelZToViewDepth(float _ZSlice, float _Near, float _Far, float _MaxSlice)
{
	return _Near * pow(_Far / _Near, (_ZSlice + 0.5f) / _MaxSlice);
}

float3 FroxelZToWorldPos(float3 _TexCoord)
{
	float	ViewDepth = FroxelZToViewDepth(_TexCoord.z, NearZ, FarZ, FroxelGridSize.z);
	
	float2 ScreenSpaceNDC;
	ScreenSpaceNDC.x = _TexCoord.x * +2.f - 1.f;
	ScreenSpaceNDC.y = (1.f - _TexCoord.y) * -2.f - 1.f;
	
	float3 ViewSpacePos;
	ViewSpacePos.x = ScreenSpaceNDC.x * ViewDepth / g_matProj[0][0];
	ViewSpacePos.y = ScreenSpaceNDC.y * ViewDepth / g_matProj[1][1];
	ViewSpacePos.z = ViewDepth;
	
	return mul(float4(ViewSpacePos, 1.f), g_matInvView).xyz;
}

float GetSliceDeltaZ(uint _ZSlice, float _MaxSlice, float _Near, float _Far)
{
	float NearSlice = _Near * pow(_Far / _Near, (float) _ZSlice / _MaxSlice);
	float FarSlice  = _Near * pow(_Far / _Near, ((float) _ZSlice + 1.f) / _MaxSlice);
	
	return FarSlice - NearSlice;
}

float Henyey_Greenstein_Phase(float _CosTheta, float g)
{
	float g2 = g * g;
	float Denum = 1.f + g2 - 2.f * g * _CosTheta;
	
	return (1.f - g2) / (4.f * PI * pow(max(Denum, 0.0001f), 1.5f));
}
float Henyey_Greenstein_DualPhase(float3 _RayDirection, float3 _FogLightDirection, float g1, float g2, float k)
{
	float CosTheta = dot(_RayDirection, _FogLightDirection);
	
	float PhaseValueA = Henyey_Greenstein_Phase(CosTheta, g1);
	float PhaseValueB = Henyey_Greenstein_Phase(CosTheta, g2);
    
	return lerp(PhaseValueB, PhaseValueA, k);
}

float GetVolumeFogDensity(float3 _Point)    
{
    //float FogHeight = exp(-_Point.y * 0.05f);
    float FogMaxHeight = max(0.0f, FogHeight - _Point.y);
	if (FogMaxHeight <= 0.f)	return 0.f;
    
	float HeightFactor = saturate(FogMaxHeight / FogHeight);
	
	float DistanceFromCam = length(_Point - FogCenterPos);
	float FadeRange = max(FogEndPos - FogStartPos, 0.001f);
	float DistanceFactor = saturate((DistanceFromCam - FogStartPos) / FadeRange);
	
	float3 NoiseTexCoord = (_Point - FogCenterPos) * FogNoiseScale;
    float4 NoiseSet = VolumeTexture.SampleLevel(LinearWrap, NoiseTexCoord, 0.f);
    
    float MainNoise = NoiseSet.r;
    float SubNoise = NoiseSet.g * 0.5f + NoiseSet.b * 0.3f + NoiseSet.a * 0.2f;
    float FinalNoise = saturate(MainNoise * 0.7f + SubNoise * 0.3f);

	return HeightFactor * FinalNoise * FogDensity * DistanceFactor;
}

float Compute_ShadowBrightness(float4 _Position)
{
    // ViewSpace Pos From ShadowCam
    float4 ShadowSpacePos = mul(_Position, g_matShadowLightViewProj);
    float2 ShadowMapUV;
    ShadowMapUV.x = (ShadowSpacePos.x) * +0.5f + 0.5f;
    ShadowMapUV.y = (ShadowSpacePos.y) * -0.5f + 0.5f;
            
    float DepthFromShadowCam = ShadowSpacePos.z;
            
    float ShadowBrightness = 1.f; // 최대 밝기 (1.f = 그림자가 안 지는 픽셀의 값)
    
    [branch]
    if (ShadowMapUV.x >= 0.0f && ShadowMapUV.x <= 1.0f && ShadowMapUV.y >= 0.0f && ShadowMapUV.y <= 1.0f)
    {
        // Compare Depth (DepthFromShadowCam : ShadowMapTexture Depth)
        // (DepthFromShadowCam < ShadowMapTexture Depth) : 1 ~ No Shadow
        // (DepthFromShadowCam > ShadowMapTexture Depth) : 0 ~ Cascade Shadow
        float ShadowFactor = ShadowMapTexture.SampleCmpLevelZero(ShadowSampler, ShadowMapUV, DepthFromShadowCam + 0.002f);

        //lerp(0.15f, 1.0f, ShadowFactor);
        ShadowBrightness = pow(ShadowFactor, 3.0f); // ShadowBrightness : 그림자의 밝기(대부분 1.f or 0.f)
    }
    return ShadowBrightness;
}

float Compute_CascadeShadow(float3 _WorldPos)
{
	float4 ViewPos = mul(float4(_WorldPos, 1.0f), g_matView);
	float ViewDepth = ViewPos.z;
	
	int CascadeIndex = 0;
	if (ViewDepth < CascadeSplits.x)
		CascadeIndex = 0;
	else if (ViewDepth < CascadeSplits.y)
		CascadeIndex = 1;
	else if (ViewDepth < CascadeSplits.z)
		CascadeIndex = 2;

	float4 ShadowSpacePos = mul(float4(_WorldPos, 1.f), ShadowViewProj[CascadeIndex]);
	ShadowSpacePos.xyz /= ShadowSpacePos.w;
	
	float2 ShadowMapUV;
	ShadowMapUV.x = ShadowSpacePos.x * +0.5f + 0.5f;
	ShadowMapUV.y = ShadowSpacePos.y * -0.5f + 0.5f;
	
	if (ShadowMapUV.x < 0.f || ShadowMapUV.x > 1.f ||
        ShadowMapUV.y < 0.f || ShadowMapUV.y > 1.f ||
		ShadowSpacePos.z > 1.f || ShadowSpacePos.z < 0.f)
	{
		return 1.f;
	}
	
	float CurrentDepth = ShadowSpacePos.z - ShadowBias.x;
	float ShadowFactor = 0.f;
	
	float2 TexelSize = 1.f / ShadowMapSize;
	
	[unroll]
	for (int x = -1; x <= 1; ++x)
	{
        [unroll]
		for (int y = -1; y <= 1; ++y)
		{
			float2 Offset = float2(x, y) * TexelSize;
            
			ShadowFactor += ShadowMapArray.SampleCmpLevelZero(ShadowSampler,
			float3(ShadowMapUV + Offset, (float)CascadeIndex), CurrentDepth);
		}
	}
	
	return ShadowFactor / 9.f;
}


[numthreads(8, 8, 8)]
void CSMain_CellInjection(uint3 ID : SV_DispatchThreadID)
{
	if (ID.x >= (uint) FroxelGridSize.x || ID.y >= (uint) FroxelGridSize.y || ID.z >= (uint) FroxelGridSize.z)	return;

	float3	TexCoord = (float3(ID.xyz) + 0.5f) / FroxelGridSize;
	
	float3	VoxelWorldPos = FroxelZToWorldPos(TexCoord);
	float	FogDensity = GetVolumeFogDensity(VoxelWorldPos);
	
	OUTPUT3D[ID] = float4(FogColor, FogDensity);
	return;
}

[numthreads(8, 8, 8)]
void CSMain_LightIntegration(uint3 ID : SV_DispatchThreadID)
{
	if (ID.x >= (uint) FroxelGridSize.x || ID.y >= (uint) FroxelGridSize.y || ID.z >= (uint) FroxelGridSize.z)
		return;
	
	float4	DensityData = VoxelDensityColor[ID];
	float	Density = DensityData.a;
	
	if (Density <= 0.0001f)
	{
		OUTPUT3D[ID] = float4(0.0f, 0.0f, 0.0f, 0.0f);
		return;
	}
	
	float3	TexCoord = (float3(ID.xyz) + 0.5f) / FroxelGridSize;
	float3	VoxelWorldPos = FroxelZToWorldPos(TexCoord);
	float3	RayDirection  = normalize(VoxelWorldPos - g_vCamPos);
	
	float	PhaseValue = Henyey_Greenstein_DualPhase(RayDirection, FogLightDirection, FogAnisotropyGA, FogAnisotropyGB, FogScatteringWeight);
	
	float	ShadowBrightness = 1.f;
	//ShadowBrightness = Compute_ShadowBrightness(float4(VoxelWorldPos, 1.0f));
	
	float3	Scattering = DensityData.rgb * FogLightColor * Density * PhaseValue * ShadowBrightness;
	float	Extinction = Density;
	
	OUTPUT3D[ID] = float4(Scattering, Extinction);
	return;
}

[numthreads(16, 16, 1)]
void CSMain_FroxelZAccumulation(uint3 ID : SV_DispatchThreadID)
{
	if (ID.x >= (uint) FroxelGridSize.x || ID.y >= (uint) FroxelGridSize.y)
		return;
	
	float3	AccumulatedScattering = float3(0.0f, 0.0f, 0.0f);
	float	AccumulatedTransmittance = 1.0f;

	[loop]
	for (uint z = 0; z < (uint) FroxelGridSize.z; ++z)
	{
		uint3	VoxelCoord = uint3(ID.xy, z);
		float4	LightingData = VoxelLightingColor[VoxelCoord];
		
		float3	Scattering = LightingData.rgb;
		float	Extinction = LightingData.a;
		
		float	RayStepSize = GetSliceDeltaZ(z, FroxelGridSize.z, NearZ, FarZ);
		
		float	StepExtinction = Extinction * RayStepSize;
		float	StepTransmittance = exp(-StepExtinction);
		
		float3 IntegratedScattering = float3(0.f, 0.f, 0.f);
		
        [branch]
		if (Extinction > 0.0001f)
		{
			IntegratedScattering = (Scattering / Extinction) * (1.f - StepTransmittance);
		}
		
		AccumulatedScattering += IntegratedScattering * AccumulatedTransmittance;
		AccumulatedTransmittance *= StepTransmittance;
		
		[branch]
		if (AccumulatedTransmittance < 0.001f)
		{
			AccumulatedTransmittance = 0.0f;
		}
		
		OUTPUT3D[VoxelCoord] = float4(AccumulatedScattering, AccumulatedTransmittance);
	}
	
	return;
}
[numthreads(16, 16, 1)]
void CSMain_RayMarching(uint3 ID : SV_DispatchThreadID)
{
	float2	TexCoord = (float2(ID.xy) + 0.5f) / ScreenResolution;
	float	Depth	 = DepthTexture.Sample(PointClamp, TexCoord).r;
	float3	WorldPos = Convert_WorldPosByDepth(Depth, TexCoord);
	
	float3	RayStart = g_vCamPos;
	float3	RayEnd	 = WorldPos;
	
	float3	RayVector	 = RayEnd - RayStart;
	float3	RayDirection = normalize(RayVector);
	float	RayLength	 = length(RayVector);
	
	const static uint MaxStep = 32;
	float	StepSize = RayLength / (float) MaxStep;
	
	float3	LightAccumulation = float3(0.0f, 0.0f, 0.0f);
	float	AccumulatedTransmittance = 1.f;
	
	for (uint i = 0; i < MaxStep; ++i)
	{
		float	SampleDistance = (i + 0.5f) * StepSize;
		float3	SamplePosition = RayStart + RayDirection * SampleDistance;
		
		float	ShadowBrightness = Compute_CascadeShadow(SamplePosition);
		
		if (ShadowBrightness > 0.001f)
		{
			float	Density = GetVolumeFogDensity(SamplePosition);
			float	PhaseValue = Henyey_Greenstein_DualPhase(RayDirection, -FogLightDirection, FogAnisotropyGA, FogAnisotropyGB, FogScatteringWeight);
		
			float3	Scattering = FogLightColor * Density * PhaseValue * ShadowBrightness;
			float	Extinction = Density;
			
			float StepTransmittance = exp(-Extinction * StepSize);
			LightAccumulation += Scattering * (1.f - StepTransmittance) * AccumulatedTransmittance;
			AccumulatedTransmittance *= StepTransmittance;
		}
		
	}
	OUTPUT[ID.xy] = float4(LightAccumulation, AccumulatedTransmittance);
	return;
}
