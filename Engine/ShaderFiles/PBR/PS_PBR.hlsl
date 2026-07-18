#include "../ShaderHeader/SH_CommonFunction.hlsli"

// Base Texture
Texture2D AlbedoMap : register(t0);
Texture2D NormalMap : register(t1);
Texture2D SMROMap : register(t2);
Texture2D EmissiveMap : register(t3);
Texture2D DepthMap : register(t4);
Texture2D AmbientMap : register(t5);
Texture2D ShadowMap : register(t6);

// Image Based Lighting
TextureCube IrridianceMap : register(t7);
TextureCube PreFilterMap : register(t8);
Texture2D LUTMap : register(t9);

Texture2D OriginColor : register(t10);

static const float ShadowSmoothness = 1.5f;
static const float ShadowBrightness = 0.45f;
static const float2 ShadowMapResolution = { 1280.f, 720.f };

static const float2 PoissonDisk[8] =
{
    float2(0.000000, 0.000000), float2(0.527837, -0.085868), float2(-0.040062, 0.536087), float2(-0.670445, -0.179949),
    float2(-0.419418, -0.616039), float2(0.440453, 0.639399), float2(-0.757088, 0.349334), float2(0.574619, -0.715851)
};

struct PS_IN
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};
struct PS_IN_BLEND
{
    float4 Position : SV_POSITION;
    float4 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float4 Binormal : BINORMAL;
    float2 TexCoord : TEXCOORD0;
    float4 WorldPos : TEXCOORD1;
    float4 ProjPos : TEXCOORD2;
};

struct PS_OUT
{
    vector Diffuse : SV_TARGET0;
};

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


float3 Compute_EnviromentLight(float3 N, float3 V, float3 albedo, float _Roughness, float _Metallic, float3 MBR)
{
    float NDV = max(dot(N, V), 0.0);
    
    float3 F = MBR + (max(float3(1.0 - _Roughness, 1.0 - _Roughness, 1.0 - _Roughness), MBR) - MBR) * pow(clamp(1.0 - NDV, 0.0, 1.0), 5.0);
    
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= (1.0 - _Metallic);
    
    float3 irradiance = IrridianceMap.Sample(LinearWrap, N).rgb;
    float3 DiffuseAmbient = kD * irradiance * albedo;
    
    float3 R = reflect(-V, N);
    
    float3 PreFilteredDiffuse = PreFilterMap.SampleLevel(LinearWrap, R, _Roughness * MAX_REFLECTION_LOD).rgb;
    
    float2 lutUV = float2(NDV, _Roughness);
    float2 brdf = LUTMap.Sample(LinearWrap, lutUV).rg;
    
    float3 SpecularAmbient = PreFilteredDiffuse * (F * brdf.x + brdf.y);
    
    return (DiffuseAmbient + SpecularAmbient);
}

float Get_GradientNoise(float2 _PixelPos)
{
    return frac(sin(dot(_PixelPos, float2(12.9898, 78.233))) * 43758.5453123);
}

float Compute_NormalShadow(float4 _WorldPos, float2 _TexCoord)
{
    float4 LightPos = mul(_WorldPos, g_matShadowLightViewProj);
    
    float2 ShadowMapUV;
    ShadowMapUV.x = (LightPos.x / LightPos.w) * +0.5f + 0.5f;
    ShadowMapUV.y = (LightPos.y / LightPos.w) * -0.5f + 0.5f;
    
    float CurrentPixelDepth = LightPos.z;
    CurrentPixelDepth -= 0.0005f; // Depth Bias
    
    float ShadowMapDepth = ShadowMap.Sample(LinearWrap, ShadowMapUV).r;
    
    float ShadowFactor = 1.0f;
    
    // CurrentPixelDepth = 월드의 어느 한 지점을 Shadow 카메라를 기준으로 평가한 깊이.
    // ShadowMapDepth    = Shadow 카메라에 기록했던 깊이
    
    // CurrentPixelDepth > ShadowMapDepth : 가려진다.(어두워짐(그림자))
    // CurrentPixelDepth = ShadowMapDepth : 똑같다.(밝아짐)
    // CurrentPixelDepth < ShadowMapDepth : 허공
    
    [branch]
    if (CurrentPixelDepth > ShadowMapDepth)
    {
        ShadowFactor = ShadowBrightness;
    }
    
    return ShadowFactor;
}
float Compute_PCFShadow(float4 _WorldPos, float2 _TexCoord)
{
    float4 LightPos = mul(_WorldPos, g_matShadowLightViewProj);
    
    float2 ShadowMapUV;
    ShadowMapUV.x = (LightPos.x / LightPos.w) * +0.5f + 0.5f;
    ShadowMapUV.y = (LightPos.y / LightPos.w) * -0.5f + 0.5f;

    float CurrentPixelDepth = LightPos.z;
    CurrentPixelDepth -= 0.0005f;

    // Sampling Near Pixels (3 X 3)
    float ShadowMapWidth, ShadowMapWHeight;
    ShadowMap.GetDimensions(ShadowMapWidth, ShadowMapWHeight);
    float2 TexelSize = float2(1.0f / ShadowMapWidth, 1.0f / ShadowMapWHeight);
    
    float ShadowFactorSum = 0.f;
    
    [unroll]
    for (int x = -1; x <= 1; ++x)
    {
        [unroll]
        for (int y = -1; y <= 1; ++y)
        {
            float2 SamplingPos = float2(x, y) * TexelSize;
            float ShadowMapDepth = ShadowMap.SampleCmpLevelZero(ShadowSampler, ShadowMapUV + SamplingPos, CurrentPixelDepth).r;
            
            [branch]
            if (CurrentPixelDepth - 0.0005f > ShadowMapDepth)
            {
                ShadowFactorSum += ShadowBrightness;
            }
            else
            {
                ShadowFactorSum += 1.0f;
            }
        }
        
    }
    return ShadowFactorSum / 9.0f;
}
float Compute_SmoothShadow(float4 _WorldPos, float2 _TexCoord, float2 _PixelPos)
{
    float4 LightPos = mul(_WorldPos, g_matShadowLightViewProj);
    
    float2 ShadowMapUV;
    ShadowMapUV.x = (LightPos.x / LightPos.w) * +0.5f + 0.5f;
    ShadowMapUV.y = (LightPos.y / LightPos.w) * -0.5f + 0.5f;
    
    float CurrentPixelDepth = LightPos.z;
    CurrentPixelDepth -= 0.0005f; // Depth Bias
    
    float RandomNoise = Get_GradientNoise(_PixelPos);
    float RandomAngle = RandomNoise * 2.f * PI;
    
    float CosAngle = cos(RandomAngle);
    float SinAngle = sin(RandomAngle);
    
    float2x2 RotationMat = float2x2(CosAngle, -SinAngle, SinAngle, CosAngle);
    
    // 주변 ShadowSmoothness 반경까지 Sampling
    float2 SamplingRange = 1.f / ShadowMapResolution * ShadowSmoothness;
    
    float ShadowFactor = 0.0f;

    [unroll]
    for (int i = 0; i < 8; ++i)
    {
        float2 RotatedOffset = mul(PoissonDisk[i], RotationMat);
        
        float2 SampleUV = ShadowMapUV + (RotatedOffset * SamplingRange);
        
        // SampleCmpLevelZero : Texture2D(ShadowMap)의 깊이와 CompareValue(CurrentPixelDepth) 를 비교했을 때 
        // CompareValue가 크면 1, 아니면 0 반환.(x값에 결과값 저장)
        ShadowFactor += ShadowMap.SampleCmpLevelZero(ShadowSampler, SampleUV, CurrentPixelDepth).x;
    }
    
    return ShadowFactor / 8.f;
}

//[earlydepthstencil]         // Test : Block Pixel OverDraw
PS_OUT PSMain(PS_IN IN)
{
    PS_OUT OUT;
    float DepthData = DepthMap.Sample(LinearWrap, IN.TexCoord).r;
    
    [branch]
    if (DepthData >= 1.0f)
    {
        OUT.Diffuse = float4(0.f, 0.f, 1.f, 1.f);
        return OUT;
    }
    
    float4 DepthWorld = Convert_WorldPosByDepth(DepthData, IN.TexCoord);
    
    float ShadowFactor = Compute_SmoothShadow(DepthWorld, IN.TexCoord, IN.Position.xy);
    
    float3 WorldNormal = NormalMap.Sample(LinearWrap, IN.TexCoord).rgb;
    WorldNormal = normalize(WorldNormal * 2.f - 1.f);
    
    float3 V = normalize(g_vCamPos - DepthWorld.xyz);
    float R = reflect(-V, WorldNormal);

    float NDV = max(dot(WorldNormal, V), 0.f);
    float3 AlbedoTex = AlbedoMap.Sample(LinearWrap, IN.TexCoord).rgb;
    
    float3 Albedo = pow(AlbedoTex.rgb, 2.2f);
    float3 SMRO = SMROMap.Sample(LinearWrap, IN.TexCoord);
    float Metallic = SMRO.r;
    float Roughness = SMRO.g;
   
    // Metallic Material Based Reflection
    float3 MBR = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, Metallic);
    
    float3 LightAccumulation = float3(0.f, 0.f, 0.f);
    
    // Multiple Light Process
    [unroll(MAX_LIGHT_COUNT)]
    for (int i = 0; i < LightCount; ++i)
    {
        float3 L, Radiance;
    
        [branch]
        if (!Compute_DynamicLight(DepthWorld.xyz, AffectedLight[i], L, Radiance))
            continue;
    
        float RawNDL = dot(WorldNormal, L);
    
        [branch]
        if (RawNDL > 0.f)
        {
            float NDL = clamp(RawNDL, 0.f, 1.f);
    
            float3 H = normalize(V + L);
            float D = DistributionGGX(WorldNormal, H, Roughness);
            float3 F = FresnelSchlick(max(dot(H, V), 0.f), MBR);
    
            float V_Spec = VisibilitySmithJointGGX(NDV, NDL, Roughness);
    
            float3 Specular = D * F * V_Spec;

            float3 kS = F;
            float3 kD = (1.0 - kS) * (1.0 - Metallic);
            float3 Diffuse = kD * Albedo / PI;
    
            LightAccumulation += (Diffuse + Specular) * Radiance * NDL;
        }
    }
    float3 BaseEmissive = EmissiveMap.Sample(LinearWrap, IN.TexCoord).rgb;
    
    // Enviroment Light Process
    float3 Ambient = Compute_EnviromentLight(WorldNormal, V, Albedo, Roughness, Metallic, MBR);
    
    float AO = AmbientMap.Sample(LinearWrap, IN.TexCoord).r;

    LightAccumulation *= ShadowFactor;
    LightAccumulation *= AO;
    
    OUT.Diffuse = float4(LightAccumulation + BaseEmissive, 1.f);
    return OUT;
}

PS_OUT PSMain_Blend(PS_IN_BLEND IN)
{
    PS_OUT OUT;

	float4 AlbedoTex = AlbedoMap.Sample(LinearWrap, IN.TexCoord) * float4(AlbedoColor, ObjectAlpha);;
    float3 Albedo = pow(AlbedoTex.rgb, 2.2f);
    
    if (AlbedoTex.a == 0.0f)
        discard;
    
	OUT.Diffuse = float4(AlbedoTex.rgb, 1.f);
	return OUT;
	
    float3 WorldNormal = Compute_WorldNormal(NormalMap, IN.TexCoord, IN.Normal, IN.Tangent);
    WorldNormal = normalize(WorldNormal * NormalIntensity);
    float3 V = normalize(g_vCamPos - IN.WorldPos.xyz);
    float R = reflect(-V, WorldNormal);
    float NDV = max(dot(WorldNormal, V), 0.f);

    float3 SMRO = SMROMap.Sample(LinearWrap, IN.TexCoord);
    float fMetallic = SMRO.r * MetallicIntensity;
    float fRoughness = SMRO.g * RoughnessIntensity;
    float fAmbient = SMRO.b * AmbientIntensity;
    
    // Metallic Material Based Reflection
    float3 MBR = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, fMetallic);
    
    float3 LightAccumulation = float3(0.f, 0.f, 0.f);
    
    [unroll(MAX_LIGHT_COUNT)]
    for (int i = 0; i < LightCount; ++i)
    {
        float3 L, Radiance;
    
        [branch]
        if (!Compute_DynamicLight(IN.WorldPos.xyz, AffectedLight[i], L, Radiance))
            continue;
    
        float RawNDL = dot(WorldNormal, L);
    
        [branch]
        if (RawNDL > 0.f)
        {
            float NDL = clamp(RawNDL, 0.f, 1.f);
        
            float3 H = normalize(V + L);
            float D = DistributionGGX(WorldNormal, H, fRoughness);
            float3 F = FresnelSchlick(max(dot(H, V), 0.f), MBR);
    
            float V_Spec = VisibilitySmithJointGGX(NDV, NDL, fRoughness);
    
            float3 Specular = D * F * V_Spec;

            float3 kS = F;
            float3 kD = (1.0 - kS) * (1.0 - fMetallic);
            float3 Diffuse = kD * Albedo / PI;
    
            LightAccumulation += (Diffuse + Specular) * Radiance * NDL;
        }
    }
    float3 fEmissive = EmissiveMap.Sample(LinearWrap, IN.TexCoord).rgb * EmissiveColor * EmissiveIntensity;
	fEmissive = pow(fEmissive, 2.2f);
	
    float3 ConstantAmbient = Albedo * 0.05f * fAmbient;
    float3 FinalColor = ConstantAmbient + LightAccumulation + fEmissive;
    
    OUT.Diffuse = float4(FinalColor, AlbedoTex.a);
    
    return OUT;
}
