#include "../ShaderDefines.hlsl"

Texture2D g_MaskTex : register(t0);   // t0: 프레임 (나침반)
Texture2D g_MinimapTex : register(t1);

cbuffer CB_MINIMAP : register(b10)
{
	float2 g_mapOffset;
	float  g_mapRotation;
	float  g_mapScale;
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

float4 PSMain(PS_IN input) : SV_Target
{
	float4 maskColor = g_MaskTex.Sample(LinearClamp, input.uv);

	// [마스킹 컷] 마스크 이미지의 알파(a) 값이 0에 가까우면 원 바깥 영역이므로 즉시 버림
	if (maskColor.a < 0.01f)
	{
		discard;
	}

	// 2. 미니맵 지도 UV 계산 (회전 및 스크롤)
	float2 mapUV = input.uv - float2(0.5f, 0.5f);
	mapUV *= g_mapScale;

	float cosAngle = cos(g_mapRotation);
	float sinAngle = sin(g_mapRotation);
	float2 rotatedUV;
	rotatedUV.x = mapUV.x * cosAngle - mapUV.y * sinAngle;
	rotatedUV.y = mapUV.x * sinAngle + mapUV.y * cosAngle;
	rotatedUV += float2(0.5f, 0.5f) + g_mapOffset;

	// 외곽 처리를 위해 7번 슬롯 Border 샘플러 사용
	float4 mapColor = g_MinimapTex.Sample(LinearBorder, rotatedUV);

	// 지도가 스크롤되어 맵의 텍스처 경계 바깥으로 밀려난 공백 영역(알파 0)도 버림
	if (mapColor.a < 0.01f)
	{
		discard;
	}

	// 3. 기존 명도 및 UI 컬러 연산 유지
	float brightness = dot(mapColor.rgb, float3(0.299, 0.587, 0.114));
	if (max(g_ui_color.r, max(g_ui_color.g, g_ui_color.b)) > 0.0f)
	{
		mapColor.rgb = g_ui_color.rgb * brightness;
	}

	// 4. 최종 출력 (원형 마스크의 알파와 UI 전체 알파 반영)
	// 원 경계면의 부드러운 안티앨리어싱을 위해 maskColor.a를 최종 알파에 곱해줍니다.
	return float4(mapColor.rgb, maskColor.a * g_ui_color.a);
}
