#include "../ShaderDefines.hlsl"

Texture2D tex : register(t0); // 단색 검은색 텍스처 또는 화면을 덮을 기본 텍스처
Texture2D maskTex : register(t1); // 노이즈 텍스처 바인딩 (VFX_T_Noise01_D.png 등)
// SamplerState는 ShaderDefines.hlsl의 LinearClamp를 사용한다고 가정

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
	float4 baseColor = tex.Sample(LinearClamp, input.uv);
    
	if (baseColor.a < 0.01f)
	{
		discard;
	}
    
	float noise1 = maskTex.Sample(LinearWrap, input.uv).r; // 큰 덩어리
	float noise2 = maskTex.Sample(LinearWrap, input.uv * 2.5f).r; // 중간 덩어리
	float noise3 = maskTex.Sample(LinearWrap, input.uv * 5.0f).r; // 자잘한 디테일

    // 가중치를 두어 섞어줍니다 (큰 덩어리의 형태를 유지하면서 디테일 추가)
	float combinedNoise = (noise1 * 0.5f) + (noise2 * 0.3f) + (noise3 * 0.2f);


    // ---------------- [2. 외곽 고속 디졸브 마스크] ----------------
	float dist = distance(input.uv, float2(0.5f, 0.5f));
    
	float radialEdge = dist * 1.5f;
    
	float finalNoise = combinedNoise - radialEdge;
    // ----------------------------------------------------------------

	float fadeAmount = g_ui_texCoord.x;
    
    // 수정 2: radialEdge가 커졌으므로, 딜레이 없이 시작하도록 최소값을 -0.8f 정도로 조정합니다.
	float mappedFade = lerp(-0.8f, 1.2f, fadeAmount);

	float dissolveAlpha = saturate((mappedFade - finalNoise) / 0.4f);

    // 기존 밝기/색상 보정 로직
	float brightness = dot(baseColor.rgb, float3(0.299, 0.587, 0.114));
	if (max(g_ui_color.r, max(g_ui_color.g, g_ui_color.b)) > 0.0f)
	{
		baseColor.rgb = g_ui_color.rgb * brightness;
	}
	else
	{
		baseColor.rgb = g_ui_color.rgb;
	}

	return float4(baseColor.rgb, baseColor.a * g_ui_color.a * dissolveAlpha);
}
