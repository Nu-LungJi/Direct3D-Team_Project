#include "../ShaderDefines.hlsl"

struct VS_IN
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
    float3 Normal   : NORMAL;
    float3 Tangent  : TANGENT;
};
struct VS_OUT
{
    float4 Position : SV_Position;
    float3 WorldPos : POSITION;
    float2 TexCoord : TEXCOORD0;
    float3 Normal   : NORMAL;
    float3 Tangent  : TANGENT;
};

VS_OUT VSMain(VS_IN IN)
{
    VS_OUT OUT;
    
    OUT.Position    = mul(float4(IN.Position, 1.f), g_matWVP);
    OUT.TexCoord    = IN.TexCoord;
    OUT.Normal      = normalize(mul(float4(IN.Normal, 0.0), g_matWorld).xyz);
    OUT.Tangent     = normalize(mul(float4(IN.Tangent, 0.0), g_matWorld).xyz);
    OUT.WorldPos    = mul(float4(IN.Position, 1.f), g_matWorld).xyz;
    
    return OUT;
}