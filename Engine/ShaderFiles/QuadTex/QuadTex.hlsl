#include "../ShaderHeader/SH_SamplerState.hlsli"
#include "../ShaderDefines.hlsl"

Texture2D tex : register(t0);

struct VS_IN
{
    float3 posL : POSITION;
    float2 uv : TEXCOORD;
};

struct PS_IN
{
    float4 posH : SV_POSITION;
    float2 uv : TEXCOORD;
};

PS_IN VSMain(VS_IN vin)
{
    PS_IN output;
    output.posH = mul(float4(vin.posL, 1.f), g_matWVP);

    output.uv = vin.uv;
    return output;
}

// Pixel Shader
float4 PSMain(PS_IN input) : SV_Target
{
    float4 texColor = tex.Sample(SamplerWrap, input.uv);
    if (texColor.a == 0.0f) discard;
    return texColor;
}
float4 PSMain_NonAlpha(PS_IN input) : SV_Target
{
    return tex.Sample(SamplerWrap, input.uv);
}