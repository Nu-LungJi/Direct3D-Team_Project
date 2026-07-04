#include "../ShaderHeader/SH_SamplerState.hlsli"
#include "../ShaderDefines.hlsl"

Texture2D g_DiffuseTexture  : register(t0);
Texture2D g_NormalTexture   : register(t1);
Texture2D g_ShadowMap       : register(t4);

struct PS_IN
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Normal   : NORMAL;
};

struct PS_OUT
{
    float4 Diffuse : SV_TARGET;
};

PS_OUT PSMain(PS_IN IN)
{
    PS_OUT OUT;
    
    OUT.Diffuse = g_DiffuseTexture.Sample(SamplerWrap, IN.TexCoord);
    
    return OUT;
}