#include "../ShaderHeader/SH_SamplerState.hlsli"
#include "../ShaderDefines.hlsl"

#define MAX_LIGHT_COUNT     8

#define LIGHT_DIRECTIONAL   0
#define LIGHT_POINT         1
#define LIGHT_SPOTLIGHT     2

#define MAX_REFLECTION_LOD  4.f

// Base Texture
Texture2D   AlbedoMap       : register(t0);
Texture2D   NormalMap       : register(t1);
Texture2D   SMROMap         : register(t2);
Texture2D   EmissiveMap     : register(t3);
Texture2D   DepthMap        : register(t4);

// Image Based Lighting
TextureCube IrridianceMap   : register(t7);
TextureCube PreFilterMap    : register(t8);
Texture2D   LUTMap          : register(t9);

cbuffer CB_OBJECT_PBR : register(b3)
{
    float4  AlbedoColor;

    float   NormalIntensity;
    float   RoughnessIntensity;
    float   MetallicIntensity;
    float   AmbientIntensity;
    float   SpecularIntensity;

    float3  EmissiveColor;
    float   EmissiveIntensity;

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

float3x3 Make_TBNMatrix(float3 _Normal, float3 _Tangent)
{
    float3 Normal = normalize(_Normal);
    float3 Tangent = normalize(_Tangent);

    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    
    float3 BiNormal = normalize(cross(Normal, Tangent));
    
    return float3x3(Tangent, BiNormal, Normal);
}
float3 Compute_WorldNormal(Texture2D _NormalTex, float2 _TexCoord, float4 _InNormal, float4 _InTangent)
{
    float3 LocalNormal = _NormalTex.Sample(SamplerWrap, _TexCoord).rgb;
    LocalNormal = normalize(LocalNormal * 2.f - 1.f);
    float3x3 TBN = Make_TBNMatrix(_InNormal.xyz, _InTangent.xyz);

    float3 N = normalize(_InNormal.xyz);
    float3 T = normalize(_InTangent.xyz);
    
    T = normalize(T - dot(T, N) * N);
    float3 B = normalize(cross(N, T));
    
    float3 worldNormal = LocalNormal.x * T + LocalNormal.y * B + LocalNormal.z * N;

    return normalize(worldNormal);
}

bool Compute_DynamicLight(DynamicLight _Light, float3 _WorldPosition, inout float3 L, inout float3 Radiance) {
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
        float MinimumDistance = 1.f;
        
        float3 LightVector = _Light.Position - _WorldPosition;
        float Distance = length(LightVector);
        
        if (Distance > _Light.LightRange) return false;

        float Attenuation = 1.f / max(Distance * Distance, 0.0001f);
        float DistanceByRange = Distance / _Light.LightRange;
        float Window = clamp(1.f - pow(DistanceByRange, 4.f), 0.f, 1.f);
        
        L = normalize(LightVector);
        Radiance = _Light.LightColor * _Light.LightIntensity * (Attenuation * Window * Window);

        return true;
    }
    // SpotLight Light PBR
    else if (_Light.LightType == LIGHT_SPOTLIGHT)
    {
        float MinimumDistance = 1.f;
        
        float3 LightVector = _Light.Position - _WorldPosition;
        float Distance = length(LightVector);
        
        if (Distance > _Light.LightRange) return false;
        
        // Decrease By Distance
        float Attenuation = 1.f / max(Distance * Distance, 0.0001f);
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

float       VisibilitySmithJointGGX(float NdotV, float NdotL, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    
    float lambdaV = NdotL * sqrt(max((-NdotV * a2 + NdotV) * NdotV + a2, 0.001f));
    float lambdaL = NdotV * sqrt(max((-NdotL * a2 + NdotL) * NdotL + a2, 0.001f));
    
    float  Denom = lambdaV + lambdaL;
    return Denom > 0.0f ? 0.5f / Denom : 0.0f;
}

float3      FresnelSchlick(float CTH, float3 MBR)
{
    float ClampCTH = clamp(CTH, 0.0f, 1.0f);
    return MBR + (1.0 - MBR) * pow(clamp(1.0 - ClampCTH, 0.0, 1.0), 5.0);
}

float3 ReconstructWorldPos(float2 uv, float depth)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0f - uv.y) * 2.0f - 1.0f;
    float z = depth;

    float4 ndcPos = float4(x, y, z, 1.0f);
    
    float4 worldPos = mul(ndcPos, g_matInvViewProj);
    return worldPos.xyz / worldPos.w;
}
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
    float DepthData = DepthMap.Sample(SamplerWrap, IN.TexCoord).r;
    
    [branch]
    if (DepthData >= 1.0f)  discard;

    float3 DepthWorld = ReconstructWorldPos(IN.TexCoord, DepthData);
    
    float3 WorldNormal = NormalMap.Sample(SamplerWrap, IN.TexCoord).rgb;
    WorldNormal = normalize(WorldNormal * 2.f - 1.f);
    
    float3 V = normalize(g_vCamPos - DepthWorld);
    float   R = reflect(-V, WorldNormal);

    float   NDV = max(dot(WorldNormal, V), 0.f);
    float3  AlbedoTex   = AlbedoMap.Sample(SamplerWrap, IN.TexCoord).rgb;

    float3  Albedo      = pow(AlbedoTex.rgb, 2.2f);
    float3  SMRO        = SMROMap.Sample(SamplerWrap, IN.TexCoord);
    float   Metallic    = SMRO.r;
    float   Roughness   = SMRO.g;
    
    // Metallic Material Based Reflection
    float3  MBR = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, Metallic);
    
    float3  LightAccumulation = float3(0.f, 0.f, 0.f);
    
    // Multiple Light Process
    [unroll(MAX_LIGHT_COUNT)]
    for (int i = 0; i < g_iLightCount; ++i)
    {
        float3 L, Radiance;
    
        [branch]
        if (!Compute_DynamicLight(AffectedLight[i], DepthWorld, L, Radiance))
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
    
            LightAccumulation += (Diffuse + Specular) * Radiance * NDL; //(Diffuse + Specular) * Radiance * NDL;

        }
    }

    float3 Emissive = EmissiveMap.Sample(SamplerWrap, IN.TexCoord).rgb;
    // Enviroment Light Process
    //float3  Ambient  = Compute_IBL(WorldNormal, V, Albedo, Roughness, Metallic, MBR);
    //float   Occlusion = clamp(1.0f + dot(R, WorldNormal), 0.0f, 1.0f);
    //Ambient *= Occlusion * Occlusion;
    //LightAccumulation += Ambient;
    //LightAccumulation = pow(LightAccumulation, 1.f / 2.2f);
    
    OUT.Diffuse = float4(LightAccumulation, 1.f) + float4(Emissive, 1.f);
 
    return OUT;
}

PS_OUT PSMain_Blend(PS_IN_BLEND IN)
{
    PS_OUT OUT;

    float4 AlbedoTex = AlbedoMap.Sample(SamplerWrap, IN.TexCoord) * AlbedoColor;
    float3 Albedo = pow(AlbedoTex.rgb, 2.2f);
    
    if (AlbedoTex.a == 0.0f)
        discard;
    
    float3 WorldNormal = Compute_WorldNormal(NormalMap, IN.TexCoord, IN.Normal, IN.Tangent);
    WorldNormal = normalize(WorldNormal * NormalIntensity);
    float3 V    = normalize(g_vCamPos - IN.WorldPos.xyz);
    float  R    = reflect(-V, WorldNormal);
    float  NDV  = max(dot(WorldNormal, V), 0.f);

    float3 SMRO         = SMROMap.Sample(SamplerWrap, IN.TexCoord);
    float fMetallic     = SMRO.r * MetallicIntensity;
    float fRoughness    = SMRO.g * RoughnessIntensity;
    float fAmbient      = SMRO.b * AmbientIntensity;
    
    // Metallic Material Based Reflection
    float3 MBR = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, fMetallic);
    
    float3 LightAccumulation = float3(0.f, 0.f, 0.f);
    
    [unroll(MAX_LIGHT_COUNT)]
    for (int i = 0; i < g_iLightCount; ++i)
    {
        float3 L, Radiance;
    
        [branch]
        if (!Compute_DynamicLight(AffectedLight[i], IN.WorldPos.xyz, L, Radiance))
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
    float3 fEmissive = EmissiveMap.Sample(SamplerWrap, IN.TexCoord).rgb * EmissiveColor * EmissiveIntensity;
    fEmissive = pow(fEmissive, 2.2f);
    
    float3 ConstantAmbient = Albedo * 0.05f * fAmbient;
    float3 FinalColor = ConstantAmbient + LightAccumulation + fEmissive;
    
    OUT.Diffuse = float4(FinalColor, AlbedoTex.a);
    return OUT;
}