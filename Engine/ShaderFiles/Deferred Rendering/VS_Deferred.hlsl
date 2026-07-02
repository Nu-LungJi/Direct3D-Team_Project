#include "../ShaderDefines.hlsl"

struct VS_IN
{
    float3 vPosition : POSITION;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;
    
    Out.vPosition = mul(vector(In.vPosition, 1.f), mul(mul(g_matWorld, g_matView), g_matProj));
    Out.vTexcoord = In.vTexcoord;
    
    return Out;
}