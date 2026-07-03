#include "../ShaderHeader/SH_SamplerState.hlsli"
#include "../ShaderDefines.hlsl"

#define MAX_LIGHT_COUNT     8

#define LIGHT_DIRECTIONAL   0
#define LIGHT_POINT         1
#define LIGHT_SPOTLIGHT     2

#define MAX_REFLECTION_LOD  4.f

// Base Texture
Texture2D   AlbedoMap     : register(t0);
Texture2D   NormalMap     : register(t1);
Texture2D   RoughnessMap  : register(t2);
Texture2D   MetallicMap   : register(t3);

// Image Based Lighting
TextureCube IrridianceMap : register(t4);
TextureCube PreFilterMap  : register(t5);
Texture2D   LUTMap        : register(t6);

cbuffer CB_OBJECT_PBR   : register(b3)
{
    float3  AlbedoValue;
    float   RoughnessValue;
    float   MetallicValue;
    float3  Padding;
};

cbuffer CB_LIGHT_BUFFER  : register(b4)
{
    DynamicLight AffectedLight[MAX_LIGHT_COUNT];
    int          g_iLightCount;
    float3       g_LightPadding;
}

struct PS_IN
{
    float4 Position : SV_Position;
    float3 WorldPos : POSITION;
    float2 TexCoord : TEXCOORD0;
    float3 Normal   : NORMAL;
    float3 Tangent  : TANGENT;
};

struct PS_OUT
{
    float4 Diffuse : SV_TARGET0;
};

float3x3    Make_TBNMatrix(float3 _Normal, float3 _Tangent)
{
    float3 Normal = normalize(_Normal);
    float3 Tangent = normalize(_Tangent);

    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal); 
    
    float3 BiNormal = normalize(cross(Normal, Tangent));
    
    return float3x3(Tangent, BiNormal, Normal);
}
float3      Compute_WorldNormal(PS_IN IN)
{
    float3x3 TBN = Make_TBNMatrix(IN.Normal, IN.Tangent);

    float3 Normal = NormalMap.Sample(SamplerWrap, IN.TexCoord).rgb;
    
    Normal = Normal * 2.f - 1.f;
    
    return normalize(mul(Normal, TBN));
}

bool        Compute_DynamicLight(DynamicLight _Light, float3 _WorldPosition, out float3 L, out float3 Radiance) {
    // Directional Light PBR
    [branch]
    if      (_Light.LightType == LIGHT_DIRECTIONAL)
    {
        L = normalize(-_Light.LightDirection);
        Radiance = _Light.LightColor * _Light.LightIntensity;
        return true;
    }
    // Point Light PBR
    else if (_Light.LightType == LIGHT_POINT)
    {
        float3 LightVector = _Light.Position - _WorldPosition;
        float Distance = length(LightVector);
        
        if (Distance > _Light.LightRange) return false;

        float Attenuation = 1.f / (Distance * Distance + 1.f);
        float DistanceByRange = Distance / _Light.LightRange;
        float Window = clamp(1.f - pow(DistanceByRange, 4.f), 0.f, 1.f);
        
        L = normalize(LightVector);
        Radiance = _Light.LightColor * _Light.LightIntensity * (Attenuation * Window * Window);

        return true;
    }
    // SpotLight Light PBR
    else if (_Light.LightType == LIGHT_SPOTLIGHT)
    {
        float3 LightVector = _Light.Position - _WorldPosition;
        float Distance = length(LightVector);
        
        if (Distance > _Light.LightRange) return false;
        
        // Decrease By Distance
        float Attenuation = 1.f / (Distance * Distance + 1.f);
        float DistanceByRange = Distance / _Light.LightRange;
        float Window = clamp(1.f - pow(DistanceByRange, 4.f), 0.f, 1.f);
        float DistanceFade = Attenuation * Window * Window;
        
        L = normalize(LightVector);
        
         // Decrease By SpotLight Cone
        float CosAngle = dot(-L, normalize(_Light.LightDirection));
        float Num   = CosAngle - _Light.OuterAttanuation;
        float DeNum = _Light.InnerAttanuation - _Light.OuterAttanuation;
        float ConeFade = clamp(Num / max(0.000001f, DeNum), 0.f, 1.f);
        
        Radiance = _Light.LightColor * _Light.LightIntensity * (DistanceFade * ConeFade * ConeFade);
        
        return true;
    }
    
    return false;
}
float       DistributionGGX(float3 N, float3 H, float _Roughness)
{
    float R = _Roughness * _Roughness;
    float R2 = R * R;
    
    float NDH = max(0.f, dot(N, H));
    float NDH2 = NDH * NDH;
    
    float Num = R2;
    float Denom = (NDH2 * (R2 - 1.0) + 1.0);
    Denom = PI * Denom * Denom;
	
    return Num / max(0.000001f, Denom);
}

//float       GeometrySchlickGGX(float NDV, float _Roughness)
//{
//    float R = (_Roughness + 1.0);
//    float K = (R * R) / 8.0;
//
//    float Num = NDV;
//    float Denom = NDV * (1.0 - K) + K;
//	
//    return Num / Denom;
//}
//float       GeometrySmith(float3 N, float3 V, float3 L, float _Roughness)
//{
//    float NDV = max(dot(N, V), 0.0);
//    float NDL = max(dot(N, L), 0.0);
//    float GGX2 = GeometrySchlickGGX(NDV, _Roughness);
//    float GGX1 = GeometrySchlickGGX(NDL, _Roughness);
//
//    return GGX1 * GGX2;
//}
//float       GeometryVisibilitySmith(float NDV, float NDL, float _Roughness)
//{
//    float r = (_Roughness + 1.0);
//    float k = (r * r) / 8.0;
//
//    float gV = NDV / (NDV * (1.0 - k) + k);
//    float gL = NDL / (NDL * (1.0 - k) + k);
//    
//    return (gV * gL) / 4.0;
//}
float       VisibilitySmithJointGGX(float NdotV, float NdotL, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    
    float lambdaV = NdotL * sqrt((-NdotV * a2 + NdotV) * NdotV + a2);
    float lambdaL = NdotV * sqrt((-NdotL * a2 + NdotL) * NdotL + a2);
    
    float Denom = lambdaV + lambdaL;
    
    if (Denom > 0.0)
    {
        return 0.5 / Denom;
    }
    return 0.0;
}

float3      FresnelSchlick(float CTH, float3 MBR)
{
    return MBR + (1.0 - MBR) * pow(clamp(1.0 - CTH, 0.0, 1.0), 5.0);
}
//float3      FresnelSchlickRoughness(float CTH, float3 MBR, float roughness)
//{
//    return MBR + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), MBR) - MBR) * pow(clamp(1.0 - CTH, 0.0, 1.0), 5.0);
//}
//float3      Compute_CookTorranceBRDF(float3 N, float3 V, float3 H, float3 L, float _Roughness, float MBR, float _NDV, float _NDL)
//{
//    float  D = DistributionGGX(N, H, _Roughness);
//    float  G = GeometrySmith(N, V, L, _Roughness);
//    float3 F = FresnelSchlick(max(dot(H, V), 0.f), MBR);
//    
//    // Specular
//    float Numerator = D * G * F;
//    float Denominator = 4.f * _NDV * _NDL;
//    
//    return Numerator / max(0.0001f, Denominator);
//}
float3      Compute_IBL(float3 N, float3 V, float3 albedo, float _Roughness, float _Metallic, float3 MBR)
{
    float NDV = max(dot(N, V), 0.0);
    
    float3 F = MBR + (max(float3(1.0 - _Roughness, 1.0 - _Roughness, 1.0 - _Roughness), MBR) - MBR) * pow(clamp(1.0 - NDV, 0.0, 1.0), 5.0);
    
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= (1.0 - _Metallic);
    
    float3 irradiance = IrridianceMap.Sample(SamplerClamp, N).rgb;
    float3 DiffuseAmbient = kD * irradiance * albedo;
    
    float3 R = reflect(-V, N);
    
    float3 PreFilteredDiffuse = PreFilterMap.SampleLevel(SamplerClamp, R, _Roughness * MAX_REFLECTION_LOD).rgb;
    
    float2 lutUV = float2(NDV, _Roughness);
    float2 brdf = LUTMap.Sample(SamplerClamp, lutUV).rg;
    
    float3 SpecularAmbient = PreFilteredDiffuse * (F * brdf.x + brdf.y);
    
    return (DiffuseAmbient + SpecularAmbient);
}

//[earlydepthstencil]         // Test : Block Pixel OverDraw
PS_OUT PSMain(PS_IN IN)
{
    PS_OUT OUT;
    
    OUT.Diffuse = AlbedoMap.Sample(SamplerClamp, IN.TexCoord);
    
    return OUT;
    
    
    float3 WorldNormal = Compute_WorldNormal(IN);
    float3 V = normalize(g_vCamPos - IN.WorldPos);
    float  R = reflect(-V, WorldNormal);

    float  NDV = max(dot(WorldNormal, V), 0.f);

    float3  Albedo      = pow(AlbedoMap.Sample(SamplerWrap, IN.TexCoord).rgb, 2.2f) * AlbedoValue;
    float   Roughness   = RoughnessMap.Sample(SamplerWrap, IN.TexCoord).r * RoughnessValue;
    float   Metallic    = MetallicMap.Sample(SamplerWrap, IN.TexCoord).r * MetallicValue;
    
    // Metallic Material Based Reflection
    float3  MBR = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, Metallic);
    
    float3  LightAccumulation = float3(0.f, 0.f, 0.f);

    // Multiple Light Process
    [unroll(MAX_LIGHT_COUNT)]
    for (int i = 0; i < g_iLightCount; ++i)
    {
        float3 L, Radiance;
        
        [branch]
        if (!Compute_DynamicLight(AffectedLight[i], IN.WorldPos, L, Radiance))    continue;
        
        float NDL = max(dot(WorldNormal, L), 0.f);
        
        [branch]
        if (NDL > 0.f)
        {
            float3  H = normalize(V + L);
            float   D = DistributionGGX(WorldNormal, H, Roughness);
            float3  F = FresnelSchlick(max(dot(H, V), 0.f), MBR);
        
            float   V_Spec = VisibilitySmithJointGGX(NDV, NDL, Roughness);
        
            float3  Specular = D * F * V_Spec;
        
            float3  kS = F;
            float3  kD = (1.0 - kS) * (1.0 - Metallic);
            float3  Diffuse = kD * Albedo / PI;
        
            LightAccumulation += (Diffuse + Specular) * Radiance * NDL;
        }
    }
    
    // Enviroment Light Process
    float3  Ambient  = Compute_IBL(WorldNormal, V, Albedo, Roughness, Metallic, MBR);
    float   Occlusion = clamp(1.0f + dot(R, WorldNormal), 0.0f, 1.0f);
    Ambient *= Occlusion * Occlusion;
    LightAccumulation += Ambient;
    
    OUT.Diffuse = float4(LightAccumulation, 1.f);
    
    return OUT;
}