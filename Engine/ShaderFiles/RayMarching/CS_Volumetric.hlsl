#include "../ShaderHeader/SH_CommonFunction.hlsli"
											
Texture2D<float>		DepthTexture			: register(t0);
Texture2D<float>		BlueNoiseTexture		: register(t1);
Texture3D<float4>		VolumeTexture			: register(t2);
Texture3D<float4>		VoxelLightingColor		: register(t3);
Texture2DArray<float>	ShadowMapArray			: register(t4);

Texture2DArray<float>	DynamicShadowMaps		: register(t10);
TextureCubeArray<float> DynamicShadowCubeMaps	: register(t12);

RWTexture2D<float4>		OUTPUT					: register(u0);
RWTexture3D<float4>		OUTPUT3D				: register(u1);

static const float		GodRayStrength				= { 3.5f };

static const float		SpotVolumetricShadowBias	= { 0.0001f };
static const float		PointVolumetricShadowBias	= { 0.002f };

static const float3		BaseWindDirection			= float3(1.f, 0.1f, 0.4f);
static const float3		DetailWindDirection			= float3(-0.4f, 0.25f, 1.f);

static const float		BaseFlowSpeed				= { 0.10f };
static const float		DetailFlowSpeed				= { 0.22f };

static const float		LocalScatteringStrength		= { 2.f };
static const float		FroxelDepthExponent			= { 1.5f };
static const float2		PCFOffsets[4] =
{
	float2(-0.5f, -0.5f),
    float2(+0.5f, -0.5f),
    float2(-0.5f, +0.5f),
    float2(+0.5f, +0.5f)
};

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
	
	float3	FogLightColor;
	float	FogAnisotropyGB;		// 후방 산란도
	
	float	FogScatteringWeight;	// 전방/후방 가중치
	
	float	FogBaseHeight;
	float	FogHeightFallOff;
	float	FogTime;
};

cbuffer CB_CSM : register(b12)
{
	matrix	ShadowViewProj[4];
	float4	CascadeSplits;
	float2	ShadowMapSize;
	float2	ShadowBias;
};

float ViewDepthToFroxelZ(float _Depth, float _Near, float _Far)
{
	float LinearDepth = saturate((_Depth - _Near) / max(_Far - _Near, 0.0001f));
	return pow(LinearDepth, 1.f / FroxelDepthExponent);
}

float3 FroxelZToWorldPos(float3 _TexCoord)
{
	float ViewDepth = lerp(NearZ, FarZ, pow(saturate(_TexCoord.z), FroxelDepthExponent));
	
	float2 ScreenSpaceNDC;
	ScreenSpaceNDC.x = _TexCoord.x * +2.f - 1.f;
	ScreenSpaceNDC.y = _TexCoord.y * -2.f + 1.f;
	
	float3 ViewSpacePos;
	ViewSpacePos.x = ScreenSpaceNDC.x * ViewDepth / g_matProj[0][0];
	ViewSpacePos.y = ScreenSpaceNDC.y * ViewDepth / g_matProj[1][1];
	ViewSpacePos.z = ViewDepth;
	
	return mul(float4(ViewSpacePos, 1.f), g_matInvView).xyz;
}

float Henyey_Greenstein_Phase(float _CosTheta, float _Anistropy)
{
	float Anistropy2 = _Anistropy * _Anistropy;
	float Denum = 1.f + Anistropy2 - 2.f * _Anistropy * _CosTheta;
	
	return (1.f - Anistropy2) / (4.f * PI * pow(max(Denum, 0.0001f), 1.5f)); 
}
float Henyey_Greenstein_DualPhase(float3 _RayDirection, float3 _FogLightDirection, float _FrontAnistropy, float _BackAnistropy, float k)
{
	float CosTheta = dot(_RayDirection, -_FogLightDirection);
	
	float PhaseValueA = Henyey_Greenstein_Phase(CosTheta, _FrontAnistropy);
	float PhaseValueB = Henyey_Greenstein_Phase(CosTheta, _BackAnistropy);
    
	return lerp(PhaseValueB, PhaseValueA, k);
}

float Compute_FogFlow(float3 _WorldPos)
{
	float3	NoiseCoord	= (_WorldPos - FogCenterPos) * FogNoiseScale;
	float3	BaseCoord	= (NoiseCoord + BaseWindDirection) * (FogTime * BaseFlowSpeed);
	float3	DetailCoord = (NoiseCoord * 2.7f + DetailWindDirection) * (FogTime * DetailFlowSpeed);
	
	float	BaseNoise	= VolumeTexture.SampleLevel(LinearWrap, BaseCoord, 0.f).r;
	float	DetailNoise	= VolumeTexture.SampleLevel(LinearWrap, DetailCoord, 0.f).g;
	
	return smoothstep(0.25f, 0.75f, BaseNoise * 0.7f + DetailNoise * 0.3f);
}

float GetVolumeFogDensity(float3 _WorldPos)    
{
	if (FogHeight <= 0.0001f)	return 0.f;
	
	float	FogMaxHeight = max(0.f, _WorldPos.y - FogBaseHeight);

	float	HeightFactor = exp(-FogMaxHeight * FogHeightFallOff);
	
	float	HeightLimit = 1.f - smoothstep(FogHeight * 0.8f, FogHeight, FogMaxHeight);
	
	float	FinalFlowNoise = Compute_FogFlow(_WorldPos);

	return HeightFactor * FogDensity * HeightLimit;// * FinalFlowNoise;
}

float Sample_CascadeShadow(float3 _WorldPos, uint _CascadeIndex)
{
	float4 ShadowSpacePos = mul(float4(_WorldPos, 1.f), ShadowViewProj[_CascadeIndex]);
	ShadowSpacePos.xyz /= ShadowSpacePos.w;
	
	float2 ShadowMapUV;
	ShadowMapUV.x = ShadowSpacePos.x * +0.5f + 0.5f;
	ShadowMapUV.y = ShadowSpacePos.y * -0.5f + 0.5f;
	
	if (ShadowMapUV.x < 0.f || ShadowMapUV.x > 1.f ||
        ShadowMapUV.y < 0.f || ShadowMapUV.y > 1.f ||
		ShadowSpacePos.z > 1.f || ShadowSpacePos.z < 0.f)
		return 1.f;

	float	CurrentDepth = ShadowSpacePos.z - ShadowBias.x;
	
	float2	TexelSize = 1.f / max(ShadowMapSize, float2(1.f, 1.f));

	float	ShadowFactor = 0.f;
	
	[unroll]
	for (uint i = 0; i < 4; ++i)
	{
		float2 SampleUV = ShadowMapUV + TexelSize * PCFOffsets[i];
		ShadowFactor += ShadowMapArray.SampleCmpLevelZero(ShadowSampler, float3(SampleUV, _CascadeIndex), CurrentDepth);
	}
	
	return	ShadowFactor * 0.25f;
}

float Compute_CascadeShadow(float3 _WorldPos)
{
	float4	ViewPos = mul(float4(_WorldPos, 1.f), g_matView);
	float	ViewDepth = abs(ViewPos.z);
	
	if (CascadeSplits.w <= 0.f || ViewDepth >= CascadeSplits.w)	return 1.f;
	
	int CascadeIndex = 3;
	if		(ViewDepth < CascadeSplits.x)	CascadeIndex = 0;
	else if (ViewDepth < CascadeSplits.y)	CascadeIndex = 1;
	else if (ViewDepth < CascadeSplits.z)	CascadeIndex = 2;
	else if (ViewDepth < CascadeSplits.w)	CascadeIndex = 3;

	float CurrentShadow = Sample_CascadeShadow(_WorldPos, CascadeIndex);
	
	if (CascadeIndex >= 3)	return CurrentShadow;
	
	float CascadeNear = CascadeIndex == 0 ? 0.f : CascadeSplits[CascadeIndex - 1];
	float CascadeFar  = CascadeSplits[CascadeIndex];
	
	float CascadeBlendStart = lerp(CascadeNear, CascadeFar, 0.85f);
	
	if (ViewDepth <= CascadeBlendStart)	return CurrentShadow;
	
	float NextShadow = Sample_CascadeShadow(_WorldPos, CascadeIndex + 1);
	
	float BlendWeight = smoothstep(CascadeBlendStart, CascadeFar, ViewDepth);
	
	return lerp(CurrentShadow, NextShadow, BlendWeight);
}

float Compute_PointVolumetricShadow(float3 _WorldPos, uint _LightIndex)
{
	int ShadowSlotNumb = AffectedLight[_LightIndex].ShadowSlot;
	
	if (ShadowSlotNumb < 0 || ShadowSlotNumb >= MAX_SHADOW_LIGHT_COUNT) return 1.f;

	float3	LightToVoxel = _WorldPos - AffectedLight[_LightIndex].Position;
	float	DistanceSQ	 = dot(LightToVoxel, LightToVoxel);
	
	if (DistanceSQ <= 0.0001f) return 1.f;
	
	float OuterRange = max(AffectedLight[_LightIndex].OuterAttanuation, 0.02f);
	
	if (DistanceSQ >= OuterRange * OuterRange)	return 1.f;

	float InvDistance = rsqrt(DistanceSQ);
	float Distance = DistanceSQ * InvDistance;

	float3 SampleDirection = LightToVoxel * InvDistance;
	
	float CurrentDepth = saturate(Distance / OuterRange - PointVolumetricShadowBias);
	
	float3 BaseUp = abs(SampleDirection.z) < 0.999f ? float3(0.f, 0.f, 1.f) : float3(1.f, 0.f, 0.f);
	
	float3 TangentX = normalize(cross(SampleDirection, BaseUp));
	float3 TangentY = normalize(cross(SampleDirection, TangentX));
	
	float FilterRadius = 2.5f / POINTLIGHT_RESOLUTION;

	float ShadowFactor = 0.f;
	
	[unroll]
	for (uint i = 0; i < 4; ++i)
	{
		float2 Offset = PCFOffsets[i];

		float3 OffsetDirection = normalize(SampleDirection + TangentX * Offset.x * FilterRadius + TangentY * Offset.y * FilterRadius);
		
		ShadowFactor += DynamicShadowCubeMaps.SampleCmpLevelZero(ShadowSampler, float4(OffsetDirection, ShadowSlotNumb), CurrentDepth);
	}

	return ShadowFactor * 0.25f;
	
}
float Compute_SpotVolumetricShadow(float3 _WorldPos, uint _LightIndex)
{
	int ShadowSlotNumb = AffectedLight[_LightIndex].ShadowSlot;
	
	if (ShadowSlotNumb < 0 || ShadowSlotNumb >= MAX_SHADOW_LIGHT_COUNT) return 1.f;

	float4 LightClipPos = mul(float4(_WorldPos, 1.f), AffectedLight[_LightIndex].g_LightViewProj[0]);
	if (LightClipPos.w <= 0.0001f) return 1.f;

	float3 LightNDC = LightClipPos.xyz / LightClipPos.w;
	
	if (LightNDC.x < -1.f || LightNDC.x > 1.f ||
        LightNDC.y < -1.f || LightNDC.y > 1.f ||
        LightNDC.z < 0.f || LightNDC.z > 1.f) return 1.f;
	
	float2 ShadowMapUV;
	ShadowMapUV.x = LightNDC.x * 0.5f + 0.5f;
	ShadowMapUV.y = LightNDC.y * -0.5f + 0.5f;
	
	float CurrentDepth = saturate(LightNDC.z - SpotVolumetricShadowBias);

	float2 TexelSize = 1.f / float2(SPOTLIGHT_RESOLUTION, SPOTLIGHT_RESOLUTION);

	float ShadowFactor = 0.f;
	
	[unroll]
	for (uint i = 0; i < 4; i++)
	{
		float2 SampleUV = ShadowMapUV + TexelSize * PCFOffsets[i];
		ShadowFactor += DynamicShadowMaps.SampleCmpLevelZero(ShadowSampler, float3(SampleUV, ShadowSlotNumb), CurrentDepth);
	}

	return ShadowFactor * 0.25f;
}

float3 Compute_LocalScattering(float3 _WorldPos, float3 _RayDirection, float _Density)
{
	float3 LocalScattering = 0.f;
	
	[loop]
	for (uint i = 0; i < LightCount; ++i)
	{
		DynamicLight Light = AffectedLight[i];
		
		if (Light.LightType == LIGHT_DIRECTIONAL || Light.VolumetricIntensity <= 0.f) continue;
		
		float3 L, Radiance;
		
		if (!Compute_DynamicLight(_WorldPos, Light, L, Radiance))	continue;

		float ShadowFactor = 1.f;
		
		if (Light.ShadowSlot >= 0)
		{
			ShadowFactor = Light.LightType == LIGHT_POINT ? Compute_PointVolumetricShadow(_WorldPos, i) : Compute_SpotVolumetricShadow(_WorldPos, i);
		}
		
		float CosTheta = dot(_RayDirection, L);
		float Phase = Henyey_Greenstein_Phase(CosTheta, FogAnisotropyGA);
		
		LocalScattering += FogColor * Radiance * _Density * Phase * ShadowFactor * Light.VolumetricIntensity * LocalScatteringStrength;
	}
	
	return LocalScattering;
}

[numthreads(8, 8, 8)]
void CSMain_LightIntegration(uint3 ID : SV_DispatchThreadID)
{
	if (ID.x >= (uint) FroxelGridSize.x || ID.y >= (uint) FroxelGridSize.y || ID.z >= (uint) FroxelGridSize.z)	return;
	
	float3	TexCoord = (float3(ID.xyz) + 0.5f) / FroxelGridSize;
	
	float3	VoxelWorldPos = FroxelZToWorldPos(TexCoord);
	float	FogDensity	  = GetVolumeFogDensity(VoxelWorldPos);
	
	float	FogDistance		  = abs(mul(float4(VoxelWorldPos, 1.f), g_matView).z);
	float	FogDistanceFactor = smoothstep(FogStartPos, max(FogEndPos, FogStartPos + 0.0001f), FogDistance);
	
	FogDensity *= FogDistanceFactor;
	
	if (FogDensity <= 0.0001f)
	{
		OUTPUT3D[ID] = float4(0.f, 0.f, 0.f, 0.f);
		return;
	}
	
	float3	RayDirection  = normalize(VoxelWorldPos - g_vCamPos);
	
	float	PhaseValue = Henyey_Greenstein_DualPhase(RayDirection, FogLightDirection, FogAnisotropyGA, FogAnisotropyGB, FogScatteringWeight);
	
	float ShadowBrightness = Compute_CascadeShadow(VoxelWorldPos);
	
	float3	DirectScattering	= FogColor * FogLightColor * FogIntensity * FogDensity * ShadowBrightness * PhaseValue * GodRayStrength;
	float3	AmbientScattering	= FogColor * FogDensity * 0.002f;
	float3	LocalScattering		= Compute_LocalScattering(VoxelWorldPos, RayDirection, FogDensity);
	
	float3	Scattering = DirectScattering + AmbientScattering + LocalScattering;
	float	Extinction = FogDensity;

	OUTPUT3D[ID] = float4(Scattering, Extinction);
	return;
}

[numthreads(8, 8, 1)]
void CSMain_FroxelZAccumulation(uint3 ID : SV_DispatchThreadID)
{
	if (ID.x >= (uint) FroxelGridSize.x || ID.y >= (uint) FroxelGridSize.y)	return;
	float2	TexCoord = (float2(ID.xy) + 0.5f) / FroxelGridSize.xy;
	
	float2	ScreenSpaceNDC;
	ScreenSpaceNDC.x = TexCoord.x * +2.f - 1.f;
	ScreenSpaceNDC.y = TexCoord.y * -2.f + 1.f;
	
	float3	ViewRayDirection = normalize(float3(
		ScreenSpaceNDC.x / g_matProj[0][0],
		ScreenSpaceNDC.y / g_matProj[1][1],
		1.f));
	
	float	ViewRayZ = max(abs(ViewRayDirection.z), 0.0001f);
	
	float3	AccumulatedScattering = float3(0.f, 0.f, 0.f);
	float	AccumulatedTransmittance = 1.f;
	
	OUTPUT3D[uint3(ID.xy, 0)] = float4(0.f, 0.f, 0.f, 1.f);

	float	SliceNear = NearZ;
	for (uint z = 0; z < (uint) FroxelGridSize.z; ++z)
	{
		float	NormalizedFarZ = ((float) z + 1.f) / FroxelGridSize.z;
		
		float	SliceFar = lerp(NearZ, FarZ, pow(NormalizedFarZ, FroxelDepthExponent));
		
		float	RayStepSize = (SliceFar - SliceNear) / ViewRayZ;
		float4	LightingData = VoxelLightingColor.Load(int4(ID.xy, z, 0));
		
		float3	Scattering = LightingData.rgb;
		float	Extinction = LightingData.a;
		
		float	StepTransmittance = exp(-Extinction * RayStepSize);
		float3	IntegratedScattering = Extinction > 0.0001f ? (Scattering / Extinction) * (1.f - StepTransmittance) : Scattering * RayStepSize;
		
		float3 StepContribution = IntegratedScattering * AccumulatedTransmittance;

		AccumulatedScattering += StepContribution;
		AccumulatedTransmittance *= StepTransmittance;
		
		OUTPUT3D[uint3(ID.xy, z + 1)] = float4(AccumulatedScattering, AccumulatedTransmittance);
		
		SliceNear = SliceFar;
	}
}

[numthreads(16, 16, 1)]
void CSMain_RayMarching(uint3 ID : SV_DispatchThreadID)
{
	if (ID.x >= (uint) FullScreenResolution.x || ID.y >= (uint) FullScreenResolution.y)
		return;
	
	float2	TexCoord = (float2(ID.xy) + 0.5f) / FullScreenResolution;
	float	Depth = DepthTexture.SampleLevel(PointClamp, TexCoord, 0).r;
	float3	WorldPos = Convert_WorldPosByDepth(Depth, TexCoord).xyz;
	
	float	PixelViewZ = abs(mul(float4(WorldPos, 1.f), g_matView).z);
	
	float3	RayVector = WorldPos - g_vCamPos;
	float	RayLength = length(RayVector);
	
	if (RayLength <= 0.0001f)
	{
		OUTPUT[ID.xy] = float4(0.f, 0.f, 0.f, 1.f);
		return;
	}
	
	float3	RayDirection = RayVector / RayLength;

	float	MaxViewDepth = min(PixelViewZ, FarZ);

	float3	ViewRayDirection = mul(float4(RayDirection, 0.f), g_matView).xyz;

	float	RayViewZ = max(abs(ViewRayDirection.z), 0.0001f);

	float3	AccumulatedScattering = float3(0.f, 0.f, 0.f);
	float	AccumulatedTransmittance = 1.f;
	
	float	SliceNear = NearZ;
	
	[loop]
	for (uint z = 0; z < (uint) FroxelGridSize.z; ++z) {
		float NormalizedFarZ = ((float) z + 1.f) / FroxelGridSize.z;
		
		float SliceFar = lerp(NearZ, FarZ, pow(NormalizedFarZ, FroxelDepthExponent));
		
		if (SliceNear >= MaxViewDepth)	break;

		SliceFar = min(SliceFar, MaxViewDepth);
		float	RayStepSize = (SliceFar - SliceNear) / RayViewZ;
		
		float	SampleFroxelZ = ((float) z + 0.5f) / FroxelGridSize.z;
		float3	SampleFroxelTexCoord = float3(TexCoord, saturate(SampleFroxelZ));

		float4	LightingData = VoxelLightingColor.SampleLevel(LinearClamp, SampleFroxelTexCoord, 0);

		float3	Scattering = LightingData.rgb;
		float	Extinction = LightingData.a;

		float	StepTransmittance = exp(-Extinction * RayStepSize);
		float3	IntegratedScattering = Extinction > 0.0001f ? (Scattering / Extinction) * (1.f - StepTransmittance) : Scattering * RayStepSize;
		float3	StepContribution	 = IntegratedScattering * AccumulatedTransmittance;

		AccumulatedScattering += StepContribution;
		AccumulatedTransmittance *= StepTransmittance;

		if (AccumulatedTransmittance < 0.001f) {
			AccumulatedTransmittance = 0.f;
			break;
		}
		
		SliceNear = SliceFar;
	}

	OUTPUT[ID.xy] = float4(AccumulatedScattering, AccumulatedTransmittance);
	return;
}
