#include "../ShaderHeader/SH_CommonFunction.hlsli"

#define PCF_ADDITIONAL_COUNT	3
#define PCF_EVALUATE_COUNT		5
#define MAX_EFFECTLIGHT_COUNT	15		

Texture2D g_DiffuseTexture  : register(t0);
Texture2D g_NormalTexture   : register(t1);
Texture2D g_SMROTexture     : register(t2);
Texture2D g_EmissiveTexture : register(t3);
Texture2D g_AmbientTexture	: register(t4);
Texture2D g_DepthTexture	: register(t5);

// Image Based Lighting
TextureCube g_IrradianceTexture  : register(t6); // Enviroment Light
TextureCube g_PreFilterTexture	 : register(t7);
Texture2D	g_LookUpTableTexture : register(t8); // BRDF LUT

// Shadow Texture
Texture2DArray<float> StaticShadowTextures : register(t9); // Directional Static
Texture2DArray<float> DynamicShadowTextures : register(t10); // Directional Dynamic

TextureCubeArray<float> StaticShadowCubeTextures	: register(t11); // Point Static
TextureCubeArray<float> DynamicShadowCubeTextures : register(t12); // Point Dynamic

Texture2DArray<float> CSMShadowTextures : register(t13); // Directional Light

static const float	ShadowSmoothness = 1.5f;
static const float	ShadowBrightness = 0.f;
static const float	PointShadowDepthBias = 0.002f;
static const float	SpotShadowDepthBias = 0.00001f;
Texture2D			DefaultNoiseTexture : register(t14);
static const float	DissolveEdgeWidth = 0.025f;

cbuffer CB_EFFECT_LIGHT : register(b11)
{
	EffectLight ELightList[MAX_EFFECTLIGHT_COUNT];

	uint ELightCount;
	float3 ELightPadding;
};
cbuffer CB_CSM : register(b12)
{
	matrix ShadowViewProj[4];
	float4 CascadeSplits;
	float2 ShadowMapSize;
	float2 ShadowBias;
};

static const float2 PoissonDisk_EightTab[8] =
{
	float2(0.000000f, 0.000000f),
	float2(0.440453f, 0.639399f),
	float2(-0.757088f, 0.349334f),
	float2(-0.419418f, -0.616039f),
	float2(0.574619f, -0.715851f),
	
	float2(0.527837f, -0.085868f),
	float2(-0.040062f, 0.536087f),
	float2(-0.670445f, -0.179949f)
};
static const float2 PoissonDisk_FourTab[4] =
{
	float2(-0.326, -0.406), float2(-0.840, 0.074), float2(-0.696, 0.457), float2(-0.203, 0.621)
};

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 iWorld0 : INSTANCE_WORLD0;
    float4 iWorld1 : INSTANCE_WORLD1;
    float4 iWorld2 : INSTANCE_WORLD2;
    float4 iWorld3 : INSTANCE_WORLD3;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float4 vTangent : TANGENT;
    float4 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
};

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out;
    
    float4x4 matWV, matWVP;
    float4x4 matWorld = float4x4(In.iWorld0, In.iWorld1, In.iWorld2, In.iWorld3);
    
    matWV = mul(matWorld, g_matView);
    matWVP = mul(matWV, g_matProj);
    
    Out.vPosition = mul(float4(In.vPosition, 1.f), matWVP);
    Out.vNormal = normalize(mul(float4(In.vNormal, 0.f), matWorld));
    Out.vTangent = normalize(mul(float4(In.vTangent, 0.f), matWorld));
    Out.vBinormal = normalize(mul(float4(In.vBinormal, 0.f), matWorld));
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), matWorld);
    Out.vProjPos = Out.vPosition;
    return Out;
}

struct PS_IN
{
    float4 vPosition    : SV_POSITION;
    float4 vNormal      : NORMAL;
    float4 vTangent     : TANGENT;
    float4 vBinormal    : BINORMAL;
    float2 vTexcoord    : TEXCOORD0;
    float4 vWorldPos    : TEXCOORD1;
    float4 vProjPos     : TEXCOORD2;
};

struct PS_OUT_NONBLEND
{
    vector vDiffuse     : SV_TARGET0;
    vector vNormal      : SV_TARGET1;
    vector vSMRO        : SV_TARGET2;
    vector vEmissive    : SV_TARGET3;
};

struct PS_OUT_BLEND
{
	vector vDiffuse		: SV_TARGET0;
};

PS_OUT_NONBLEND PSMain_NonBlend(PS_IN IN)
{
	PS_OUT_NONBLEND Out;
    
    float4 fDiffuse     = g_DiffuseTexture.Sample(LinearWrap, IN.vTexcoord) * float4(AlbedoColor, 1.f);
    
    //if (fDiffuse.a == 0.0f) discard;
	clip(fDiffuse.a - 0.25f);
	
	float3 fNormal = Compute_WorldNormal(g_NormalTexture, IN.vTexcoord, IN.vNormal, IN.vTangent) * NormalIntensity;

	float4 fMRO         = g_SMROTexture.Sample(LinearWrap, IN.vTexcoord);
    
    float fFinalMetallic    = fMRO.r * MetallicIntensity;
    float fFinalRoughness   = fMRO.g * RoughnessIntensity;
    float fFinalAO          = fMRO.b * AmbientIntensity;
	float fFinalAlpha		= fMRO.a * ObjectAlpha;

    float3 fEmissive = g_EmissiveTexture.Sample(LinearWrap, IN.vTexcoord).r * EmissiveColor * EmissiveIntensity;
    
    float3 fFinalEmissive = Apply_DissolveEffect(DefaultNoiseTexture, fEmissive, IN.vTexcoord, DissolveEdgeWidth);
	
	Out.vDiffuse	= float4(fDiffuse.rgb, 1.f);
	//Out.vDiffuse	= float4(1.f, 0.f, 0.f, 1.f);
    Out.vNormal     = float4(fNormal * 0.5f + 0.5f, 1.f);
    Out.vSMRO       = float4(fFinalMetallic, fFinalRoughness, fFinalAO, 1.f);
	Out.vEmissive	= float4(fFinalEmissive, 1.f);
    
    return Out;
}


float MergeShadowMap(int _ShadowSlot, float2 _SamplerUV, float _CurrentPixelDepth)
{
	return DynamicShadowTextures.SampleCmpLevelZero(ShadowSampler, float3(_SamplerUV, _ShadowSlot), _CurrentPixelDepth);
}
float MergeShadowCubeMap(int _ShadowSlot, float3 _SamplerUV, float _CurrentPixelDepth)
{
	return DynamicShadowCubeTextures.SampleCmpLevelZero(ShadowSampler, float4(_SamplerUV, _ShadowSlot), _CurrentPixelDepth);
}
float Compute_SmoothShadow(float4 _WorldPos, float2x2 _RandomRotMat, float2 _SamplingRange, int _ShadowSlot, uint _LightIndex)
{
	float4 LightPos = mul(float4(_WorldPos.xyz, 1.f), AffectedLight[_LightIndex].g_LightViewProj[0]);
	float3 LightNDC = LightPos.xyz * rcp(LightPos.w);
	
	[branch]
	if (LightNDC.x < -1.f || LightNDC.x > 1.f ||
	 	LightNDC.y < -1.f || LightNDC.y > 1.f ||
		LightNDC.z < 0.f || LightNDC.z > 1.f)
		return 1.f;

	float2 ShadowMapUV;
	ShadowMapUV.x = LightNDC.x * +0.5f + 0.5f;
	ShadowMapUV.y = LightNDC.y * -0.5f + 0.5f;
	
	float CurrentPixelDepth = saturate(LightNDC.z - SpotShadowDepthBias);
	float ShadowFactor = 0.f;
	
    [unroll]
	for (int i = 0; i < PCF_EVALUATE_COUNT; ++i)
	{
		float2 RotatedOffset = mul(PoissonDisk_EightTab[i], _RandomRotMat);
        
		float2 SampleUV = ShadowMapUV + (RotatedOffset * _SamplingRange);
		
		ShadowFactor += MergeShadowMap(_ShadowSlot, SampleUV, CurrentPixelDepth);
	}
	
	[branch]		// Every Direction ShadowFactor 0.f
	if (ShadowFactor <= 0.0001f)
		return ShadowBrightness;
	
	[branch]		// Every Direction ShadowFactor 1.f
	if (ShadowFactor >= (float) PCF_EVALUATE_COUNT - 0.0001f)
		return 1.f;
	
	[unroll]
	for (int j = PCF_EVALUATE_COUNT; j < PCF_EVALUATE_COUNT + PCF_ADDITIONAL_COUNT; ++j)
	{
		float2 RotatedOffset = mul(PoissonDisk_EightTab[j], _RandomRotMat);
        
		float2 SampleUV = ShadowMapUV + (RotatedOffset * _SamplingRange);
		
		ShadowFactor += MergeShadowMap(_ShadowSlot, SampleUV, CurrentPixelDepth);
	}

	return lerp(ShadowBrightness, 1.f, saturate(ShadowFactor * (1.f / (PCF_EVALUATE_COUNT + PCF_ADDITIONAL_COUNT))));
}

float Compute_PointShadow(float4 _WorldPos, float2x2 _RandomRotMat, int _ShadowSlot, uint _LightIndex)
{
	float3 LightToPixel = _WorldPos.xyz - AffectedLight[_LightIndex].Position;
	
	float DistanceSQ = max(dot(LightToPixel, LightToPixel), 0.000001f);
	
	float InvDistance = rsqrt(DistanceSQ);
	float Distance = DistanceSQ * InvDistance;
	float3 Direction = LightToPixel * InvDistance;
	
	float CurrentPixelDepth = Distance / max(0.02f, AffectedLight[_LightIndex].OuterAttanuation);
	CurrentPixelDepth -= PointShadowDepthBias;
	
	float3 BaseUP = abs(Direction.z) < 0.999f ? float3(0.f, 0.f, 1.f) : float3(1.f, 0.f, 0.f);

	float3 TangentX = normalize(cross(Direction, BaseUP));
	float3 TangentY = normalize(cross(Direction, TangentX));
    
	float FilterRadius = (ShadowSmoothness / POINTLIGHT_RESOLUTION);
	
	float ShadowFactor = 0.f;

    [unroll]
	for (int i = 0; i < PCF_EVALUATE_COUNT; ++i)
	{
		float2 RotatedOffset = mul(PoissonDisk_EightTab[i], _RandomRotMat);
        
		float3 Offset3D = (TangentX * RotatedOffset.x + TangentY * RotatedOffset.y) * FilterRadius;
        
		float3 SampleDirection = Direction + Offset3D;
		
		ShadowFactor += MergeShadowCubeMap(_ShadowSlot, SampleDirection, CurrentPixelDepth);
	}
	
	[branch]		// Every Direction ShadowFactor 0.f
	if (ShadowFactor <= 0.0001f)
		return ShadowBrightness;
	
	[branch]		// Every Direction ShadowFactor 1.f
	if (ShadowFactor >= (float) PCF_EVALUATE_COUNT - 0.0001f)
		return 1.f;
	
	[unroll]
	for (int j = PCF_EVALUATE_COUNT; j < PCF_EVALUATE_COUNT + PCF_ADDITIONAL_COUNT; ++j)
	{
		float2 RotatedOffset = mul(PoissonDisk_EightTab[j], _RandomRotMat);
        
		float3 Offset3D = (TangentX * RotatedOffset.x + TangentY * RotatedOffset.y) * FilterRadius;
        
		float3 SampleDirection = Direction + Offset3D;
		
		ShadowFactor += MergeShadowCubeMap(_ShadowSlot, SampleDirection, CurrentPixelDepth);
	}
	
	return lerp(ShadowBrightness, 1.f, saturate(ShadowFactor * (1.f / (PCF_EVALUATE_COUNT + PCF_ADDITIONAL_COUNT))));
}

float Compute_CascadeShadow(float4 _WorldPos, float2x2 _RandomRotMat)
{
	float4 ViewPos = mul(float4(_WorldPos.xyz, 1.f), g_matView);
	float ViewDepth = abs(ViewPos.z);
	
	if (CascadeSplits.w <= 0.f || ViewDepth >= CascadeSplits.w)
		return 1.f;
	
	int CascadeIndex;
	if (ViewDepth < CascadeSplits.x)
		CascadeIndex = 0;
	else if (ViewDepth < CascadeSplits.y)
		CascadeIndex = 1;
	else if (ViewDepth < CascadeSplits.z)
		CascadeIndex = 2;
	else
		CascadeIndex = 3;
	
	float4 LightPos = mul(float4(_WorldPos.xyz, 1.f), ShadowViewProj[CascadeIndex]);
	
	if (abs(LightPos.w) <= 0.00001f)
		return 1.f;
	
	float3 LightNDC = LightPos.xyz / LightPos.w;
	
	if (LightNDC.x < -1.f || LightNDC.x > 1.f ||
		LightNDC.y < -1.f || LightNDC.y > 1.f ||
		LightNDC.z < 0.f || LightNDC.z > 1.f)
		return 1.f;

	float2 ShadowMapUV;
	ShadowMapUV.x = LightNDC.x * +0.5f + 0.5f;
	ShadowMapUV.y = LightNDC.y * -0.5f + 0.5f;
	
	float CurrentDepth = LightNDC.z - ShadowBias.x;
	
	float2 SamplingRange = ShadowSmoothness / max(ShadowMapSize, float2(1.f, 1.f));
	float ShadowFactor = 0.f;
	
	[branch]
	if (CascadeIndex <= 1)
	{
		[unroll]
		for (int i = 0; i < PCF_EVALUATE_COUNT; ++i)
		{
			float2 RotatedOffset = mul(PoissonDisk_EightTab[i], _RandomRotMat);
		
			float2 SampleUV = ShadowMapUV + RotatedOffset * SamplingRange;
			ShadowFactor += CSMShadowTextures.SampleLevel(LinearClamp, float3(SampleUV, (float) CascadeIndex), 0.f).r;
		}
		
		[branch]		// Every Direction ShadowFactor 0.f
		if (ShadowFactor <= 0.0001f)
			return ShadowBrightness;
	
		[branch]		// Every Direction ShadowFactor 1.f
		if (ShadowFactor >= (float) PCF_EVALUATE_COUNT - 0.0001f)
			return 1.f;
		
		[unroll]
		for (int j = PCF_EVALUATE_COUNT; j < PCF_EVALUATE_COUNT + PCF_ADDITIONAL_COUNT; ++j)
		{
			float2 RotatedOffset = mul(PoissonDisk_EightTab[j], _RandomRotMat);
		
			float2 SampleUV = ShadowMapUV + RotatedOffset * SamplingRange;
			ShadowFactor += CSMShadowTextures.SampleLevel(LinearClamp, float3(SampleUV, (float) CascadeIndex), 0.0f).r;
		}
		float TotalSampleCount = (float) (PCF_EVALUATE_COUNT + PCF_ADDITIONAL_COUNT);
		ShadowFactor *= (1.f / TotalSampleCount);
	}
	else
	{
		[unroll]
		for (int i = 0; i < PCF_EVALUATE_COUNT - 1; ++i)
		{
			float2 RotatedOffset = mul(PoissonDisk_FourTab[i], _RandomRotMat);
		
			float2 SampleUV = ShadowMapUV + RotatedOffset * SamplingRange;
			ShadowFactor += CSMShadowTextures.SampleCmpLevelZero(ShadowSampler, float3(SampleUV, (float) CascadeIndex), 0.f).r;
		}
		ShadowFactor *= 0.25f;
	}
	
	return lerp(ShadowBrightness, 1.f, ShadowFactor);
}

float3 Compute_EnviromentLight(float3 N, float3 V, float3 _Albedo, float _Roughness, float _Metallic, float3 MBR)
{
	N = normalize(N);
	V = normalize(V);
	
	float Roughness = saturate(_Roughness);
	float Metallic = saturate(_Metallic);
	
	float ReverseRoughness = 1.f - Roughness;
	float3 Fresnel = max(float3(ReverseRoughness, ReverseRoughness, ReverseRoughness), MBR);
	
	float NDV = saturate(dot(N, V));
	float3 F = MBR + (Fresnel - MBR) * pow(1.f - NDV, 5.f);
	
	float3 KS = F;
	float3 KD = (1.f - KS) * (1.f - Metallic);
	
	float3 Irradiance = g_IrradianceTexture.Sample(LinearClamp, N).rgb;
	
	float3 DiffuseAmbient = KD * Irradiance * _Albedo;
	float3 R = reflect(-V, N);
	
	float3 PreFilteredMap = g_PreFilterTexture.SampleLevel(LinearClamp, R, Roughness * MAX_REFLECTION_LOD).rgb;
	
	float2 BRDF = g_LookUpTableTexture.Sample(LinearClamp, float2(NDV, Roughness)).rg;
	
	float3 SpecularAmbient = PreFilteredMap * (F * BRDF.x + BRDF.y);
	
	return DiffuseAmbient + SpecularAmbient;
}

PS_OUT_BLEND PSMain_Blend(PS_IN IN)
{
	PS_OUT_BLEND Out;

	float4 DepthWorld = IN.vWorldPos;

	float3 TangentSpaceNormal = g_NormalTexture.Sample(LinearWrap, IN.vTexcoord).rgb * 2.f - 1.f;
	
	float3 N = normalize(IN.vNormal);
	float3 T = normalize(IN.vTangent);
	float3 B = normalize(cross(N, T));
	float3x3 TBN = float3x3(T, B, N);
	
	float3	WorldNormal = normalize(mul(TangentSpaceNormal, TBN));
	
	float4	AlbedoTex = g_DiffuseTexture.Sample(LinearWrap, IN.vTexcoord);
	float3	Albedo = pow(AlbedoTex.rgb, 2.2f);
	
	float3	MultipleTex = g_SMROTexture.Sample(LinearWrap, IN.vTexcoord).rgb;
	float	Metallic = MultipleTex.r;
	float	Roughness = clamp(MultipleTex.g, 0.15f, 1.f);
    
	float3 V = normalize(g_vCamPos - DepthWorld.xyz);
	float NDV = max(dot(WorldNormal, V), 0.0001f);
	
	float3 MBR = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, Metallic);
	float3 LightAccumulation = float3(0.f, 0.f, 0.f);
    
	float2x2 RandomNoiseMatrix = Get_RandomNoise(IN.vPosition.xy);
	float2 SamplingRange = 1.f / float2(POINTLIGHT_RESOLUTION, POINTLIGHT_RESOLUTION) * ShadowSmoothness;
	
	[loop]
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
				float3 kD = (1.f - kS) * (1.f - Metallic);
				float3 Diffuse = kD * Albedo / PI;
                
				float ShadowFactor = 1.f;
				int ShadowSlot = AffectedLight[i].ShadowSlot;
                
                [branch]
				if (ShadowSlot >= 0 && ShadowSlot < MAX_SHADOW_LIGHT_COUNT)
				{
                    //[branch]
					//if (AffectedLight[i].LightType == LIGHT_DIRECTIONAL)
					//{
					//	ShadowFactor = Compute_CascadeShadow(DepthWorld, RandomNoiseMatrix);
					//}
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
	
	float3	BaseEmissive = g_EmissiveTexture.Sample(LinearWrap, IN.vTexcoord).rgb;
    
	float2	ScreenUV = IN.vPosition.xy / float2(SCREENX, SCREENY);
	float	AmbientOcclusion = g_AmbientTexture.Sample(LinearWrap, ScreenUV).r;
    
	float3	Ambient = Compute_EnviromentLight(WorldNormal, V, Albedo, Roughness, Metallic, MBR);
	float3	EnviromentLight = Ambient * AmbientOcclusion * EnviromentIntensity;
    
	float3	FillLighting = Albedo * (1.f - Metallic) * FillLightBrightness;
	float3	DirectLighting = LightAccumulation * DirectLightBrightness;
    
	float3	FinalColor = EnviromentLight + FillLighting + DirectLighting + BaseEmissive;
    
	Out.vDiffuse = float4(FinalColor, AlbedoTex.a);
	return Out;

}
