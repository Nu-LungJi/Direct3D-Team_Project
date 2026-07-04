#include "../ShaderDefines.hlsl"

struct VS_IN
{
    float3 Position : POSITION; 
    float3 Normal   : NORMAL;
    float3 Tangent  : TANGENT;
    float3 BiNormal : BINORMAL;
    float2 TexCoord : TEXCOORD0;
};
struct VS_OUT
{
    float4 Position : SV_POSITION;
    float4 Normal   : NORMAL;
    float4 Tangent  : TANGENT;
    float4 BiNormal : BINORMAL;
    float2 TexCoord : TEXCOORD0;
    float4 WorldPos : TEXCOORD1;
    float4 ProjPos  : TEXCOORD2;
};

VS_OUT VSMain(VS_IN IN)
{
    VS_OUT OUT;
    
    OUT.Position    = mul(float4(IN.Position, 1.f), g_matWVP);
    OUT.TexCoord    = IN.TexCoord;
    OUT.Normal      = float4(normalize(mul(IN.Normal, (float3x3) g_matWorld)), 0.0f);
    OUT.Tangent     = normalize(mul(float4(IN.Tangent, 0.f), g_matWorld));
    OUT.BiNormal    = normalize(mul(float4(IN.BiNormal, 0.f), g_matWorld));
    OUT.WorldPos    = mul(float4(IN.Position, 1.f), g_matWorld);
    OUT.ProjPos     = OUT.Position;
    
    return OUT;
}