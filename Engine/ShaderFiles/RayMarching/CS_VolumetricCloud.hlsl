#include "../ShaderHeader/SH_CommonFunction.hlsli"

#define MAX_STEP			64
#define MIN_STEP			16
#define STEP_SIZE			15.f

static const float	CLOUD_HEIGHT_MIN	= { 200.f };
static const float	CLOUD_HEIGHT_MAX	= { 300.f };
static const float	CLOUD_HEIGHT_INV	= { 1.f / (CLOUD_HEIGHT_MAX - CLOUD_HEIGHT_MIN) };
									   
static const float	SPHERE_RADIUS		= { 600000.f };
static const float3 SPHERE_CENTER		= { float3(0.f, -SPHERE_RADIUS, 0.f) };
									   
static const float	CLOUD_CUTOFF		= { 0.55f	 };
static const float	EXTINCTION_COEF		= { 0.015f	 };
static const float	FOG_DENSITY			= { 0.00003f };

static const float	LOD_DISTANCE		= { 50000.f };
static const float3 RANDOM_FACTOR		= { float3(0.06711056f, 0.00583715f, 52.9829189f) };

static const uint	LIGHT_STEP_COUNT	= { 4 };
static const float	LIGHT_STEP_SIZE		= { 40.f };

Texture2D<float4>	OriginalTexture : register(t0);
Texture3D<float>	VolumeTexture	: register(t1);
Texture2D<float>	DepthTexture	: register(t2);

RWTexture2D<float4> OUTPUT			: register(u0);

cbuffer CB_FroxelConfig : register(b10)
{
	float3 FroxelGridSize;
	float SliceDepthRatio;
	
	float2 FullScreenResolution;
	float2 HalfScreenResolution;
	
	float NearZ;
	float FarZ;
	float AnalyticBlendStart;
	float AnalyticBlendEnd;
	
	float3 JitterOffset;
	float CB_FROXELPADDING;
	
	matrix PreviousViewProj;
};

cbuffer CB_VOLUMECLOUD : register(b13)
{
	float	CloudDensity;
	float3	LightDirection;
}

float Compute_GradientNoise(float2 PixelPos)
{
	return frac(RANDOM_FACTOR.z * frac(dot(PixelPos, RANDOM_FACTOR.xy)));
}

float Get_CloudDensity(float3 WorldPos, float BaseDensity)
{
	float RawDensity = VolumeTexture.SampleLevel(LinearWrap, WorldPos * 0.002f, 0).r;
	
	float CurrentHeight = distance(WorldPos, SPHERE_CENTER) - SPHERE_RADIUS;
	float HeightFraction = saturate((CurrentHeight - CLOUD_HEIGHT_MIN) * CLOUD_HEIGHT_INV);
	float HeightGradient = 4.0f * HeightFraction * (1.0f - HeightFraction);
	
	return max(0.0f, RawDensity - CLOUD_CUTOFF) * BaseDensity * HeightGradient * 100.f;
}

float2 Get_SphereIntersection(float3 _RayOrigin, float3 _RayDirection, float3 _Center, float _Radius)
{
	float3 OffsetToCenter = _RayOrigin - _Center;
	float Projection = dot(_RayDirection, OffsetToCenter);
	float DistanceSQDiff = dot(OffsetToCenter, OffsetToCenter) - (_Radius * _Radius);

	float Discriminant = (Projection * Projection) - DistanceSQDiff;
    
	if (Discriminant < 0.f)	return float2(-1.f, -1.f);

	float DiscriminantSQ = sqrt(Discriminant);
	
	return float2(-Projection - DiscriminantSQ, -Projection + DiscriminantSQ);
}

[numthreads(16, 16, 1)]
void CSMain(uint3 ID : SV_DispatchThreadID)
{
	uint Width, Height;
	OriginalTexture.GetDimensions(Width, Height);
	if (ID.x >= Width || ID.y >= Height)	return;
	
	uint2	PixelPos = ID.xy;
	float2	ScreenSpaceNDC = (float2(PixelPos) / float2(Width, Height)) * float2(2.f, -2.f) + float2(-1.f, 1.f);

	float4	ViewPos = mul(float4(ScreenSpaceNDC, 1.f, 1.f), g_matInvProj);
	float3	ViewDir = normalize(ViewPos.xyz / ViewPos.w);
	float3	RayDirection = normalize(mul(float4(ViewDir, 0.f), g_matInvView).xyz);

	float2	CloudMin = Get_SphereIntersection(g_vCamPos, RayDirection, SPHERE_CENTER, SPHERE_RADIUS + CLOUD_HEIGHT_MIN);
	float2	CloudMax = Get_SphereIntersection(g_vCamPos, RayDirection, SPHERE_CENTER, SPHERE_RADIUS + CLOUD_HEIGHT_MAX);
	
	float	CloudStartHeight = CloudMin.y, CloudEndHeight = CloudMax.y;
	if (CloudEndHeight < 0.f || CloudStartHeight < 0.f || CloudEndHeight <= CloudStartHeight)
	{
		OUTPUT[PixelPos] = OriginalTexture[PixelPos];
		return;
	}

	float	Depth = DepthTexture[PixelPos].r;
	
	float4	ClipSpace	= float4(ScreenSpaceNDC, Depth, 1.f);
	float4	ViewSpace	= mul(ClipSpace, g_matInvProj);
	float4	WorldSpace	= mul(ViewSpace / ViewSpace.w, g_matInvView);

	float	DistanceToGeometry = (Depth >= 0.9999f) ? 9999999.0f : distance(WorldSpace.xyz, g_vCamPos);
	if (DistanceToGeometry < CloudStartHeight)
	{
		OUTPUT[PixelPos] = OriginalTexture[PixelPos];
		return;
	}

	float	MaxTravelDistance = min(CloudEndHeight, DistanceToGeometry) - CloudStartHeight;
	float	DistanceRatio = saturate(CloudStartHeight / LOD_DISTANCE);
	uint	DynamicStepCount = (uint) lerp((float) MAX_STEP, (float) MIN_STEP, DistanceRatio);
	
	float	DynamicStepSize = MaxTravelDistance / float(DynamicStepCount);
	float3	CurrentPosition = g_vCamPos + (RayDirection * CloudStartHeight);
	
	float	TemporalSeed = JitterOffset.x * 1000.0f;
	float2	DynamicJitter = float2(PixelPos.x + TemporalSeed, PixelPos.y + TemporalSeed);
	float	JitterValue = Compute_GradientNoise(DynamicJitter);
	CurrentPosition += RayDirection * (DynamicStepSize * JitterValue);

	float	AccumulatedTransmittance = 1.f;
	float3	FinalColor = float3(0.f, 0.f, 0.f);
	
	float PhaseValue = Henyey_Greenstein_DualPhase(RayDirection, LightDirection, 0.8f, -0.2f, 0.5f);
	
	for (uint Step = 0; Step < DynamicStepCount; ++Step)
	{
		float Density = Get_CloudDensity(CurrentPosition, CloudDensity) ;
		
		if (Density > 0.f)
		{
			float LightAccumulatedDensity = 0.0f;
			float3 LightRayPos = CurrentPosition;
			
			for (uint lStep = 0; lStep < LIGHT_STEP_COUNT; ++lStep)
			{
				LightRayPos += LightDirection * LIGHT_STEP_SIZE;
				LightAccumulatedDensity += Get_CloudDensity(LightRayPos, CloudDensity);
			}
			float	LightTransmittance = exp(-LightAccumulatedDensity * LIGHT_STEP_SIZE * EXTINCTION_COEF * 10.f);
			
			float	OpticalDepth = Density * DynamicStepSize;
			float	PowderEffect = 1.f - exp(-OpticalDepth * 2.f);
			
			float3	BaseColor = float3(0.05f, 0.05f, 0.08f);
			float3	SunColor = float3(1.f, 1.f, 1.f);
			float3	CloudColor = BaseColor + SunColor * LightTransmittance * PhaseValue;
			
			float CurrentTransmittance = exp(-OpticalDepth * EXTINCTION_COEF);
			
			float3 LightContribution = CloudColor * (1.f + PowderEffect);
			
			FinalColor += LightContribution * (1.f - CurrentTransmittance) * AccumulatedTransmittance;
			AccumulatedTransmittance *= CurrentTransmittance;
			
			if (AccumulatedTransmittance < 0.01f) {
				AccumulatedTransmittance = 0.f;
				break;
			}
		}
		
		// Progress
		CurrentPosition += RayDirection * DynamicStepSize;
	}
	
	float FogFactor = exp(-CloudStartHeight * FOG_DENSITY);
	
	FinalColor *= FogFactor;
	AccumulatedTransmittance = lerp(1.f, AccumulatedTransmittance, FogFactor);
	
	OUTPUT[PixelPos] = float4(FinalColor + OriginalTexture[PixelPos].rgb * AccumulatedTransmittance, 1.f);
	return;
}
