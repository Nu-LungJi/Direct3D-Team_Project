#include "../ShaderHeader/SH_SamplerState.hlsli"
#include "../ShaderDefines.hlsl"

Texture2D g_DiffuseTexture : register(t0);
Texture2D g_NormalTexture : register(t1);
Texture2D g_SMROTexture : register(t2);
Texture2D g_EmissiveTexture : register(t3);

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

// Pixel Shader : 불투명(NONBLEND) 오브젝트 그릴 때는 사용X(Normal, SMRO, Emissive에서 안 그려져서 정상적으로 렌더X)
float4 PSMain(PS_IN input)  : SV_TARGET0
{
    float4 TexColor = g_DiffuseTexture.Sample(SamplerWrap, input.uv);
    if (TexColor.a == 0.01f)  discard;
    
    return TexColor;
}
float4 PSMain_NonAlpha(PS_IN input) : SV_TARGET0
{
    return g_DiffuseTexture.Sample(SamplerWrap, input.uv);;
}