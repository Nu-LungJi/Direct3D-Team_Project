#include "../ShaderHeader/SH_CommonFunction.hlsli"

// Base Texture
Texture2D   AlbedoMap       : register(t0);
Texture2D   NormalMap       : register(t1);
Texture2D   SMROMap         : register(t2);
Texture2D   EmissiveMap     : register(t3);
Texture2D   DepthMap        : register(t4);
Texture2D   AmbientMap      : register(t5);

// Image Based Lighting
TextureCube IrridianceMap   : register(t7);
TextureCube PreFilterMap    : register(t8);
Texture2D   LUTMap          : register(t9);

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
    
    float3 irradiance = IrridianceMap.Sample(LinearWrap, N).rgb;
    float3 DiffuseAmbient = kD * irradiance * albedo;
    
    float3 R = reflect(-V, N);
    
    float3 PreFilteredDiffuse = PreFilterMap.SampleLevel(LinearWrap, R, _Roughness * MAX_REFLECTION_LOD).rgb;
    
    float2 lutUV = float2(NDV, _Roughness);
    float2 brdf = LUTMap.Sample(LinearWrap, lutUV).rg;
    
    float3 SpecularAmbient = PreFilteredDiffuse * (F * brdf.x + brdf.y);
    
    return (DiffuseAmbient + SpecularAmbient);
}

//[earlydepthstencil]         // Test : Block Pixel OverDraw
PS_OUT PSMain(PS_IN IN)
{
    PS_OUT OUT;
    float DepthData = DepthMap.Sample(LinearWrap, IN.TexCoord).r;
    
    [branch]
    if (DepthData >= 1.0f)  discard;

    float3 DepthWorld = ReconstructWorldPos(IN.TexCoord, DepthData);
    
    float3 WorldNormal = NormalMap.Sample(LinearWrap, IN.TexCoord).rgb;
    WorldNormal = normalize(WorldNormal * 2.f - 1.f);
    
    float3 V = normalize(g_vCamPos - DepthWorld);
    float   R = reflect(-V, WorldNormal);

    float   NDV = max(dot(WorldNormal, V), 0.f);
    float3 AlbedoTex = AlbedoMap.Sample(LinearWrap, IN.TexCoord).rgb;
    
   

    float3  Albedo      = pow(AlbedoTex.rgb, 2.2f);
    float3 SMRO = SMROMap.Sample(LinearWrap, IN.TexCoord);
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
    
            LightAccumulation += (Diffuse + Specular) * Radiance * NDL;

        }
    }

    float3 Emissive = EmissiveMap.Sample(LinearWrap, IN.TexCoord).rgb;
    // Enviroment Light Process
    
    float3 Ambient = Compute_IBL(WorldNormal, V, Albedo, Roughness, Metallic, MBR);
    //LightAccumulation += Ambient * 3.f;
      
    float AO = AmbientMap.Sample(LinearWrap, IN.TexCoord).r;
   // LightAccumulation = pow(LightAccumulation, 1.f / 2.2f);
    
   // LightAccumulation *= AO;
    OUT.Diffuse = float4(LightAccumulation, 1.f) + float4(Emissive, 1.f);
    OUT.Diffuse *= AO;
    return OUT;
}

PS_OUT PSMain_Blend(PS_IN_BLEND IN)
{
    PS_OUT OUT;

    float4 AlbedoTex = AlbedoMap.Sample(LinearWrap, IN.TexCoord) * float4(AlbedoColor, ObjectAlpha);;
    float3 Albedo = pow(AlbedoTex.rgb, 2.2f);
    
    if (AlbedoTex.a == 0.0f)
        discard;
    
    float3 WorldNormal = Compute_WorldNormal(NormalMap, IN.TexCoord, IN.Normal, IN.Tangent);
    WorldNormal = normalize(WorldNormal * NormalIntensity);
    float3 V    = normalize(g_vCamPos - IN.WorldPos.xyz);
    float  R    = reflect(-V, WorldNormal);
    float  NDV  = max(dot(WorldNormal, V), 0.f);

    float3 SMRO = SMROMap.Sample(LinearWrap, IN.TexCoord);
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
    float3 fEmissive = EmissiveMap.Sample(LinearWrap, IN.TexCoord).rgb * EmissiveColor * EmissiveIntensity;
    fEmissive = pow(fEmissive, 2.2f);
    
    float3 ConstantAmbient = Albedo * 0.05f * fAmbient;
    float3 FinalColor = ConstantAmbient + LightAccumulation + fEmissive;
    
    OUT.Diffuse = float4(FinalColor, AlbedoTex.a);
    return OUT;
}
