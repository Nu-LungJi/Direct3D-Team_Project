#include "../ShaderDefines.hlsl"

struct VS_IN
{
    float3 Position : POSITION; 
    float2 TexCoord : TEXCOORD0;
    float3 Normal   : NORMAL;
};
struct VS_IN_BLEND
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
};
struct VS_OUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0; 
};

struct VS_OUT_BLEND
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float4 vTangent : TANGENT;
    float4 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
};

VS_OUT VSMain(VS_IN IN)
{
    VS_OUT OUT;
    
    OUT.Position    = float4(IN.Position.xy, 1.0f, 1.0f);
    OUT.TexCoord    = IN.TexCoord;
    
    return OUT;
}

VS_OUT_BLEND VSMain_Blend(VS_IN_BLEND IN)
{
    VS_OUT_BLEND Out;
    
    Out.vPosition = mul(float4(IN.vPosition, 1.f), g_matWVP);
    Out.vNormal = normalize(mul(float4(IN.vNormal, 0.f), g_matWorld));
    Out.vTangent = normalize(mul(float4(IN.vTangent, 0.f), g_matWorld));
    Out.vBinormal = normalize(mul(float4(IN.vBinormal, 0.f), g_matWorld));
    Out.vTexcoord = IN.vTexcoord;
    Out.vWorldPos = mul(float4(IN.vPosition, 1.f), g_matWorld);
    Out.vProjPos = Out.vPosition;
    
    return Out;
}
