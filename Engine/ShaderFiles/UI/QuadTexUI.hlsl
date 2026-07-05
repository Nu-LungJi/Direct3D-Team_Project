#include "../ShaderDefines.hlsl"

Texture2D tex : register(t0);
SamplerState samp : register(s0);

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
    float4 texColor = tex.Sample(samp, input.uv);

    //if (max(texColor.r, max(texColor.g, texColor.b)) < 0.001)
    //{
    //    discard;
    //}

    return float4(texColor.rgb, g_ui_color.a);
}