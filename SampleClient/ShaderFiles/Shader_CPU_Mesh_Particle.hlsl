#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"


#define MAX_LIGHT_COUNT     8
#define LIGHT_DIRECTIONAL   0
#define LIGHT_POINT         1
#define LIGHT_SPOTLIGHT     2

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;

    float4 vInstRow0 : INSTANCE_WORLD0;
    float4 vInstRow1 : INSTANCE_WORLD1;
    float4 vInstRow2 : INSTANCE_WORLD2;
    float4 vInstRow3 : INSTANCE_WORLD3;
    float4 vInstColor : INSTANCE_COLOR;
    float4 vInstEmissive : INSTANCE_EMISSIVE;
    float vInstLife : INSTANCE_LIFE;
    float vInstMaxLife : INSTANCE_MAXLIFE; 
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float4 vColor : COLOR0;
    float3 vNormal : NORMAL0;
    float3 vTangent : TANGENT0;
    float3 vBinormal : BINORMAL0;
    float4 vEmissive : COLOR1;
    float3 vWorldPos : TEXCOORD1;
    float life : TEXCOORD2;
    float maxLife : TEXCOORD3;
};

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;

    matrix matWorld = matrix(In.vInstRow0, In.vInstRow1, In.vInstRow2, In.vInstRow3);
    float4 vWorldPos = mul(float4(In.vPosition, 1.0f), matWorld);

    Out.vPosition = mul(vWorldPos, g_matViewProj);
    Out.vWorldPos = vWorldPos.xyz;
    Out.vTexcoord = In.vTexcoord;
    Out.vNormal = normalize(mul(In.vNormal, (float3x3) matWorld));
    Out.vTangent = normalize(mul(In.vTangent, (float3x3) matWorld));
    Out.vBinormal = normalize(mul(In.vBinormal, (float3x3) matWorld));
    Out.vColor = In.vInstColor;
    Out.vEmissive = In.vInstEmissive;
    Out.life = In.vInstLife; 
    Out.maxLife = In.vInstMaxLife;
    return Out;
}

Texture2D   AlbedoMap    : register(t0);
Texture2D   NormalMap    : register(t1);
Texture2D   SMROMap      : register(t2);
Texture2D   EmissiveMap : register(t3);
Texture2D   DepthMap    : register(t4);
Texture2D   NoiseMap    : register(t5);

//SamplerState g_LinearSampler : register(s0);

struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;
};

PS_OUT PSMain(VS_OUT In)
{
    PS_OUT Out = (PS_OUT) 0;
    
    float DepthData = DepthMap.Sample(LinearWrap, In.vTexcoord).r;
    
    [branch]
    if (DepthData >= 1.0f)
        discard;
    
    float4 noise = NoiseMap.Sample(LinearWrap, In.vTexcoord);
    
    float ratio = 1.0f - (In.life / In.maxLife);

    if (noise.r < ratio) 
        discard;
    
    float3 DepthWorld = ReconstructWorldPos(In.vTexcoord, DepthData);
    
    float4 AlbedoTex = AlbedoMap.Sample(LinearWrap, In.vTexcoord) * float4(AlbedoColor, ObjectAlpha) * In.vColor;
    if (AlbedoTex.a == 0.0f)
        discard;

    float3 Albedo = pow(AlbedoTex.rgb, 2.2f);

    float3 WorldNormal = Compute_WorldNormal(NormalMap, In.vTexcoord, In.vNormal, In.vTangent);
    WorldNormal = normalize(WorldNormal * NormalIntensity);

    float3 V = normalize(g_vCamPos - DepthWorld); //In.vWorldPos);
    float  NDV = max(dot(WorldNormal, V), 0.f);

    float3 SMRO = SMROMap.Sample(LinearWrap, In.vTexcoord).rgb;
    float fMetallic  = SMRO.r * MetallicIntensity;
    float fRoughness = SMRO.g * RoughnessIntensity;
    float fAmbient   = SMRO.b * AmbientIntensity;

    float3 MBR = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, fMetallic);

    float3 LightAccumulation = float3(0.f, 0.f, 0.f);

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
            float D = DistributionGGX(WorldNormal, H, fRoughness);
            float3 F = FresnelSchlick(max(dot(H, V), 0.f), MBR);
            float V_Spec = VisibilitySmithJointGGX(NDV, NDL, fRoughness);

            float3 Specular = D * F * V_Spec * SpecularIntensity;

            float3 kS = F;
            float3 kD = (1.0 - kS) * (1.0 - fMetallic);
            float3 Diffuse = kD * Albedo / PI;

            LightAccumulation += (Diffuse + Specular) * Radiance * NDL;
        }
    }

    // 인스턴스별 이미시브 + 오브젝트 이미시브 텍스처 둘 다 반영
    float3 texEmissive = EmissiveMap.Sample(LinearWrap, In.vTexcoord).rgb + EmissiveColor * EmissiveIntensity;
    texEmissive = pow(texEmissive, 2.2f);
    float3 instEmissive = In.vEmissive.rgb * In.vEmissive.a;

    float3 ConstantAmbient = Albedo * 0.05f * fAmbient;
    float3 FinalColor = ConstantAmbient + LightAccumulation + texEmissive + instEmissive;

    Out.vDiffuse = float4(FinalColor, AlbedoTex.a);
    return Out;
    
}
