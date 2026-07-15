#include "../ShaderDefines.hlsl"

Texture2D g_BaseTex     : register(t0); //
Texture2D g_DistortTex  : register(t1); // 
Texture2D g_WispyTex    : register(t2); // 
Texture2D g_WavyNormal  : register(t3); // 

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
        // UV 왜곡
        // =========================================================
        float2 normalUV = input.uv + float2(-g_Time * g_DistSpeed, -g_Time * (g_DistSpeed * 0.7f));
        float2 normalData = g_WavyNormal.Sample(LinearWrap, normalUV).rg;

        // 원본 UV 밀어내기
        float2 refractionVector = (normalData * 2.0f) - 1.0f;
        float2 pushDirection = float2(-0.05f, 0.05f);
        refractionVector += pushDirection;
        float2 distortedUV = input.uv + (refractionVector * g_DistStrength);

        // =========================================================
        // 텍스처 샘플
        // =========================================================
        float2 wispyUV = distortedUV + float2(-g_Time * 0.8f, -g_Time * 0.2f);
        float wispyGlow = g_WispyTex.Sample(LinearWrap, wispyUV).r;

        // =========================================================
        // 게이지 채우기
        // =========================================================
        float fillGradient = 1.0f - distortedUV.y;

        float4 finalColor = float4(0, 0, 0, 1);
        float rippleThickness = 0.03f; // 파동(경계선) 두께 설정

        if (g_Amount >= 0.99f) // 다 채웠을때 경계선 안나오게
        {
            float3 glowEffect = g_FillColor.rgb * wispyGlow * 1.5f;
            finalColor.rgb = (g_FillColor.rgb * baseColor.rgb) + glowEffect;
        }

        if (fillGradient > g_Amount + rippleThickness)
        {
            // 쿨타임이 차지 않은 영역
            finalColor.rgb = g_EmptyColor.rgb * baseColor.rgb;
        }
        else if (fillGradient < g_Amount - rippleThickness)
        {
            // 쿨타임이 채워진 영역
            float3 glowEffect = g_FillColor.rgb * wispyGlow * 1.5f;
            finalColor.rgb = (g_FillColor.rgb * baseColor.rgb) + glowEffect;
        }
        else
        {
            // 차오르는 경계선 영역
            finalColor.rgb = g_RippleColor.rgb * 1.5f;
        }

        // 투명도
        finalColor.a = baseColor.a * g_ui_color.a;

        return finalColor;
}
