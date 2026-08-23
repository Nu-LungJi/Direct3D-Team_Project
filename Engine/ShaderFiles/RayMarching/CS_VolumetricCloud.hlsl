#include "../ShaderHeader/SH_CommonFunction.hlsli"

#define MAX_STEP			64
#define MIN_STEP			16
#define STEP_SIZE			15.f

static const float	MS_SCATTERING_MULTIPLIER = 0.5f;
static const float	MS_EXTINCTION_MULTIPLIER = 0.5f;
static const float	MS_PHASE_MULTIPLIER		 = 0.5f;

static const float	SPHERE_RADIUS		= { 600000.f };
static const float3 SPHERE_CENTER		= { float3(0.f, -SPHERE_RADIUS, 0.f) };
									   
static const float	FOG_DENSITY			= { 0.0001f };

static const float3 RANDOM_FACTOR		= { float3(0.06711056f, 0.00583715f, 52.9829189f) };

static const uint	LIGHT_STEP_COUNT	= { 4 };
static const float	LIGHT_STEP_SIZE		= { 40.f };

static const float3 GLOBAL_SUN_COLOR = { float3(0.6f, 0.6f, 0.8f) };

static const int2 Offsets[4] = { int2(0, -1), int2(0, 1), int2(-1, 0), int2(1, 0) };
	
Texture2D<float4>	OriginalTexture		: register(t0);
Texture2D<float4>	HistoryTexture		: register(t1);
Texture3D<float>	VolumeTexture		: register(t2);
Texture2D<float>	DepthTexture		: register(t3);
Texture2D<float4>	WeatherMapTexture	: register(t4);


RWTexture2D<float4> OUTPUT				: register(u0);

cbuffer CB_CLOUDTAA : register(b9)
{
	matrix CloudJitterInvProj;
	matrix CloudPrevViewProj;
	
	float2 ScreenResolution;
	float2 InvScreenResolution;
}

cbuffer CB_VOLUMECLOUD : register(b13)
{
	float3	WindDirection;
	float	WindTimeAccumulation;
	
	float3	CloudColor;
	float	CloudBrightness;
	
	float	CloudCoverage;
	float	CloudDensity;
	float	CloudScattering;

	float	BaseCloudNoiseScale;
	float	DetailCloudNoiseScale;
	
	float	CloudMinHeight;
	float	CloudMaxHeight;
	float	CloudLODDistance;
	
	float3	CloudLightDirection;
	float	LightAbsorption;
	
	float3	CloudJitterOffset;
	float	CB_VOLUMECLOUD_PADDING;
}
  
float Compute_GradientNoise(float2 PixelPos)
{
	return frac(RANDOM_FACTOR.z * frac(dot(PixelPos, RANDOM_FACTOR.xy)));
}
float Remap(float originalValue, float originalMin, float originalMax, float newMin, float newMax)
{
	return newMin + (((originalValue - originalMin) / (originalMax - originalMin)) * (newMax - newMin));
}
float3 Compute_MultipleScattering(float3 _RayDirection, float3 _LightDirection, float _OpticalDepth, float _BaseScattering)
{
	float3 MultipleScatteringTotal = float3(0.f, 0.f, 0.f);
    
	float CurrentWeight = 1.f;
	float CurrentOpticalDepth = _OpticalDepth;
    
	float G1 = 0.8f;
	float G2 = -0.2f;
	
    [unroll]
	for (int i = 0; i < 3; ++i)
	{
		float Transmittance = exp(-CurrentOpticalDepth);
        
		float Phase = Henyey_Greenstein_DualPhase(_RayDirection, _LightDirection, G1, G2, _BaseScattering);
        
		MultipleScatteringTotal += CurrentWeight * Transmittance * Phase;
        
		CurrentWeight *= MS_SCATTERING_MULTIPLIER;
		CurrentOpticalDepth *= MS_EXTINCTION_MULTIPLIER;
		G1 *= MS_PHASE_MULTIPLIER;
		G2 *= MS_PHASE_MULTIPLIER;
	}
    
	return MultipleScatteringTotal;
}

float Get_CloudDensity(float3 WorldPos, float BaseDensity)
{
	float3	WindOffset = WindDirection * WindTimeAccumulation;
	float3	MovedPos = WorldPos + WindOffset;
	
	float2	WeatherOffset = float2(90000.f, -2000.f);
	float2	WeatherMapUV = (MovedPos.xz + WeatherOffset) * 0.0000095f;
	float3	WeatherMapTex = WeatherMapTexture.SampleLevel(LinearWrap, WeatherMapUV, 0).rgb;
	
	float	WeatherCoverage = smoothstep(0.05f, 0.55f, WeatherMapTex.r);
	float	WeatherType		= smoothstep(0.0f, 0.6f, WeatherMapTex.g) * 0.8f;
	float	WeatherDensity	= WeatherMapTex.b; 
	
	float	BaseNoise	= VolumeTexture.SampleLevel(LinearWrap, MovedPos * BaseCloudNoiseScale, 0).r;
	float	DetailNoise = VolumeTexture.SampleLevel(LinearWrap, MovedPos * DetailCloudNoiseScale, 0).r;
	float	RawDensity = BaseNoise * 0.8f + DetailNoise * 0.2f;
	
	float	ErodedNoise = saturate(Remap(BaseNoise, DetailNoise * 0.2f, 1.f, 0.f, 1.f));
	
	float3	PlanetCenter = float3(g_vCamPos.x, -SPHERE_RADIUS, g_vCamPos.z);
	float	CurrentHeight = distance(WorldPos, PlanetCenter) - SPHERE_RADIUS;
	
	float	DynamicMaxHeight = lerp(CloudMinHeight + 0.f, CloudMaxHeight, WeatherType);
	
	float	HeightRange = max(1.f, DynamicMaxHeight - CloudMinHeight);
	float	HeightFraction = saturate((CurrentHeight - CloudMinHeight) / HeightRange);
	
	float	HeightGradient = 4.f * HeightFraction * (1.f - HeightFraction);
	
	float	FinalCoverage = CloudCoverage * WeatherCoverage;
	float	CutOffValue = 1.f - FinalCoverage;
	
	float CloudShape = saturate((ErodedNoise - CutOffValue) / max(0.001f, WeatherCoverage));
	CloudShape = smoothstep(0.15f, 1.0f, CloudShape);
	CloudShape *= WeatherCoverage;
	CloudShape *= HeightGradient;
	CloudShape = pow(CloudShape, 1.5f);
		
	return	CloudShape * BaseDensity * 150.f;
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

	float4	ViewPos = mul(float4(ScreenSpaceNDC, 1.f, 1.f), CloudJitterInvProj);
	float3	ViewDir = normalize(ViewPos.xyz / ViewPos.w);
	float3	RayDirection = normalize(mul(float4(ViewDir, 0.f), g_matInvView).xyz);

	float3	PlanetCenter = float3(g_vCamPos.x, -SPHERE_RADIUS, g_vCamPos.z);
	
	float2	CloudMin = Get_SphereIntersection(g_vCamPos, RayDirection, PlanetCenter, SPHERE_RADIUS + CloudMinHeight);
	float2	CloudMax = Get_SphereIntersection(g_vCamPos, RayDirection, PlanetCenter, SPHERE_RADIUS + CloudMaxHeight);
	
	float	CloudStartHeight = CloudMin.y;
	float	CloudEndHeight = CloudMax.y;
	
	if (CloudEndHeight < 0.f || CloudStartHeight < 0.f || CloudStartHeight > CloudLODDistance)
	{
		OUTPUT[PixelPos] = OriginalTexture[PixelPos];
		return;
	}

	uint DepthWidth, DepthHeight;
	DepthTexture.GetDimensions(DepthWidth, DepthHeight);
	float2	TexCoord = float2(PixelPos) / float2(Width, Height);
	
	uint2	DepthPixelPos = uint2(TexCoord * float2(DepthWidth, DepthHeight));
	float	Depth = DepthTexture[DepthPixelPos].r; 
	
	bool bIsSky = (Depth == 1.f || Depth == 0.f);
	
	float4	ClipSpace	= float4(ScreenSpaceNDC, Depth, 1.f);
	float4	ViewSpace = mul(ClipSpace, CloudJitterInvProj);
	float4	WorldSpace	= mul(ViewSpace / ViewSpace.w, g_matInvView);
	float2 EarthHit = Get_SphereIntersection(g_vCamPos, RayDirection, PlanetCenter, SPHERE_RADIUS);
	if (EarthHit.y > 0.f && bIsSky)
	{
		OUTPUT[PixelPos] = OriginalTexture[PixelPos];
		return;
	}
	
	float DistanceToGeometry = bIsSky ? 9999999.f : distance(WorldSpace.xyz, g_vCamPos);
	
	if (DistanceToGeometry < CloudStartHeight || CloudStartHeight > CloudLODDistance)
	{
		OUTPUT[PixelPos] = OriginalTexture[PixelPos];
		return;
	}

	float EndDistance = min(CloudEndHeight, min(DistanceToGeometry, CloudLODDistance));
	float MaxTravelDistance = max(0.f, EndDistance - CloudStartHeight);
	if (MaxTravelDistance <= 0.f)
	{
		OUTPUT[PixelPos] = OriginalTexture[PixelPos];
		return;
	}
	float DistanceRatio = saturate(CloudStartHeight / CloudLODDistance);
	uint	DynamicStepCount = (uint) lerp((float) MAX_STEP, (float) MIN_STEP, DistanceRatio);
	
	float	DynamicStepSize = MaxTravelDistance / float(DynamicStepCount);
	float3	CurrentPosition = g_vCamPos + (RayDirection * CloudStartHeight);
	
	float	TemporalSeed = CloudJitterOffset.x * 1000.f;
	float2	DynamicJitter = float2(PixelPos.x + TemporalSeed, PixelPos.y + TemporalSeed);
	float	JitterValue = Compute_GradientNoise(DynamicJitter);
	CurrentPosition += RayDirection * (DynamicStepSize * JitterValue);

	float	AccumulatedTransmittance = 1.f;
	float3	FinalColor = float3(0.f, 0.f, 0.f);
	
	float	HorizonFade = saturate(RayDirection.y * 2.f);

	for (uint Step = 0; Step < DynamicStepCount; ++Step)
	{
		float Density = Get_CloudDensity(CurrentPosition, CloudDensity);
		
		float DistanceFromCam = distance(CurrentPosition, g_vCamPos);
		float DistanceFade = 1.f - saturate(DistanceFromCam / CloudLODDistance);
		
		Density *= pow(DistanceFade, 3.f) * HorizonFade;
		
		if (Density > 0.f)
		{
			float	LightAccumulatedDensity = 0.f;
			float3	LightRayPos = CurrentPosition;
			
			for (uint lStep = 0; lStep < LIGHT_STEP_COUNT; ++lStep)
			{
				LightRayPos += -CloudLightDirection * LIGHT_STEP_SIZE;
				LightAccumulatedDensity += Get_CloudDensity(LightRayPos, CloudDensity);
			}
			float	LightTransmittance = exp(-LightAccumulatedDensity * LIGHT_STEP_SIZE * LightAbsorption * 10.f);
			float	LightOpticalDepth = LightAccumulatedDensity * LIGHT_STEP_SIZE * LightAbsorption * 10.f;
			float3	MultipleScattering = Compute_MultipleScattering(RayDirection, CloudLightDirection, LightOpticalDepth, CloudScattering);
			
			float	OpticalDepth = Density * DynamicStepSize;
			float	PowderEffect = 1.f - exp(-OpticalDepth * 2.f);
			
			float3	BaseColor = CloudColor * CloudBrightness;
			float3	ShadedColor = BaseColor + GLOBAL_SUN_COLOR * MultipleScattering;
			
			float	CurrentTransmittance = exp(-OpticalDepth * LightAbsorption);
			
			float3	LightContribution = ShadedColor * (1.f + PowderEffect);
			
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

[numthreads(16, 16, 1)]
void CSMain_CloudTAA(uint3 ID : SV_DispatchThreadID)
{
	if (ID.x >= (uint) ScreenResolution.x || ID.y >= (uint) ScreenResolution.y) return;

	uint2	PixelPos = ID.xy;
	float2	TexCoord = (float2(PixelPos) + 0.5f) * InvScreenResolution;
	float2	ScreenSpaceNDC = TexCoord * float2(2.f, -2.f) + float2(-1.f, 1.f);

	float	Depth = DepthTexture[PixelPos].r;
    
	float4	ClipSpace  = float4(ScreenSpaceNDC, Depth, 1.f);
	float4	ViewSpace  = mul(ClipSpace, CloudJitterInvProj);
	float4	WorldSpace = mul(ViewSpace / ViewSpace.w, g_matInvView);
    
	float4	PrevClipSpace = mul(WorldSpace, CloudPrevViewProj);
	PrevClipSpace /= PrevClipSpace.w;
	float2	PrevTexCoord = PrevClipSpace.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
	
	float3	CurrentColor = OriginalTexture[PixelPos].rgb;
	
	if (any(PrevTexCoord < 0.f) || any(PrevTexCoord > 1.f)) {
		OUTPUT[PixelPos] = float4(CurrentColor, 1.f);
		return;
	}
	
	float3	MinColor = CurrentColor;
	float3	MaxColor = CurrentColor;
    
	[unroll]
	for (int x = -1; x <= 1; ++x)
	{
    [unroll]
		for (int y = -1; y <= 1; ++y)
		{
			if (x == 0 && y == 0) continue;
        
			uint2 NeighborPos = clamp(PixelPos + int2(x, y), 0, (uint2) ScreenResolution - 1);
			float3 NeighborColor = OriginalTexture[NeighborPos].rgb;
        
			MinColor = min(MinColor, NeighborColor);
			MaxColor = max(MaxColor, NeighborColor);
		}
	}

	float3 HistoryColor = HistoryTexture.SampleLevel(LinearClamp, PrevTexCoord, 0).rgb;
	HistoryColor = clamp(HistoryColor, MinColor, MaxColor);
    
	float CurrentLuma = dot(CurrentColor, float3(0.299f, 0.587f, 0.114f));
	float HistoryLuma = dot(HistoryColor, float3(0.299f, 0.587f, 0.114f));
	float LumaDiff = abs(CurrentLuma - HistoryLuma);
	
	float BlendWeight = lerp(0.05f, 0.2f, saturate(LumaDiff * 5.f));
	
	//OUTPUT[PixelPos] = float4(lerp(HistoryColor, CurrentColor, BlendWeight), 1.f);
	
	float3 FinalColor = lerp(HistoryColor, CurrentColor, BlendWeight);
	
	float3 NeighborSum = 0.f;
	NeighborSum += OriginalTexture[clamp(PixelPos + int2(0, -1), 0, ScreenResolution - 1)].rgb;
	NeighborSum += OriginalTexture[clamp(PixelPos + int2(0, 1), 0, ScreenResolution - 1)].rgb;
	NeighborSum += OriginalTexture[clamp(PixelPos + int2(-1, 0), 0, ScreenResolution - 1)].rgb;
	NeighborSum += OriginalTexture[clamp(PixelPos + int2(1, 0), 0, ScreenResolution - 1)].rgb;

	float3 BlurColor = NeighborSum * 0.25f;
	
	float SharpenAmount = 0.15f;
	FinalColor = FinalColor + (FinalColor - BlurColor) * SharpenAmount;
	
	FinalColor = max(0.f, FinalColor);

	OUTPUT[PixelPos] = float4(FinalColor, 1.f);
	return;
}
