#include "../ShaderHeader/SH_CommonFunction.hlsli"

#define POISSON_COUNT	4

RWTexture2D<float4> OUTPUT : register(u0);

// Base Texture
Texture2D<float4>	AlbedoMap		: register(t0);
Texture2D<float4>	NormalMap		: register(t1);
Texture2D<float4>	SMROMap			: register(t2);
Texture2D<float4>	EmissiveMap		: register(t3);
Texture2D<float4>	AmbientMap		: register(t4);
Texture2D<float>	DepthMap		: register(t5);

// Image Based Lighting
TextureCube			IrridianceMap	: register(t6);					// Enviroment Light
TextureCube			PreFilterMap	: register(t7); 
Texture2D<float4>	LookUpTableMap	: register(t8);					// BRDF LUT

// Shadow Texture
Texture2DArray<float>	StaticShadowMaps	  : register(t9);		// Directional Static
Texture2DArray<float>	DynamicShadowMaps	  : register(t10);		// Directional Dynamic

TextureCubeArray<float> StaticShadowCubeMaps  : register(t11);		// Point Static
TextureCubeArray<float> DynamicShadowCubeMaps : register(t12);		// Point Dynamic

static const float2		ShadowMapResolution		= { 1280.f, 720.f };

static const float		ShadowSmoothness		= 1.5f;
static const float		ShadowBrightness		= 1.f;

static const float		EnviromentIntensity		= 1.f;				// 환경광 밝기
static const float		FillLightBrightness		= 0.75f;			// 등지는 영역의 밝기
static const float		DirectLightBrightness	= 1.f;				// 빛받는 영역의 밝기

static const float2		PoissonDisk_EightTab[8] =
{
    float2(0.000000, 0.000000), float2(0.527837, -0.085868), float2(-0.040062, 0.536087), float2(-0.670445, -0.179949),
    float2(-0.419418, -0.616039), float2(0.440453, 0.639399), float2(-0.757088, 0.349334), float2(0.574619, -0.715851)
};
static const float2		PoissonDisk_FourTab[4] =
{
	// 퀄 ㅈㄴ 떨어져서 보류
	float2(-0.326, -0.406), float2(-0.840, 0.074), float2(-0.696, 0.457), float2(-0.203, 0.621)
};

float Get_GradientNoise(float2 _PixelPos)
{
    return frac(sin(dot(_PixelPos, float2(12.9898, 78.233))) * 43758.5453123);
}

float2x2 Get_RandomNoise(float2 _PixelPos)
{
	float RandomNoise = Get_GradientNoise(_PixelPos);
	float RandomAngle = RandomNoise * 2.f * PI;

	float SinAngle = sin(RandomAngle);
	float CosAngle = cos(RandomAngle);
	
	return float2x2(CosAngle, -SinAngle, SinAngle, CosAngle);
}

float DistributionGGX(float3 N, float3 H, float _Roughness)
{
    float R = _Roughness * _Roughness;
    float R2 = R * R;
    
    float NDH = max(0.f, dot(N, H));
    float NDH2 = NDH * NDH;
    
    float Num = R2;
    float Denom = ((NDH * NDH) * (R2 - 1.0) + 1.0);
    Denom = PI * Denom * Denom;
	
    return Num / max(0.000001f, Denom);
}
float VisibilitySmithJointGGX(float NDY, float NDL, float _Roughness)
{
    float R = _Roughness * _Roughness;
    float R2 = R * R;
    
    float lambdaV = NDL * sqrt(max((-NDY * R2 + NDY) * NDY + R2, 0.001f));
    float lambdaL = NDY * sqrt(max((-NDL * R2 + NDL) * NDL + R2, 0.001f));
    
    float Denom = lambdaV + lambdaL;
    return Denom > 0.0f ? 0.5f / Denom : 0.0f;
}
float3 FresnelSchlick(float CTH, float3 MBR)
{
    float ClampCTH = clamp(CTH, 0.0f, 1.0f);
    return MBR + (1.0 - MBR) * pow(clamp(1.0 - ClampCTH, 0.0, 1.0), 5.0);
}
float MergeShadowMap(int _ShadowSlot, float2 _SamplerUV, float _CurrentPixelDepth)
{
	float StaticShadow	= StaticShadowMaps.SampleCmpLevelZero(ShadowSampler, float3(_SamplerUV, _ShadowSlot), _CurrentPixelDepth);
	float DynamicShadow = DynamicShadowMaps.SampleCmpLevelZero(ShadowSampler, float3(_SamplerUV, _ShadowSlot), _CurrentPixelDepth);
	
	return min(StaticShadow, DynamicShadow);
}
float MergeShadowCubeMap(int _ShadowSlot, float3 _SamplerUV, float _CurrentPixelDepth)
{
	float StaticShadow	= StaticShadowCubeMaps.SampleCmpLevelZero(ShadowSampler, float4(_SamplerUV, _ShadowSlot), _CurrentPixelDepth);
	float DynamicShadow = DynamicShadowCubeMaps.SampleCmpLevelZero(ShadowSampler, float4(_SamplerUV, _ShadowSlot), _CurrentPixelDepth);
	
	return min(StaticShadow, DynamicShadow);
}
float Attenuate_ShadowStrength(float _ShadowFactor, float _Distance, uint _LightIndex)
{
	float Attenuation = saturate(1.f - (_Distance / AffectedLight[_LightIndex].LightRange));
	//Attenuation = Attenuation * Attenuation; // 거리 기반 그림자 감쇄
	float ShadowStrength = saturate(AffectedLight[_LightIndex].LightIntensity);
	
	// 그림자가 젤 어둡게 지는 값(1.f)과 그림자 인수와 lerp -> 빛과 가까우면 어둡고, 멀면 옅음.
	return lerp(1.f, _ShadowFactor, ShadowStrength * Attenuation);
}
float Compute_SmoothShadow( float4 _WorldPos, float2x2 _RandomRotMat, float2 _SamplingRange, int _ShadowSlot, uint _LightIndex)
{
	float4 LightPos = mul(float4(_WorldPos.xyz, 1.f), AffectedLight[_LightIndex].g_LightViewProj[0]);
			
    float2 ShadowMapUV;
    ShadowMapUV.x = (LightPos.x / LightPos.w) * +0.5f + 0.5f;
    ShadowMapUV.y = (LightPos.y / LightPos.w) * -0.5f + 0.5f;

	float3 LightNDC = LightPos.xyz / LightPos.w;
	
	[branch]
	if (LightNDC.x < -1.f || LightNDC.x > 1.f ||
	 	LightNDC.y < -1.f || LightNDC.y > 1.f || 
		LightNDC.z <  0.f || LightNDC.z > 1.f)
	{
		//return (AffectedLight[_LightIndex].LightType == LIGHT_DIRECTIONAL) ? ShadowBrightness : 1.f;
		return 1.f;
	}
    
	float CurrentPixelDepth = LightNDC.z;
    CurrentPixelDepth -= 0.0001f; // Depth Bias
    
	float ShadowFactor = 0.f;
	
    [unroll]
	for (int i = 0; i < POISSON_COUNT; ++i)
	{
		float2 RotatedOffset = mul(PoissonDisk_EightTab[i], _RandomRotMat);
        
		float2 SampleUV = ShadowMapUV + (RotatedOffset * _SamplingRange);
		
		ShadowFactor += MergeShadowMap(_ShadowSlot, SampleUV, CurrentPixelDepth);
        // SampleCmpLevelZero : Texture2D(ShadowMap)의 깊이와 CompareValue(CurrentPixelDepth) 를 비교했을 때
        // CompareValue가 크면 1, 아니면 0 반환.(x값에 결과값 저장)
		//FinalShadowFactor += FinalShadowMap[_LightIndex].SampleCmpLevelZero(ShadowSampler, SampleUV, CurrentPixelDepth).x;
	}
	float NormalShadowFactor = lerp(ShadowBrightness, 1.f, ShadowFactor / POISSON_COUNT);
	if (AffectedLight[_LightIndex].LightType == LIGHT_DIRECTIONAL)
	{
		return NormalShadowFactor;
	}
	
	NormalShadowFactor *= ShadowBrightness;
	float FinalShadowFactor = Attenuate_ShadowStrength(NormalShadowFactor, length(_WorldPos.xyz - AffectedLight[_LightIndex].Position), _LightIndex);
	// NormalShadowFactor : 감쇄X 그림자 (지속적으로 같은 밝기의 그림자)
	// FinalShadowFactor  : 감쇄O 그림자 (거리기반 밝기 감쇄 그림자)
	
	return FinalShadowFactor;
}

float Compute_PointShadow(float4 _WorldPos, float2x2 _RandomRotMat, int _ShadowSlot, uint _LightIndex)
{
	float3 LightToPixel = _WorldPos.xyz - AffectedLight[_LightIndex].Position;
    float  Distance = length(LightToPixel);
	
	float CurrentPixelDepth = Distance / AffectedLight[_LightIndex].LightRange;
	CurrentPixelDepth -= 0.001f; // Depth Bias
	
    float  InvDistance  = 1.0f / max(Distance, 0.0001f);
    float3 Direction    = LightToPixel * InvDistance;
    float3 BaseUP       = abs(Direction.z) < 0.999f ? float3(0.f, 0.f, 1.f) : float3(1.f, 0.f, 0.f);
	
	float3 TangentX		= normalize(cross(Direction, BaseUP));
	float3 TangentY		= normalize(cross(Direction, TangentX));
    
	float FilterRadius = (ShadowSmoothness * 0.05f);
	
    float ShadowFactor = { 0.f };

    [unroll]
	for (int i = 0; i < POISSON_COUNT; ++i)
    {
		float2 RotatedOffset = mul(PoissonDisk_EightTab[i], _RandomRotMat);
        
        float3 Offset3D = (TangentX * RotatedOffset.x + TangentY * RotatedOffset.y) * FilterRadius;
        
		float3 SampleUV = Direction + Offset3D;
			
		ShadowFactor += MergeShadowCubeMap(_ShadowSlot, SampleUV, CurrentPixelDepth);
	}
	float NormalShadowFactor = lerp(ShadowBrightness, 1.f, ShadowFactor * 0.125f);
	
	NormalShadowFactor *= ShadowBrightness;
	float FinalShadowFactor = Attenuate_ShadowStrength(NormalShadowFactor, Distance, _LightIndex);
	// NormalShadowFactor : 감쇄X 그림자 (지속적으로 같은 밝기의 그림자)
	// FinalShadowFactor : 감쇄O 그림자  (거리기반 밝기 감쇄 그림자)
	
	return FinalShadowFactor;
}

float3 Compute_EnviromentLight(float3 N, float3 V, float3 _Albedo, float _Roughness, float _Metallic, float3 MBR)
{
	N = normalize(N);
	V = normalize(V);
	
	float Roughness = saturate(_Roughness);
	float Metallic  = saturate(_Metallic);
	
	float ReverseRoughness = 1.f - Roughness;
	float3 Fresnel = max(float3(ReverseRoughness, ReverseRoughness, ReverseRoughness), MBR);
	
	
	float NDV = saturate(dot(N, V));
	float3 F = MBR + (Fresnel - MBR) * pow(1.f - NDV, 5.f);
	
	float3 KS = F;
	float3 KD = (1.f - KS) * (1.f - Metallic);
	
	float3 Irridiance = IrridianceMap.SampleLevel(LinearClamp, N, 0.f).rgb;
	
	float3 DiffuseAmbient = KD * Irridiance * _Albedo;
	float3 R = reflect(-V, N);
	
	float3 PreFilteredMap = PreFilterMap.SampleLevel(LinearClamp, R, Roughness * MAX_REFLECTION_LOD).rgb;
	
	float2 BRDF = LookUpTableMap.SampleLevel(LinearClamp, float2(NDV, Roughness), 0.f).rg;
	
	float3 SpecularAmbient = PreFilteredMap * (F * BRDF.x + BRDF.y);
	
	return DiffuseAmbient + SpecularAmbient;
}

[numthreads(16, 16, 1)]
void CSMain(uint3 ID : SV_DispatchThreadID)
{
	[branch]
	if (ID.x >= SCREENX || ID.y >= SCREENY) return; // 스레드가 해상도 넘어가면 출력X
	
	float2	TexCoord = (float2(ID.xy) + 0.5f) / float2(SCREENX, SCREENY);
    float	Depth = DepthMap.SampleLevel(LinearWrap, TexCoord, 0.f).r; // 해당 픽셀 깊이 계산

	[branch]
    if (Depth >= 1.f)
    {
        OUTPUT[ID.xy] = float4(0.f, 0.f, 1.f, 1.f);
        return;
    }
	
    float4 DepthWorld = Convert_WorldPosByDepth(Depth, TexCoord);

    float3 WorldNormal = normalize(NormalMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb * 2.f - 1.f);
	
    float3 AlbedoTex = AlbedoMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb;
    float3 Albedo = pow(AlbedoTex.rgb, 2.2f);

    float3 MultipleTex = SMROMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb;
    float Metallic = MultipleTex.r;
    float Roughness = MultipleTex.g;
    //float   Ambient     = MultipleTex.b;

    float3 V = normalize(g_vCamPos - DepthWorld.xyz);
    float NDV = max(dot(WorldNormal, V), 0.0001f);
    
    // Metallic Material Based Reflection
    float3 MBR = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, Metallic);
	
    float3	LightAccumulation = float3(0.f, 0.f, 0.f);
	
	float2x2 RandomNoiseMatrix = Get_RandomNoise(float2(ID.xy));
	
	float2	SamplingRange = 1.f / float2(POINTLIGHT_RESOLUTION, POINTLIGHT_RESOLUTION) * ShadowSmoothness;
	
	[unroll]
    for (uint i = 0; i < LightCount; ++i)
	{
		float3 L, Radiance;
		[branch]
		if (Compute_DynamicLight(DepthWorld.xyz, AffectedLight[i], L, Radiance))
		{
			float RawNDL = dot(WorldNormal, L);
			[branch]
			if (RawNDL > 0.f)
			{
				float NDL = clamp(RawNDL, 0.f, 1.f);

				float3	H = normalize(V + L);
				float	D = DistributionGGX(WorldNormal, H, Roughness);
				float3	F = FresnelSchlick(max(dot(H, V), 0.f), MBR);

				float V_Spec = VisibilitySmithJointGGX(NDV, NDL, Roughness);

				float3 Specular = D * F * V_Spec / 20.f;

				float3 kS = F;
				float3 kD = (1.0f - kS) * (1.0f - Metallic);
				float3 Diffuse = kD * Albedo / PI;
				 
				float ShadowFactor = 1.f;
				int ShadowSlot = AffectedLight[i].ShadowSlot;
				
				[branch]
				if (ShadowSlot >= 0 && ShadowSlot < MAX_LIGHT_COUNT)
				{
					[branch]
					if (AffectedLight[i].LightType == LIGHT_POINT)
					{
						ShadowFactor = Compute_PointShadow(DepthWorld, RandomNoiseMatrix, ShadowSlot, i);
					}
					else
					{
						ShadowFactor = Compute_SmoothShadow(DepthWorld, RandomNoiseMatrix, SamplingRange, ShadowSlot, i);
					}
				}
				LightAccumulation += (Diffuse + Specular) * Radiance * NDL * ShadowFactor;
			}
		}
	}

	float3	BaseEmissive = EmissiveMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb * EmissiveColor * EmissiveIntensity;
    
	float	AmbientOcclusion = AmbientMap.SampleLevel(LinearWrap, TexCoord, 0.f).r;
	float3	Ambient = Compute_EnviromentLight(WorldNormal, V, Albedo, Roughness, Metallic, MBR);
	
	float3	EnviromentLight = Ambient * AmbientOcclusion * EnviromentIntensity;		// Enviroment Light
	
	float3	FillLighting	= Albedo * (1.f - Metallic) * FillLightBrightness;		// Shadow Face
	float3	DirectLighting	= LightAccumulation * DirectLightBrightness;			// Light Face
	
	float3	FinalColor		= EnviromentLight + FillLighting + DirectLighting + BaseEmissive;
	
	OUTPUT[ID.xy] = float4(FinalColor, 1.f);
    return;
}

[numthreads(16, 16, 1)]
void CSMain_Blend(uint3 ID : SV_DispatchThreadID)
{
	[branch]
	if (ID.x >= SCREENX || ID.y >= (uint) SCREENY) return;

	float2 TexCoord = (float2(ID.xy) + 0.5f) / float2(SCREENX, SCREENY);
	float Depth = DepthMap.SampleLevel(LinearWrap, TexCoord, 0.f).r; // 해당 픽셀 깊이 계산
	
	[branch]
	if (Depth >= 1.f)
	{
		OUTPUT[ID.xy] = float4(0.f, 0.f, 1.f, 1.f);
		return;
	}
	
	float4 DepthWorld = Convert_WorldPosByDepth(Depth, TexCoord);

	float3 WorldNormal = normalize(NormalMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb * 2.f - 1.f);
	
	float3 AlbedoTex = AlbedoMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb;
	float3 Albedo = pow(AlbedoTex.rgb, 2.2f);

	float3 MultipleTex = SMROMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb;
	
	float Metallic = MultipleTex.r;
	float Roughness = MultipleTex.g;
    //float   Ambient     = MultipleTex.b;

	float3 V = normalize(g_vCamPos - DepthWorld.xyz);
	float NDV = max(dot(WorldNormal, V), 0.0001f);
    
    // Metallic Material Based Reflection
	float3 MBR = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, Metallic);
	
	float3 LightAccumulation = float3(0.f, 0.f, 0.f);
	
	float2x2 RandomNoiseMatrix = Get_RandomNoise(float2(ID.xy));
	
	float2 SamplingRange = 1.f / float2(POINTLIGHT_RESOLUTION, POINTLIGHT_RESOLUTION) * ShadowSmoothness;
	
	[unroll]
	for (uint i = 0; i < LightCount; ++i)
	{
		float3 L, Radiance;
		[branch]
		if (Compute_DynamicLight(DepthWorld.xyz, AffectedLight[i], L, Radiance))
		{
			float RawNDL = dot(WorldNormal, L);
			[branch]
			if (RawNDL > 0.f)
			{
				float NDL = clamp(RawNDL, 0.f, 1.f);

				float3 H = normalize(V + L);
				float D = DistributionGGX(WorldNormal, H, Roughness);
				float3 F = FresnelSchlick(max(dot(H, V), 0.f), MBR);

				float V_Spec = VisibilitySmithJointGGX(NDV, NDL, Roughness);

				float3 Specular = D * F * V_Spec / 10.f;

				float3 kS = F;
				float3 kD = (1.0f - kS) * (1.0f - Metallic);
				float3 Diffuse = kD * Albedo / PI;
				 
				float ShadowFactor = 1.f;
				int ShadowSlot = AffectedLight[i].ShadowSlot;
				
				[branch]
				if (ShadowSlot >= 0 && ShadowSlot < MAX_LIGHT_COUNT)
				{
					[branch]
					if (AffectedLight[i].LightType == LIGHT_POINT)
					{
						ShadowFactor = Compute_PointShadow(DepthWorld, RandomNoiseMatrix, ShadowSlot, i);
					}
					else
					{
						ShadowFactor = Compute_SmoothShadow(DepthWorld, RandomNoiseMatrix, SamplingRange, ShadowSlot, i);
					}
				}
				LightAccumulation += (Diffuse + Specular) * Radiance * NDL * ShadowFactor;
			}
		}
	}

	float3 BaseEmissive = EmissiveMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb * EmissiveColor * EmissiveIntensity;
    
	float AmbientOcclusion = AmbientMap.SampleLevel(LinearWrap, TexCoord, 0.f).r;
	float3 Ambient = Compute_EnviromentLight(WorldNormal, V, Albedo, Roughness, Metallic, MBR);
	
	float3 EnviromentLight = Ambient * AmbientOcclusion * EnviromentIntensity;	// Enviroment Light
	
	float3 FillLighting = Albedo * (1.f - Metallic) * FillLightBrightness;		// Shadow Face
	float3 DirectLighting = LightAccumulation * DirectLightBrightness;			// Light Face
	
	float3 FinalColor = EnviromentLight + FillLighting + DirectLighting + BaseEmissive;
	
	OUTPUT[ID.xy] = float4(FinalColor, 1.f);
	return;
}

[numthreads(16, 16, 1)]
void CSMain_NonShadow(uint3 ID : SV_DispatchThreadID)
{
	[branch]
	if (ID.x >= SCREENX || ID.y >= (uint) SCREENY)
		return;

	float2	TexCoord = (float2(ID.xy) + 0.5f) / float2(SCREENX, SCREENY);
	float	Depth = DepthMap.SampleLevel(LinearWrap, TexCoord, 0.f).r; // 해당 픽셀 깊이 계산
	
	[branch]
	if (Depth >= 1.f)
	{
		OUTPUT[ID.xy] = float4(0.f, 0.f, 1.f, 1.f);
		return;
	}

	float4 DepthWorld = Convert_WorldPosByDepth(Depth, TexCoord);

	float3 WorldNormal = normalize(NormalMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb * 2.f - 1.f);
	
	float3 AlbedoTex = AlbedoMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb;
	float3 Albedo = pow(AlbedoTex.rgb, 2.2f);

	float3 MultipleTex = SMROMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb;
	float Metallic = MultipleTex.r;
	float Roughness = clamp(MultipleTex.g, 0.15f, 1.f);
    //float   Ambient     = MultipleTex.b;

	float3 V = normalize(g_vCamPos - DepthWorld.xyz);
	float NDV = max(dot(WorldNormal, V), 0.0001f);
    
    // Metallic Material Based Reflection
	float3 MBR = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, Metallic);
	
	float3 LightAccumulation = float3(0.f, 0.f, 0.f);
	
	[unroll]
	for (uint i = 0; i < LightCount; ++i)
	{
		float3 L, Radiance;
		[branch]
		if (Compute_DynamicLight(DepthWorld.xyz, AffectedLight[i], L, Radiance))
		{
			float RawNDL = dot(WorldNormal, L);
			[branch]
			if (RawNDL > 0.f)
			{
				float NDL = clamp(RawNDL, 0.f, 1.f);

				float3 H = normalize(V + L);
				float D = DistributionGGX(WorldNormal, H, Roughness);
				float3 F = FresnelSchlick(max(dot(H, V), 0.f), MBR);

				float V_Spec = VisibilitySmithJointGGX(NDV, NDL, Roughness);

				float3 Specular = D * F * V_Spec / 10.f;

				float3 kS = F;
				float3 kD = (1.0f - kS) * (1.0f - Metallic);
				float3 Diffuse = kD * Albedo / PI;
				
				LightAccumulation += (Diffuse + Specular) * Radiance * NDL;
			}
		}
	}

	float3 BaseEmissive = EmissiveMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb * EmissiveColor * EmissiveIntensity;
    
	float AmbientOcclusion = AmbientMap.SampleLevel(LinearWrap, TexCoord, 0.f).r;
	float3 Ambient = Compute_EnviromentLight(WorldNormal, V, Albedo, Roughness, Metallic, MBR);
	
	float3 EnviromentLight = Ambient * AmbientOcclusion * EnviromentIntensity;		// Enviroment Light
	
	float3 FillLighting = Albedo * (1.f - Metallic) * FillLightBrightness;			// Shadow Face
	float3 DirectLighting = LightAccumulation * DirectLightBrightness;				// Light Face  
	
	float3 FinalColor = EnviromentLight + FillLighting + DirectLighting + BaseEmissive;
	
	OUTPUT[ID.xy] = float4(FinalColor, 1.f);
	return;
}
