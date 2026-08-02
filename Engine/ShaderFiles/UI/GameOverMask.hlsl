#include "../ShaderDefines.hlsl"

Texture2D maskTex : register(t0); // UI_T_GameOverMask
Texture2D glitterTex : register(t1); // UI_T_SharpGlitterMask
Texture2D smokeNoiseTex : register(t2); // UI_T_SmokesNoiseMask
Texture2D distortTex : register(t3); // UI_T_CloudDistortion

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

float2 GetMirrorUV(float2 uv)
{
	return abs(frac(uv * 0.5f) * 2.0f - 1.0f);
}

float4 PSMain(PS_IN input) : SV_Target
{
	float time = g_ui_texCoord.y;

    // 1. 타일링된 기본 UV (밀도 높은 구름 세팅)
	float2 noiseUV = input.uv * float2(2.0f, 2.35f);

    // 2. 왜곡 텍스처 샘플링 (까만 선 방지를 위한 SampleLevel 사용)
	float2 distortScroll = noiseUV + (time * float2(0.01f, -0.02f));
	float2 distortSample = distortTex.SampleLevel(LinearWrap, GetMirrorUV(distortScroll), 0).rg;
	float2 distortUV = (distortSample * 2.0f) - 1.0f;
	float distortStrength = 0.05f;

    // 3. 디테일 연기(Smoke) 샘플링
	float2 smokeScroll = noiseUV + (time * float2(-0.01f, -0.03f)) + (distortUV * distortStrength);
	float3 smokeSample = smokeNoiseTex.SampleLevel(LinearWrap, GetMirrorUV(smokeScroll), 0).rgb;
	float smokeDetail = saturate(smokeSample.r * 0.6f + smokeSample.g * 0.3f + smokeSample.b * 0.1f);

    // 4. 글씨 뒤 붉은 구름 마스크
	float3 maskSample = maskTex.Sample(LinearWrap, input.uv).rgb;
	float baseMask = pow(saturate((maskSample.r + maskSample.g + maskSample.b) * 0.7f), 1.5f);

    // Edge Fade
	float fadeX = smoothstep(0.0f, 0.15f, input.uv.x) * (1.0f - smoothstep(0.85f, 1.0f, input.uv.x));
	float fadeY = smoothstep(0.0f, 0.25f, input.uv.y) * (1.0f - smoothstep(0.75f, 1.0f, input.uv.y));
	float edgeFade = fadeX * fadeY;
	float cloudMask = baseMask * smokeDetail * 2.0f * edgeFade;

    // 색상 적용
	float3 cloudColor = float3(0.35f, 0.02f, 0.06f);
	float3 finalCloud = cloudColor * cloudMask;


	float2 glitterUV = input.uv * 2.f;

	float2 speedR = float2(-0.015f, -0.01f);
	float2 speedG = float2(0.005f, 0.0125f);
	float2 speedB = float2(0.01f, -0.005f);

	float glitterR = glitterTex.Sample(LinearWrap, glitterUV + (time * speedR)).r;
	float glitterG = glitterTex.Sample(LinearWrap, glitterUV + (time * speedG)).g;
	float glitterB = glitterTex.Sample(LinearWrap, glitterUV + (time * speedB)).b;

	float glitterMask = saturate(glitterR + glitterG + glitterB);
	
	float glitterThreshold = 0.3f;
	glitterMask = saturate(glitterMask - glitterThreshold);

    // 반짝이 밝기
	glitterMask = pow(glitterMask, 2.0f) * 3.0f;
    
    // 글리터 사이즈
	glitterMask *= (baseMask * edgeFade * 1.5f);

	float3 glitterColor = float3(1.0f, 0.8f, 0.2f);
	float3 finalGlitter = glitterColor * glitterMask;

    // 최종
	float3 finalRGB = finalCloud + finalGlitter;
	float finalAlpha = saturate(cloudMask + glitterMask);

	return float4(finalRGB, finalAlpha * g_ui_color.a);
}
