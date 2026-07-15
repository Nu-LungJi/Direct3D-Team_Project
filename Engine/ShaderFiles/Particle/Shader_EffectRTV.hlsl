#include "./../ShaderDefines.hlsl"

struct VS_IN
{
    float3 pos : POSITION; // (-1~1)
    float2 uv : TEXCOORD;
};

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

Texture2D g_DiffuseTexture : register(t0); // Effect (파티클만, 알파 있음)
Texture2D g_BackgroundTexture : register(t7);

VS_OUT VSMain(VS_IN vin)
{
    VS_OUT o;
    o.pos = float4(vin.pos.xy, 1.0f, 1.0f);
    o.uv = vin.uv;
    return o;
}

float4 PSMain(VS_OUT pin) : SV_Target
{

    vector vDiffuse = g_DiffuseTexture.Sample(LinearWrap, pin.uv); // 파티클(Effect)
    vector vBackground = g_BackgroundTexture.Sample(LinearWrap, pin.uv); // 배경(PBR)

    // ---- 파티클(vDiffuse)의 알파값으로 배경 위에 합성 ----
    float3 finalColor = lerp(vBackground.rgb, vDiffuse.rgb, vDiffuse.a);

    return float4(finalColor, 1.f);
}

