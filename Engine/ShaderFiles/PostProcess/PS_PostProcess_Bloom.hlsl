#include "../ShaderDefines.hlsl"

Texture2D BrightPassTexture : register(t0);     // PostProcess 이전 텍스쳐
Texture2D BlurPassTexture   : register(t1);     // BrightPass 이후 텍스쳐

const static float  BrightThreshold = 1.2f;
const static float  Weights[5]      = { 0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f };
const static float2 TexelSize       = { 1.0f / 1280.0f, 1.0f / 720.0f };

float4 PSMain_BrightPass(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
    float4 BrightDiffuse = BrightPassTexture.Sample(LinearWrap, TexCoord);
    
    float luminance = dot(BrightDiffuse.rgb, float3(0.2126f, 0.7152f, 0.0722f));
    
    [branch]
    if (luminance < BrightThreshold)    return float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    return BrightDiffuse;
}

float4 PSMain_GaussianBlur_Vertical(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
    float4 CenterPixel = BlurPassTexture.Sample(LinearWrap, TexCoord) * Weights[0];
    
    for (int i = 1; i < 5; ++i)
    {
        CenterPixel += BlurPassTexture.Sample(LinearWrap, TexCoord + float2(TexelSize.x, 0.0f) * i) * Weights[i];
        CenterPixel += BlurPassTexture.Sample(LinearWrap, TexCoord - float2(TexelSize.x, 0.0f) * i) * Weights[i];
    }
    
    return CenterPixel;
}

float4 PSMain(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
    float4 OriginalColor = BrightPassTexture.Sample(LinearWrap, TexCoord);
    float4 VerticalBlurColor = BlurPassTexture.Sample(LinearWrap, TexCoord) * Weights[0];
    
    for (int i = 1; i < 5; ++i)
    {
        VerticalBlurColor += BlurPassTexture.Sample(LinearWrap, TexCoord + float2(0.0f, TexelSize.y) * i) * Weights[i];
        VerticalBlurColor += BlurPassTexture.Sample(LinearWrap, TexCoord - float2(0.0f, TexelSize.y) * i) * Weights[i];
    }
    float3 FinalColor = OriginalColor.rgb + (VerticalBlurColor.rgb * BloomIntensity);
    
    return float4(FinalColor, 1.0f);
}
