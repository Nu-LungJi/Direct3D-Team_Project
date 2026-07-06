
#include "./ShaderDefines.hlsl"

struct VS_IN
{
    float3 vPosition : POSITION;
    float2 vUV : TEXCOORD0;
    float4 vColor : COLOR0; // a에 나이 기반 페이드(fLifeRatio)가 실려 있음
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vUV : TEXCOORD0;
    float4 vColor : COLOR0;
};

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;

    Out.vPosition = mul(float4(In.vPosition, 1.f), g_matViewProj);
    Out.vUV = In.vUV;
    Out.vColor = In.vColor;

    return Out;
}

Texture2D g_TrailTexture : register(t0);
SamplerState g_Sampler : register(s0);

float4 PSMain(VS_OUT In) : SV_TARGET
{
    float4 vTexColor = g_TrailTexture.Sample(g_Sampler, In.vUV);
    vTexColor.x = 0.4f;
    // 폭 방향(U, 0~1) 기준으로 중심은 밝고 가장자리는 부드럽게 사라지는 글로우 코어.
    // 텍스처 자체가 이미 그라디언트라면 이 보정은 살짝만 줘도 되고,
    // 텍스처가 단순한 흰 띠라면 이 계산이 대부분의 느낌을 만들어준다.
    float fDistFromCenter = abs(In.vUV.x - 0.5f) * 2.f; // 0(중심)~1(가장자리)
    float fCoreGlow = 1.f - smoothstep(0.f, 1.f, fDistFromCenter);

    float4 vFinalColor = vTexColor * In.vColor;
    vFinalColor.rgb *= (0.5f + fCoreGlow * 1.5f); // 중심을 확 밝게 - 가산 블렌딩과 합쳐지면 하얗게 탄 코어처럼 보임
    vFinalColor.a *= fCoreGlow; // 가장자리는 알파도 같이 죽여서 부드럽게 페이드아웃

    if (vFinalColor.a <= 0.01f)
        discard;

    return vFinalColor;
}
