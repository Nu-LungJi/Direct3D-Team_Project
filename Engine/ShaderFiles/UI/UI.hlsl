#include "../ShaderDefines.hlsl"

/*
cbuffer CB_PER_UI : register(b7)
{
    float2 g_ui_texCoord;
    float2 g_ui_uvSize;
    float4 g_ui_color;
    uint g_ui_texIndex;

    uint g_ui_frameIndex;
    uint g_ui_flag;
    float2 _g_ui_pad;
};
*/

Texture2DArray gItemTexture16_16Array : register(t6);
Texture2DArray gTexture256_256Array : register(t12);
Texture2DArray gTexture300_300Array : register(t17);
SamplerState samp : register(s0);
Texture2D gPlayerInvenUITex : register(t3);

struct VS_IN
{
    float3 posL : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_OUT
{
    float4 posH : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

struct PS_OUT
{
    float4 target0 : SV_Target;
};

VS_OUT VSMain(VS_IN vin)
{
    VS_OUT vout;
    vout.posH = mul(float4(vin.posL, 1.f), g_matWVP);
    vout.texCoord = vin.uv;
   // vout.texCoord = g_ui_texCoord + vin.uv * g_ui_uvSize;
    return vout;
}

float2 NineSliceUV(float2 uv)
{
    if (g_ui_borderPx.x <= 0.f || g_ui_borderPx.y <= 0.f)
        return g_ui_texCoord + uv * g_ui_uvSize;
    
    float2 px = uv * g_ui_rectSizePx;
    float2 result;

    // X
    if (px.x < g_ui_borderPx.x)
        result.x = (px.x / g_ui_borderPx.x) * g_ui_borderUV.x;
    else if (px.x > g_ui_rectSizePx.x - g_ui_borderPx.x)
        result.x = g_ui_uvSize.x - g_ui_borderUV.x
            + ((px.x - (g_ui_rectSizePx.x - g_ui_borderPx.x)) / g_ui_borderPx.x) * g_ui_borderUV.x;
    else
        result.x = g_ui_borderUV.x
            + ((px.x - g_ui_borderPx.x) / (g_ui_rectSizePx.x - 2.f * g_ui_borderPx.x))
            * (g_ui_uvSize.x - 2.f * g_ui_borderUV.x);

    // Y
    if (px.y < g_ui_borderPx.y)
        result.y = (px.y / g_ui_borderPx.y) * g_ui_borderUV.y;
    else if (px.y > g_ui_rectSizePx.y - g_ui_borderPx.y)
        result.y = g_ui_uvSize.y - g_ui_borderUV.y
            + ((px.y - (g_ui_rectSizePx.y - g_ui_borderPx.y)) / g_ui_borderPx.y) * g_ui_borderUV.y;
    else
        result.y = g_ui_borderUV.y
            + ((px.y - g_ui_borderPx.y) / (g_ui_rectSizePx.y - 2.f * g_ui_borderPx.y))
            * (g_ui_uvSize.y - 2.f * g_ui_borderUV.y);

    return g_ui_texCoord + result;
}

// Pixel Shader
PS_OUT PSMain(VS_OUT pin)
{
    float2 tc = NineSliceUV(pin.texCoord);

    uint groupId = GetTexArrayGroup(g_ui_texIndex);
    uint sliceIndex = GetTexSliceIndex(g_ui_texIndex);
    float4 albedo;
    
    if (groupId == 6)
    {
        albedo = gItemTexture16_16Array.Sample(samp, float3(tc, sliceIndex));
    }
    else if (groupId == 12)
    {
        albedo = gTexture256_256Array.Sample(samp, float3(tc, sliceIndex));
    }
    else if (groupId == 17)
    {
        albedo = gTexture300_300Array.Sample(samp, float3(tc, sliceIndex));
    }
    else if (groupId == 200)
    {
        albedo = gPlayerInvenUITex.Sample(samp, float2(tc));
    }
    
        clip(albedo.a - 0.5f);
    
    
    PS_OUT pout;
    pout.target0 = albedo * g_ui_color;
    return pout;
}