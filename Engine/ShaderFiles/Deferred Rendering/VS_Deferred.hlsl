#include "../ShaderDefines.hlsl"

struct VS_IN
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
    float3 Normal : NORMAL;
};

struct VS_OUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Normal : NORMAL;
};

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out;
    
    Out.Position = float4(In.Position.xy, 1.0f, 1.0f);
    Out.TexCoord = In.TexCoord;
    Out.Normal = normalize(mul(float4(In.Normal, 1.f), g_matWorld));
    
    return Out;
}