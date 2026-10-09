#include "../ShaderHeader/SH_CommonFunction.hlsli"

#define MAX_STEP			48
#define MIN_STEP			12
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

static const uint	BLUENOISE_TEXSIZE = { 256 };

static const float	CLOUD_GROUND_Y = -500.f;

static const int2	Offsets[4] = { int2(0, -1), int2(0, 1), int2(-1, 0), int2(1, 0) };

static const float Weights[3][3] =
{
	{ 1.f, 2.f, 1.f },
	{ 2.f, 4.f, 2.f },
	{ 1.f, 2.f, 1.f }
};
	
Texture2D<float4>	OriginalTexture		: register(t0);
Texture2D<float4>	HistoryTexture		: register(t1);
Texture3D<float>	BaseVolumeTexture	: register(t2);
Texture3D<float>	DetailVolumeTexture : register(t3);

Texture2D<float>	DepthTexture		: register(t4);
Texture2D<float4>	WeatherMapTexture	: register(t5);
Texture2D<float4>	BlueNoiseTexture	: register(t6);
Texture2D<float4>	CurlNoiseTexture	: register(t7);

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
float Jitter_BlueNoise(float2 _PixelPos, uint _Seed)
{
	uint2 TemporalOffset = uint2((uint) _Seed * 137, (uint) _Seed * 293);
	uint3 SamplePos = uint3((_PixelPos + TemporalOffset) % BLUENOISE_TEXSIZE, 0);
	
	return BlueNoiseTexture.Load(SamplePos).r;
}
float PackWorleyFBM(float3 worley)
{
	return worley.r * 0.625f + worley.g * 0.25f + worley.b * 0.125f;
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
	const float WindSpeed = 100.f;
	float3	WindOffset = WindDirection * WindTimeAccumulation * WindSpeed;
	float3	MovedPos = WorldPos - WindOffset;
	
	float2	CurlUV = MovedPos.xz * 0.00005f;
	float3	CurlNoiseValue = CurlNoiseTexture.SampleLevel(LinearWrap, CurlUV, 0).rgb;
	CurlNoiseValue = CurlNoiseValue * 2.f - 1.f;
	
	float	DistortionStrength = 300.f;
	float3	DistortedPos = MovedPos + (CurlNoiseValue * DistortionStrength);
	
	float2	WeatherOffset = float2(90000.f, -2000.f);
	float2	WeatherMapUV  = (MovedPos.xz + WeatherOffset) * 0.00001f;
	float3	WeatherMapTex = WeatherMapTexture.SampleLevel(LinearWrap, WeatherMapUV, 0).rgb;
	
	float	WeatherCoverage = smoothstep(0.05f, 0.55f, WeatherMapTex.r);
	float	WeatherType		= smoothstep(0.f, 0.6f, WeatherMapTex.g) * 0.1f;
	float	WeatherDensity	= WeatherMapTex.b;
	
	float	DistToCam = distance(MovedPos, g_vCamPos);
	float	DetailFade = 1.f - saturate(DistToCam / 50000.f);
	
	float3	PlanetCenter = float3(g_vCamPos.x, CLOUD_GROUND_Y - SPHERE_RADIUS, g_vCamPos.z);
	float	CurrentHeight = distance(WorldPos, PlanetCenter) - SPHERE_RADIUS;
	
	float	HeightRange = max(1.f, CloudMaxHeight - CloudMinHeight);
	float	HeightFraction = saturate((CurrentHeight - CloudMinHeight) / HeightRange);
	
	float	CumulusProfile = smoothstep(0.0f, 0.35f, HeightFraction) * (1.f - smoothstep(0.4f, 0.95f, HeightFraction));
	float	StratusProfile = smoothstep(0.0f, 0.1f, HeightFraction) * (1.f - smoothstep(0.4f, 0.9f, HeightFraction));
	float	HeightGradient = lerp(CumulusProfile, StratusProfile, WeatherType);
	
	float3	BaseScale = float3(BaseCloudNoiseScale, BaseCloudNoiseScale * 0.4f, BaseCloudNoiseScale);
	float4	BaseNoiseRGBA = BaseVolumeTexture.SampleLevel(LinearWrap, MovedPos * BaseScale, 0);
	
	float	PerlinWorley = BaseNoiseRGBA.r;
	float	BaseWorleyFBM = PackWorleyFBM(BaseNoiseRGBA.gba);
	
	float	BaseCloud = Remap(PerlinWorley, BaseWorleyFBM - 1.f, 1.f, 0.f, 1.f);
	BaseCloud *= HeightGradient;
	
	float	FinalCoverage = CloudCoverage * WeatherCoverage;
	float	CutOffValue = 1.f - FinalCoverage;
	
	float	BaseCloudShape = saturate(Remap(BaseCloud, CutOffValue, 1.f, 0.f, 1.f));
	if (BaseCloudShape <= 0.f)	return 0.f;
	
	float3	DetailScale = float3(DetailCloudNoiseScale, DetailCloudNoiseScale * 0.6f, DetailCloudNoiseScale);
	float4	DetailNoiseRGBA = DetailVolumeTexture.SampleLevel(LinearWrap, DistortedPos * DetailScale, 0);
	float	DetailWorleyFBM = PackWorleyFBM(DetailNoiseRGBA.rgb);
	
	float	ErosionStrength = lerp(0.1f, 0.7f, saturate(HeightFraction));
	float	CloudFinal = Remap(BaseCloudShape, DetailWorleyFBM * ErosionStrength, 1.f, 0.f, 1.f);
	
	CloudFinal = smoothstep(0.f, 1.f, CloudFinal);
	return CloudFinal * BaseDensity * 150.f;
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
	OUTPUT.GetDimensions(Width, Height);
	if (ID.x >= Width || ID.y >= Height)	return;
	
	uint2 PixelPos = ID.xy;
	float2	TexCoord = (float2(PixelPos) + 0.5f) / float2(Width, Height);
	float2 ScreenSpaceNDC = TexCoord * float2(2.f, -2.f) + float2(-1.f, 1.f);

	float4	ViewPos = mul(float4(ScreenSpaceNDC, 1.f, 1.f), CloudJitterInvProj);
	float3	ViewDir = normalize(ViewPos.xyz / ViewPos.w);
	float3	RayDirection = normalize(mul(float4(ViewDir, 0.f), g_matInvView).xyz);

	float3	PlanetCenter = float3(g_vCamPos.x, CLOUD_GROUND_Y - SPHERE_RADIUS, g_vCamPos.z);
	
	float	CloudStartHeight = Get_SphereIntersection(g_vCamPos, RayDirection, PlanetCenter, SPHERE_RADIUS + CloudMinHeight).y;
	float	CloudEndHeight	 = Get_SphereIntersection(g_vCamPos, RayDirection, PlanetCenter, SPHERE_RADIUS + CloudMaxHeight).y;
	
	if (CloudEndHeight < 0.f || CloudStartHeight < 0.f || CloudStartHeight > CloudLODDistance)
	{
		//OUTPUT[PixelPos] = OriginalTexture[PixelPos];
		OUTPUT[PixelPos] = float4(0.f, 0.f, 0.f, 1.f);
		return;
	}

	uint DepthWidth, DepthHeight;
	DepthTexture.GetDimensions(DepthWidth, DepthHeight);
	
	uint2 DepthPixelPos = min(uint2(TexCoord * float2(DepthWidth, DepthHeight)), uint2(DepthWidth - 1, DepthHeight - 1));
	float	Depth = DepthTexture[DepthPixelPos].r; 
	
	bool	bIsSky = (Depth == 1.f || Depth == 0.f);
	
	float4	ClipSpace	= float4(ScreenSpaceNDC, Depth, 1.f);
	float4	ViewSpace	= mul(ClipSpace, CloudJitterInvProj);
	float4	WorldSpace	= mul(ViewSpace / ViewSpace.w, g_matInvView);
	float2	EarthHit	= Get_SphereIntersection(g_vCamPos, RayDirection, PlanetCenter, SPHERE_RADIUS);
	
	if (EarthHit.y > 0.f && bIsSky)
	{
		//OUTPUT[PixelPos] = OriginalTexture[PixelPos];
		OUTPUT[PixelPos] = float4(0.f, 0.f, 0.f, 1.f);
		return;
	}
	
	float DistanceToGeometry = bIsSky ? 9999999.f : distance(WorldSpace.xyz, g_vCamPos);
	
	if (DistanceToGeometry < CloudStartHeight || CloudStartHeight > CloudLODDistance)
	{
		//OUTPUT[PixelPos] = OriginalTexture[PixelPos];
		OUTPUT[PixelPos] = float4(0.f, 0.f, 0.f, 1.f);
		return;
	}

	float EndDistance = min(CloudEndHeight, min(DistanceToGeometry, CloudLODDistance));
	float MaxTravelDistance = max(0.f, EndDistance - CloudStartHeight);
	
	if (MaxTravelDistance <= 0.f)
	{
		//OUTPUT[PixelPos] = OriginalTexture[PixelPos];
		OUTPUT[PixelPos] = float4(0.f, 0.f, 0.f, 1.f);
		return;
	}
	
	float	DistanceRatio = saturate(CloudStartHeight / CloudLODDistance);
	uint	DynamicStepCount = (uint) lerp((float) MAX_STEP, (float) MIN_STEP, DistanceRatio);
	
	float	DynamicStepSize = MaxTravelDistance / float(DynamicStepCount);
	float3	CurrentPosition = g_vCamPos + (RayDirection * CloudStartHeight);
	
	uint	TemporalSeed = (uint) CloudJitterOffset.x;
	float	JitterValue = Jitter_BlueNoise(PixelPos, TemporalSeed);
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
		
		if (Density > 0.f) {
			float	LightAccumulatedDensity = 0.f;
			float	LightOpticalDepth = 0.f;
			float3	LightRayPos = CurrentPosition;
			
			float	CurrentLightStep = 50.f;
			
			for (uint lStep = 0; lStep < LIGHT_STEP_COUNT; ++lStep)
			{
				LightRayPos += CloudLightDirection * CurrentLightStep;
				float StepDensity = Get_CloudDensity(LightRayPos, CloudDensity);
				LightOpticalDepth += StepDensity * CurrentLightStep;
				CurrentLightStep *= 2.f;
			}
			
			LightOpticalDepth *= (LightAbsorption * 10.f);
			float3	MultipleScattering = Compute_MultipleScattering(RayDirection, CloudLightDirection, LightOpticalDepth, CloudScattering);
			
			float	HeightAboveGround = distance(CurrentPosition, PlanetCenter) - SPHERE_RADIUS;
			float	HeightRatio = saturate((HeightAboveGround - CloudMinHeight) / max(1.f, CloudMaxHeight - CloudMinHeight));
			float	AmbientAO = lerp(0.3f, 1.f, HeightRatio);
			
			float3	BaseColor = CloudColor * CloudBrightness * AmbientAO;
			float3	ShadedColor = BaseColor + GLOBAL_SUN_COLOR * MultipleScattering;
			
			float	OpticalDepth = Density * DynamicStepSize;
			float	PowderEffect = 1.f - exp(-OpticalDepth * 2.f);
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
	
	//OUTPUT[PixelPos] = float4(FinalColor + OriginalTexture[PixelPos].rgb * AccumulatedTransmittance, 1.f);
	OUTPUT[PixelPos] = float4(FinalColor, AccumulatedTransmittance);
	return;
}

[numthreads(16, 16, 1)]
void CSMain_CloudTAA(uint3 ID : SV_DispatchThreadID)
{
	uint Width, Height;
	OUTPUT.GetDimensions(Width, Height);

	if (ID.x >= Width || ID.y >= Height)
		return;

	int2 Pixel = int2(ID.xy);
	float2 UV = (float2(Pixel) + 0.5f) / float2(Width, Height);

	float4 Current = OriginalTexture.Load(int3(Pixel, 0));

    // 이전 화면 좌표를 구하지 못하면 현재 값 사용
	OUTPUT[Pixel] = Current;

    // 현재 픽셀의 시선 방향 계산
	float2 NDC = UV * float2(2.f, -2.f) + float2(-1.f, 1.f);

	float4 ViewPosition =
        mul(float4(NDC, 1.f, 1.f), CloudJitterInvProj);

	float3 ViewDirection =
        normalize(ViewPosition.xyz / ViewPosition.w);

	float3 WorldDirection =
        mul(float4(ViewDirection, 0.f), g_matInvView).xyz;

    // 방향 벡터(w=0)로 이전 화면에 투영
    // 카메라 회전만 보정하는 간단한 방식
	float4 PreviousClip =
        mul(float4(WorldDirection, 0.f), CloudPrevViewProj);

	if (PreviousClip.w <= 0.0001f)
		return;

	float2 PreviousUV =
        PreviousClip.xy / PreviousClip.w
        * float2(0.5f, -0.5f) + 0.5f;

	if (any(PreviousUV < 0.f) || any(PreviousUV >= 1.f))
		return;

	float4 History =
        HistoryTexture.SampleLevel(LinearClamp, PreviousUV, 0);

    // 이전 결과가 현재 주변 값에서 크게 벗어나지 않도록 제한
	float4 MinValue = Current;
	float4 MaxValue = Current;

    [unroll]
	for (int y = -1; y <= 1; ++y)
	{
        [unroll]
		for (int x = -1; x <= 1; ++x)
		{
			int2 P = clamp(
                Pixel + int2(x, y),
                int2(0, 0),
                int2(Width, Height) - 1);

			float4 Neighbor = OriginalTexture.Load(int3(P, 0));

			MinValue = min(MinValue, Neighbor);
			MaxValue = max(MaxValue, Neighbor);
		}
	}

	History = clamp(History, MinValue, MaxValue);

    // 이전 85%, 현재 15%. RGB와 투과율 A를 함께 누적
	OUTPUT[Pixel] = lerp(History, Current, 0.15f);
}

[numthreads(16, 16, 1)]
void CSMain_CloudComposite(uint3 ID : SV_DispatchThreadID)
{
	uint Width, Height;
	OUTPUT.GetDimensions(Width, Height);

	if (ID.x >= Width || ID.y >= Height)	return;

	float2 UV = (float2(ID.xy) + 0.5f) / float2(Width, Height);

	float4 Scene = OriginalTexture[ID.xy];
	float4 Cloud = HistoryTexture.SampleLevel(LinearClamp, UV, 0);

	OUTPUT[ID.xy] = float4(Scene.rgb * Cloud.a + Cloud.rgb, Scene.a);
}
