#include "../ShaderDefines.hlsl"

Texture2D g_BaseTex     : register(t0); //
Texture2D g_DistortTex  : register(t1); // 
Texture2D g_WispyTex    : register(t2); // 
Texture2D g_WavyNormal  : register(t3); // 
Texture2D g_RippleTex   : register(t4);
Texture2D g_SkillIconTex: register(t5);

// 스킬 쿨타임
cbuffer CB_SPELLMETER : register(b10)
{
	float g_Amount;
	float g_DistSpeed;
	float g_DistStrength;
	float g_Time;

	float4 g_FillColor;
	float4 g_EmptyColor;
	float4 g_RippleColor;
	float4 g_WispyColor;
};

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
float4 baseColor = g_BaseTex.Sample(LinearWrap, input.uv);

    if (baseColor.a < 0.01f)
    {
        discard;
    }

    // =========================================================
    // 1. UV 왜곡 및 마법 연기 샘플링
    // =========================================================
    float2 normalUV = input.uv + float2(-g_Time * g_DistSpeed, -g_Time * (g_DistSpeed * 0.7f));
    float2 normalData = g_WavyNormal.Sample(LinearWrap, normalUV).rg;

    float2 refractionVector = (normalData * 2.0f) - 1.0f;
    float2 distortedUV = input.uv + (refractionVector * g_DistStrength);

    float2 wispyUV = distortedUV + float2(-g_Time * 0.2f, g_Time * 0.4f);
    float wispyGlow = g_WispyTex.Sample(LinearWrap, wispyUV).r;

    // =========================================================
    // 2. 스킬 아이콘 
    // =========================================================
    float iconScale = 1.f;
    float2 iconUV = (input.uv - 0.5f) * iconScale + 0.5f;

    float4 iconColor = g_SkillIconTex.Sample(LinearWrap, iconUV);

    if (iconUV.x < 0.0f || iconUV.x > 1.0f || iconUV.y < 0.0f || iconUV.y > 1.0f)
    {
        iconColor.a = 0.0f; 
    }

    float gray = dot(iconColor.rgb, float3(0.299f, 0.587f, 0.114f));
    float3 grayIconColor = float3(gray, gray, gray) * 0.4f;
    float3 colorIconColor = iconColor.rgb;

    float fillGradient = 1.0f - input.uv.y;
    float4 finalColor = float4(0, 0, 0, 1);
    float rippleThickness = 0.3f;

    if (g_Amount >= 0.99f)
    {
        float3 bg = (g_FillColor.rgb * baseColor.rgb) + (g_WispyColor.rgb * wispyGlow * 1.5f);
        finalColor.rgb = lerp(bg, colorIconColor, iconColor.a);
    }
    else if (fillGradient > g_Amount + rippleThickness)
    {
        float3 bg = g_EmptyColor.rgb * baseColor.rgb;
        finalColor.rgb = lerp(bg, grayIconColor, iconColor.a);
    }
    else if (fillGradient < g_Amount - rippleThickness)
    {
        float3 bg = (g_FillColor.rgb * baseColor.rgb) + (g_WispyColor.rgb * wispyGlow * 1.5f);
        finalColor.rgb = lerp(bg, colorIconColor, iconColor.a);
    }
    else
    {
        float localV = (fillGradient - (g_Amount - rippleThickness)) / (2.0f * rippleThickness);
        float3 fillBg = (g_FillColor.rgb * baseColor.rgb) + (g_WispyColor.rgb * wispyGlow * 1.5f);
        float3 emptyBg = g_EmptyColor.rgb * baseColor.rgb;
        float3 blendedBg = lerp(fillBg, emptyBg, localV);

        float3 blendedIcon = lerp(colorIconColor, grayIconColor, localV);


        float3 bgWithIcon = lerp(blendedBg, blendedIcon, iconColor.a);

        float localU = input.uv.x - (g_Time * 1.0f);
        float4 rippleTexData = g_RippleTex.Sample(LinearWrap, float2(localU, localV));
        float rippleAlpha = rippleTexData.r;

        finalColor.rgb = bgWithIcon + (g_RippleColor.rgb * rippleAlpha * 2.0f);
    }

    return finalColor;
}
