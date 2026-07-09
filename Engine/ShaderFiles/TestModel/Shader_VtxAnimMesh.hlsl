#include "../ShaderDefines.hlsl"

Texture2D g_DiffuseTexture : register(t0);
Texture2D g_NormalTexture : register(t1);
Texture2D g_SMROTexture : register(t2);
Texture2D g_EmissiveTexture : register(t3);

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
    
    uint4 vBlendIndices : BLENDINDICES;
    float4 vBlendWeights : BLENDWEIGHT;
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
    
    float fWeightW = 1.f - (In.vBlendWeights.x + In.vBlendWeights.y + In.vBlendWeights.z);
    
    float4x4 BoneMatrix =
        g_BoneMatrices[In.vBlendIndices.x] * In.vBlendWeights.x +
        g_BoneMatrices[In.vBlendIndices.y] * In.vBlendWeights.y +
        g_BoneMatrices[In.vBlendIndices.z] * In.vBlendWeights.z +
        g_BoneMatrices[In.vBlendIndices.w] * fWeightW;
    
    vector vPosition = mul(float4(In.vPosition, 1.f), BoneMatrix);
    vector vNormal = mul(float4(In.vNormal, 0.f), BoneMatrix);
    
    Out.vPosition = mul(vPosition, g_matWVP);
    Out.vNormal = normalize(mul(vNormal, g_matWorld));
    Out.vTangent = normalize(mul(float4(In.vTangent, 0.f), g_matWorld));
    Out.vBinormal = normalize(mul(float4(In.vBinormal, 0.f), g_matWorld));
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), g_matWorld);
    Out.vProjPos = Out.vPosition;
    
    return Out;
}
struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float4 vTangent : TANGENT;
    float4 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
};

struct PS_OUT
{
    vector vDiffuse : SV_TARGET0;
    vector vNormal : SV_TARGET1;
    vector vSMRO : SV_TARGET2;
    vector vEmissive : SV_TARGET3;
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
    float3 LocalNormal = _NormalTex.Sample(LinearWrap, _TexCoord).rgb;
    LocalNormal = normalize(LocalNormal * 2.f - 1.f);
    float3x3 TBN = Make_TBNMatrix(_InNormal.xyz, _InTangent.xyz);

    float3 N = normalize(_InNormal.xyz);
    float3 T = normalize(_InTangent.xyz);
    
    T = normalize(T - dot(T, N) * N);
    float3 B = normalize(cross(N, T));
    
    float3 worldNormal = LocalNormal.x * T + LocalNormal.y * B + LocalNormal.z * N;

    return normalize(worldNormal);
}

PS_OUT PSMain(PS_IN IN)
{
    PS_OUT Out;
    
    float4 fDiffuse = g_DiffuseTexture.Sample(LinearWrap, IN.vTexcoord) * float4(AlbedoColor, ObjectAlpha);
    float3 fNormal = Compute_WorldNormal(g_NormalTexture, IN.vTexcoord, IN.vNormal, IN.vTangent) * NormalIntensity;
    float3 fMRO = g_SMROTexture.Sample(LinearWrap, IN.vTexcoord);
    
    float fFinalMetallic = fMRO.r * MetallicIntensity;
    float fFinalRoughness = fMRO.g * RoughnessIntensity;
    float fFinalAO = fMRO.b * AmbientIntensity;
    
    float3 fEmissive = g_EmissiveTexture.Sample(LinearWrap, IN.vTexcoord).rgb * EmissiveColor * EmissiveIntensity;
    
    if (fDiffuse.a == 0.0f)
        discard;

    Out.vDiffuse = fDiffuse;
    Out.vNormal = float4(fNormal * 0.5f + 0.5f, 1.f);
    Out.vSMRO = float4(fFinalMetallic, fFinalRoughness, fFinalAO, 1.f);
    Out.vEmissive = float4(fEmissive, 1.f);
    
    return Out;
}
