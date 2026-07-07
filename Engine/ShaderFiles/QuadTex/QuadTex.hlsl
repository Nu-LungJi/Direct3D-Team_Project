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
struct PS_OUT
{
    vector vDiffuse : SV_TARGET0;
    vector vNormal : SV_TARGET1;
    vector vSMRO : SV_TARGET2;
    vector vEmissive : SV_TARGET3;
};

// Pixel Shader
PS_OUT PSMain(PS_IN input)
{
    PS_OUT OUT;
    float4 TexColor = tex.Sample(SamplerWrap, input.uv);
    if (TexColor.a == 0.01f)  discard;
    
    OUT.vDiffuse = TexColor;
    OUT.vNormal  = float4(0.f, 0.f, 0.f, 0.f);
    OUT.vSMRO = float4(0.f, 0.f, 0.f, 0.f);
    OUT.vEmissive = float4(0.f, 0.f, 0.f, 0.f);

    return OUT;
}
PS_OUT PSMain_NonAlpha(PS_IN input)
{
    PS_OUT OUT;
    float4 TexColor = tex.Sample(SamplerWrap, input.uv);
    
    OUT.vDiffuse = TexColor;
    OUT.vNormal = float4(0.f, 0.f, 0.f, 1.f);
    OUT.vSMRO = float4(0.f, 0.f, 0.f, 1.f);
    OUT.vEmissive = float4(0.f, 0.f, 0.f, 1.f);

    return OUT;
}