
#include "ShaderDefines.hlsl"

TextureCube g_SkyTexture : register(t0);

struct VS_IN
{
    float3 position : POSITION;
    float4 color : COLOR0;
};

struct VS_OUT
{
    float4 position : SV_POSITION;
    float3 direction : TEXCOORD0;
};

VS_OUT VSMain(VS_IN input)
{
    VS_OUT output;
    output.position = mul(float4(input.position, 1.f), g_matWVP);
    output.position.z = output.position.w * 0.999999f;
    output.direction = input.position;
    return output;
}

float4 PSMain(VS_OUT input) : SV_TARGET0
{
    return float4(g_SkyTexture.Sample(LinearWrap, normalize(input.direction)).rgb, 1.f);
}

