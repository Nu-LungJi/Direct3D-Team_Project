#include "../../Engine/ShaderFiles/ShaderDefines.hlsl"

Texture2D g_DiffuseTexture : register(t0);
Texture2D g_NormalTexture : register(t1);
Texture2D g_SMROTexture : register(t2);
Texture2D g_EmissiveTexture : register(t3);

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
};

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out;
    
    Out.vPosition = mul(float4(In.vPosition, 1.f), g_matWVP);
    Out.vNormal = normalize(mul(float4(In.vNormal, 0.f), g_matWorld));
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), g_matWorld);
    Out.vProjPos = Out.vPosition;
    
    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
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

PS_OUT PSMain(PS_IN IN)
{
    PS_OUT Out;
    
    vector vMtrlDiffuse = g_DiffuseTexture.Sample(LinearWrap, IN.vTexcoord * 50.f);
    
    Out.vDiffuse = vMtrlDiffuse;
    Out.vNormal = float4(IN.vNormal.xyz * 0.5f + 0.5f, 0.f);
    Out.vSMRO = float4(0.f, 0.f, 0.f, 1.f);
    Out.vEmissive = float4(0.f, 0.f, 0.f, 1.f);
    
    return Out;
}
