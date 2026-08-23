#include "../ShaderDefines.hlsl"

Texture2D tex : register(t0);
//SamplerState samp : register(s0);

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

    output.uv = lerp(vin.uv, 1.f - vin.uv, g_ui_uvFlip);

    return output;
}

float4 PSMain(PS_IN input) : SV_Target
{
	float2 uv = g_ui_texCoord + input.uv * g_ui_uvSize;
	float4 texColor = tex.Sample(LinearWrap, uv);

    // 1. 아주 미세한 찌꺼기만 버림 (부드러운 외곽선 유지)
	if (max(texColor.r, max(texColor.g, texColor.b)) < 0.01f)
	{
		discard;
	}

    // 2. 흑백 밝기 추출
	float luminance = dot(texColor.rgb, float3(0.299f, 0.587f, 0.114f));

    // 3. 밝기 부스팅 (어두운 부분을 밝게 끌어올림)
    // 0.4f ~ 0.5f 사이를 추천합니다. 숫자가 작아질수록 어두운 곳이 더 밝아집니다.
	float boostedLuminance = pow(luminance, 0.4f);

    // 어두운 부분에 깔릴 배경색 (짙은 붉은/마젠타 톤)
	float3 baseSmokeColor = float3(0.5f, 0.1f, 0.3f);
    
    // 밝은 중심부에 맺힐 색 (g_ui_color 활용 또는 밝은 핑크/흰색)
	float3 coreSmokeColor = g_ui_color.rgb; // 또는 float3(1.0f, 0.8f, 0.9f)
    
    // 밝기에 따라 두 색을 자연스럽게 섞음
	texColor.rgb = lerp(baseSmokeColor, coreSmokeColor, boostedLuminance);
	
    // 4. 알파값 처리 (boostedLuminance를 알파로 쓰면 연기가 더 빵빵해집니다)
	float finalAlpha = boostedLuminance * g_ui_color.a;

	return float4(texColor.rgb, finalAlpha);
}
