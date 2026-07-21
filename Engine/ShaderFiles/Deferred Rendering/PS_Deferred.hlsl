#include "../ShaderDefines.hlsl"

#define MAX_LIGHT_COUNT     8

#define LIGHT_DIRECTIONAL   0
#define LIGHT_POINT         1
#define LIGHT_SPOTLIGHT     2

#define MAX_REFLECTION_LOD  4.f

Texture2D g_BackGroundTexture : register(t0);
Texture2D g_OverDrawTexture   : register(t1);

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

float4 PSMain(PS_IN IN) : SV_TARGET
{
    float4 BackGround = g_BackGroundTexture.Sample(LinearWrap, IN.TexCoord);
    float4 OverTexture = g_OverDrawTexture.Sample(LinearWrap, IN.TexCoord);

	return float4(BackGround.rgb, 1.f);
	
    float3 fogColor = OverTexture.rgb;      // LightAccumulation
    float transmittance = OverTexture.a;    // LightTransmittance
    
    float3 finalRGB = (BackGround.rgb * transmittance) + fogColor;
        
    return float4(finalRGB.rgb, BackGround.a); //lerp(BackGround, OverTexture, OverTexture.a);

}
float4 PSMain_OverDraw(PS_IN IN) : SV_TARGET
{
    return float4(g_BackGroundTexture.Sample(LinearWrap, IN.TexCoord).rgb, 1.f);
}
