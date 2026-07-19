#include "../ShaderHeader/SH_CommonFunction.hlsli"

// Base Texture
Texture2D<float4> AlbedoMap : register(t0);
Texture2D<float4> NormalMap : register(t1);
Texture2D<float4> SMROMap : register(t2);
Texture2D<float4> EmissiveMap : register(t3);
Texture2D<float4> AmbientMap : register(t4);

Texture2D<float> DepthMap : register(t5);

Texture2DArray<float> StaticShadowMaps : register(t6);
Texture2DArray<float> DynamicShadowMaps : register(t7);

TextureCubeArray<float> StaticShadowCubeMaps : register(t8);
TextureCubeArray<float> DynamicShadowCubeMaps : register(t9);

// Image Based Lighting
TextureCube IrridianceMap : register(t10);
TextureCube PreFilterMap : register(t11);
Texture2D<float4> LUTMap : register(t12);

RWTexture2D<float4> OUTPUT : register(u0);

static const float2 ScreenResolution = { 1280.f, 720.f };
static const float2 ShadowMapResolution = { 1280.f, 720.f };

static const float ShadowSmoothness = 1.5f;
static const float ShadowBrightness = 0.45f;

static const float DissolveEdgeWidth = 0.025f;

static const float2 PoissonDisk[8] =
{
    float2(0.000000, 0.000000), float2(0.527837, -0.085868), float2(-0.040062, 0.536087), float2(-0.670445, -0.179949),
    float2(-0.419418, -0.616039), float2(0.440453, 0.639399), float2(-0.757088, 0.349334), float2(0.574619, -0.715851)
};
float Get_GradientNoise(float2 _PixelPos)
{
    return frac(sin(dot(_PixelPos, float2(12.9898, 78.233))) * 43758.5453123);
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
float MergeShadowMap(int _LightIndex, float2 _SamplerUV, float _CurrentPixelDepth)
{
	float StaticShadow = StaticShadowMaps.SampleCmpLevelZero(ShadowSampler, float3(_SamplerUV, _LightIndex), _CurrentPixelDepth);
	float DynamicShadow = DynamicShadowMaps.SampleCmpLevelZero(ShadowSampler, float3(_SamplerUV, _LightIndex), _CurrentPixelDepth);
	return min(StaticShadow, DynamicShadow);
}
float MergeShadowCubeMap(int _LightIndex, float3 _SamplerUV, float _CurrentPixelDepth)
{
	float StaticShadow	= StaticShadowCubeMaps.SampleCmpLevelZero(ShadowSampler, float4(_SamplerUV, _LightIndex), _CurrentPixelDepth);
	float DynamicShadow = DynamicShadowCubeMaps.SampleCmpLevelZero(ShadowSampler, float4(_SamplerUV, _LightIndex), _CurrentPixelDepth);
	return min(StaticShadow, DynamicShadow);
}
float Compute_SmoothShadow(DynamicLight _Light, float4 _WorldPos, float2 _TexCoord, float2 _PixelPos, int _LightIndex)
{
	float4 LightPos = mul(float4(_WorldPos.xyz, 1.f), _Light.g_LightViewProj[_LightIndex]);
    
    float2 ShadowMapUV;
    ShadowMapUV.x = (LightPos.x / LightPos.w) * +0.5f + 0.5f;
    ShadowMapUV.y = (LightPos.y / LightPos.w) * -0.5f + 0.5f;
    
	float3 ShadowTexCoord = LightPos.xyz / LightPos.w;
	float2 ShadowUV = ShadowTexCoord.xy * 0.5f + 0.5f;

	if (ShadowMapUV.x < 0.f || ShadowMapUV.x > 1.f ||
	 	ShadowMapUV.y < 0.f || ShadowMapUV.y > 1.f)
	{
		return ShadowBrightness;
	}
    
	float CurrentPixelDepth = LightPos.z / LightPos.w;;
    CurrentPixelDepth -= 0.0005f; // Depth Bias
    
    float RandomNoise = Get_GradientNoise(_PixelPos);
    float RandomAngle = RandomNoise * 2.f * PI;
    
    float CosAngle = cos(RandomAngle);
    float SinAngle = sin(RandomAngle);
    float2x2 RotationMat = float2x2(CosAngle, -SinAngle, SinAngle, CosAngle);
   
    // 주변 ShadowSmoothness 반경까지 Sampling
    float2 SamplingRange = 1.f / ShadowMapResolution * ShadowSmoothness;
    
    float FinalShadowFactor = 0.0f;
	
    [unroll]
    for (int i = 0; i < 8; ++i)
	{
        float2 RotatedOffset = mul(PoissonDisk[i], RotationMat);
        
        float2 SampleUV = ShadowMapUV + (RotatedOffset * SamplingRange);
		
		FinalShadowFactor += MergeShadowMap(_LightIndex, SampleUV, CurrentPixelDepth);
        // SampleCmpLevelZero : Texture2D(ShadowMap)의 깊이와 CompareValue(CurrentPixelDepth) 를 비교했을 때 
        // CompareValue가 크면 1, 아니면 0 반환.(x값에 결과값 저장)
		//FinalShadowFactor += FinalShadowMap[_LightIndex].SampleCmpLevelZero(ShadowSampler, SampleUV, CurrentPixelDepth).x;
	}
    
    FinalShadowFactor /= 8.f;
			
	return lerp(ShadowBrightness, 1.f, FinalShadowFactor);
}

float Compute_PointShadow(DynamicLight _Light, float4 _WorldPos, float2 _PixelPos, int _LightIndex)
{
    float3 LightToPixel = _WorldPos.xyz - _Light.Position;
    float  Distance = length(LightToPixel);
    
    float  CurrentPixelDepth = Distance / _Light.LightRange;
    CurrentPixelDepth -= 0.0005f; // Depth Bias
    
    float  InvDistance  = 1.0f / max(Distance, 0.0001f);
    float3 Direction    = LightToPixel * InvDistance;
    float3 BaseUP       = abs(Direction.z) < 0.999f ? float3(0.f, 0.f, 1.f) : float3(1.f, 0.f, 0.f);
    
    float3 TangentX     = cross(Direction, BaseUP);
    float3 TangentY     = cross(Direction, TangentX);
    
    float  RandomNoise  = Get_GradientNoise(_PixelPos);
    float  RandomAngle  = RandomNoise * 2.f * PI;
    
    float  CosAngle     = cos(RandomAngle);
    float  SinAngle     = sin(RandomAngle);
    float2x2 RotationMat = float2x2(CosAngle, -SinAngle, SinAngle, CosAngle);
    
    float FilterRadius  = (ShadowSmoothness * 0.05f) / _Light.LightRange;
    
    float FinalShadowFactor = { 0.f };
    
    [unroll]
    for (int i = 0; i < 8; ++i)
    {
        float2 RotatedOffset = mul(PoissonDisk[i], RotationMat);
        
        float3 Offset3D = (TangentX * RotatedOffset.x + TangentY * RotatedOffset.y) * FilterRadius;
        
        float3 SampleUV = Direction + Offset3D;
		
		FinalShadowFactor += MergeShadowCubeMap(_LightIndex, SampleUV, CurrentPixelDepth);
	}
    
    FinalShadowFactor /= 8.f;
    
    return lerp(ShadowBrightness, 1.0f, FinalShadowFactor);
}

float3 Compute_EnviromentLight(float3 N, float3 V, float3 albedo, float _Roughness, float _Metallic, float3 MBR)
{
    float NDV = max(dot(N, V), 0.0);
    
    float3 F = MBR + (max(float3(1.0 - _Roughness, 1.0 - _Roughness, 1.0 - _Roughness), MBR) - MBR) * pow(clamp(1.0 - NDV, 0.0, 1.0), 5.0);
    
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= (1.0 - _Metallic);
    
    float3 irradiance = IrridianceMap.SampleLevel(LinearWrap, N, 0.f).rgb;
    float3 DiffuseAmbient = kD * irradiance * albedo;
    
    float3 R = reflect(-V, N);
    
    float3 PreFilteredDiffuse = PreFilterMap.SampleLevel(LinearWrap, R, _Roughness * MAX_REFLECTION_LOD).rgb;
    
    float2 lutUV = float2(NDV, _Roughness);
    float2 brdf = LUTMap.SampleLevel(LinearWrap, lutUV, 0.f).rg;
    
    float3 SpecularAmbient = PreFilteredDiffuse * (F * brdf.x + brdf.y);
    
    return (DiffuseAmbient + SpecularAmbient);
}

[numthreads(16, 16, 1)]
void CSMain(uint3 ID : SV_DispatchThreadID)
{
	if (ID.x >= (uint) ScreenResolution.x || ID.y >= (uint) ScreenResolution.y) return; // 스레드가 해상도 넘어가면 출력X

    float2	TexCoord = (float2(ID.xy) + 0.5f) / ScreenResolution;
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
    for (uint i = 0; i < LightCount; ++i)
	{
		float ShadowFactor = 1.f;
		float3 L, Radiance;
		if (Compute_DynamicLight(DepthWorld.xyz, AffectedLight[i], L, Radiance))
		{
			float RawNDL = dot(WorldNormal, L);

			if (RawNDL > 0.f)
			{
				float NDL = clamp(RawNDL, 0.f, 1.f);

				float3 H = normalize(V + L);
				float D = DistributionGGX(WorldNormal, H, Roughness);
				float3 F = FresnelSchlick(max(dot(H, V), 0.f), MBR);

				float V_Spec = VisibilitySmithJointGGX(NDV, NDL, Roughness);

				float3 Specular = D * F * V_Spec;

				float3 kS = F;
				float3 kD = (1.0f - kS) * (1.0f - Metallic);
				float3 Diffuse = kD * Albedo / PI;
				

                [branch]
				if (AffectedLight[i].LightType == LIGHT_POINT)
				{
					ShadowFactor = Compute_PointShadow(AffectedLight[i], DepthWorld, float2(ID.xy), i);
				}
				else
				{
					ShadowFactor = Compute_SmoothShadow(AffectedLight[i], DepthWorld, TexCoord, float2(ID.xy), i);
					if (ShadowFactor == 999.f)
					{
						OUTPUT[ID.xy] = float4(1.f, 1.f, 1.f, 1.f);
						return;
					}
				}
				
				LightAccumulation += (Diffuse + Specular) * Radiance * NDL * ShadowFactor;
			}
		}
	}
	
	float3 BaseEmissive = EmissiveMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb * EmissiveColor * EmissiveIntensity;
    
    float   AO = AmbientMap.SampleLevel(LinearWrap, TexCoord, 0.f).r;
    
    float3  Ambient = Compute_EnviromentLight(WorldNormal, V, Albedo, Roughness, Metallic, MBR);
    float3  BaseAmbient = max(Ambient * AO, Albedo * 0.05f);
	float3	ExtraColor = BaseAmbient + BaseEmissive;
	
    OUTPUT[ID.xy] = float4(ExtraColor + LightAccumulation, 1.f);
    return;
}
