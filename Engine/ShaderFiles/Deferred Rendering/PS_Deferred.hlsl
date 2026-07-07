#include "../ShaderHeader/SH_SamplerState.hlsli"
#include "../ShaderDefines.hlsl"

#define MAX_LIGHT_COUNT     8

#define LIGHT_DIRECTIONAL   0
#define LIGHT_POINT         1
#define LIGHT_SPOTLIGHT     2

#define MAX_REFLECTION_LOD  4.f

Texture2D g_DiffuseTexture  : register(t0);
Texture2D g_AOTexture       : register(t1);
//Texture2D g_ShadowMap       : register(t4);

struct PS_IN
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Normal   : NORMAL;
};

struct PS_IN_FORWARD
{
    float4 Position  : SV_POSITION;
    float3 WorldPos  : POSITION;
    float4 Normal    : NORMAL;
    float2 TexCoord  : TEXCOORD0;
    float4 ScreenPos : TEXCOORD1;
};

struct PS_OUT
{
    float4 Diffuse : SV_TARGET;
};

PS_OUT PSMain(PS_IN IN)
{
    PS_OUT OUT;
    
    float4 Diffuse = g_DiffuseTexture.Sample(SamplerWrap, IN.TexCoord);
   
    
    float3 finalColor = Diffuse;
    OUT.Diffuse = float4(finalColor, 1.f);
    return OUT;
}
