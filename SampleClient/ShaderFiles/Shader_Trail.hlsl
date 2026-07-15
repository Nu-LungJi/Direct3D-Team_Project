#include "../../Engine/ShaderFiles/ShaderDefines.hlsl"


cbuffer CB_SCROLL : register(b0)
{
    float g_fScrollOffset;
    float3 _pad;
};



struct VS_IN
{
    float3 vPosition : POSITION;
    float2 vUV : TEXCOORD0;
    float4 vColor : COLOR0; // a에 나이 기반 페이드(fLifeRatio)가 실려 있음
    float4 vEmissive : COLOR1;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vUV : TEXCOORD0;
    float4 vColor : COLOR0;
    float4 vEmissive : COLOR1;
};

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;

    Out.vPosition = mul(float4(In.vPosition, 1.f), g_matViewProj);
    Out.vUV = In.vUV;
    Out.vColor = In.vColor;
    Out.vEmissive = In.vEmissive;
    return Out;
}

Texture2D g_TrailTexture : register(t0);
//SamplerState g_Sampler : register(s0);
Texture2D g_NoiseTexture : register(t1);

float4 PSMain(VS_OUT In) : SV_TARGET
{
    float2 vTrailUV = float2(In.vUV.x * 2.f, In.vUV.y);
    float4 vTexColor = g_TrailTexture.Sample(LinearWrap, vTrailUV);
    if (all(vTexColor.rgb < 0.1f))
        discard;

      
    // 폭 방향(U, 0~1) 기준으로 중심은 밝고 가장자리는 부드럽게 사라지는 글로우 코어.
    // 텍스처 자체가 이미 그라디언트라면 이 보정은 살짝만 줘도 되고,
    // 텍스처가 단순한 흰 띠라면 이 계산이 대부분의 느낌을 만들어준다.
    float fDistFromCenter = abs(In.vUV.y - 0.5f) * 2.f; // 0(중심)~1(가장자리)

// 가장자리 쪽에서만 완만하게 줄어들도록 시작점을 뒤로 미룸
    float fEdgeFade = 1.f - smoothstep(0.5f, 1.f, fDistFromCenter);

// 코어 글로우(밝기용)는 기존처럼 중심에서 밝게
    float fCoreGlow = 1.f - smoothstep(0.f, 1.f, fDistFromCenter);

    float4 vFinalColor = vTexColor * In.vColor;
    vFinalColor.rgb *= (0.5f + fCoreGlow * 1.5f);
    vFinalColor.a *= fEdgeFade; // 알파는 더 넓은 구간까지 유지되다 가장자리 근처에서만 페이드


    return float4(vFinalColor.xyz + In.vEmissive.xyz * In.vEmissive.w,In.vColor.a);
    
}
//Texture2D g_TrailTexture : register(t0);
//Texture2D g_NoiseTexture : register(t1);


//
//float4 PSMain(VS_OUT In) : SV_TARGET
//{
//    float4 vTexColor = g_TrailTexture.Sample(LinearWrap, In.vUV);
//    if (all(vTexColor.rgb < 0.1f))
//        discard;
//
//    // 노이즈 UV: 길이 방향(U)으로 스크롤 오프셋을 더해서 흐르는 느낌 부여
//    float2 vNoiseUV = float2(In.vUV.x * 2.f + g_fScrollOffset, In.vUV.y);
//    float fNoise = g_NoiseTexture.Sample(LinearWrap, vNoiseUV).r;
//
//    // In.vColor.a = CPU에서 넘어온 fLifeRatio (1=방금생김 ~ 0=수명다함)
//    float fAgeProgress = 1.f - In.vColor.a; // 0(방금생김)~1(수명다함)
//
//    // 디졸브: 노이즈보다 나이가 더 많이 진행되면 그 픽셀부터 사라짐
//    float fDissolveMask = fNoise - fAgeProgress;
//    float fSoftness = 0.15f;
//    float fDissolveAlpha = smoothstep(0.f, fSoftness, fDissolveMask);
//
//    float fDistFromCenter = abs(In.vUV.y - 0.5f) * 2.f;
//    float fEdgeFade = 1.f - smoothstep(0.5f, 1.f, fDistFromCenter);
//    float fCoreGlow = 1.f - smoothstep(0.f, 1.f, fDistFromCenter);
//
//    float4 vFinalColor = vTexColor * In.vColor;
//    vFinalColor.rgb *= (0.5f + fCoreGlow * 1.5f);
//    vFinalColor.a *= fEdgeFade * fDissolveAlpha;
//
//    if (vFinalColor.a <= 0.01f)
//        discard;
//
//    return float4(vFinalColor.xyz + In.vEmissive.xyz * In.vEmissive.w, vFinalColor.a);
//}
