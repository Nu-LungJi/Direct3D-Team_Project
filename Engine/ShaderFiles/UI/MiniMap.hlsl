#include "../ShaderDefines.hlsl"

Texture2D g_FrameTex : register(t0);   // t0: 프레임 (나침반)
Texture2D g_MinimapTex : register(t1);

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
	float4 frameColor = g_FrameTex.Sample(LinearClamp, input.uv);

	if (frameColor.a < 0.01f)
	{
		discard;
	}

	// 미니맵 지도 UV 계산 (회전 및 스크롤)
	// 원점을 (0.5, 0.5)로 이동
	float2 mapUV = input.uv - float2(0.5f, 0.5f);

	// 확대/축소
	mapUV *= g_mapScale;

	// 회전
	float cosAngle = cos(g_mapRotation);
	float sinAngle = sin(g_mapRotation);
	float2 rotatedUV;
	rotatedUV.x = mapUV.x * cosAngle - mapUV.y * sinAngle;
	rotatedUV.y = mapUV.x * sinAngle + mapUV.y * cosAngle;

	// 원점 복구 및 플레이어 위치 오프셋(스크롤) 더하기
	// (이때 샘플러는 바깥 영역 처리를 위해 LinearClamp 보단 Border/Black 계열이 안전합니다)
	rotatedUV += float2(0.5f, 0.5f) + g_mapOffset;

	// 지도 텍스처 샘플링
	float4 mapColor = g_MinimapTex.Sample(LinearBorder, rotatedUV);

	float brightness = dot(mapColor.rgb, float3(0.299, 0.587, 0.114));
	brightness = pow(brightness, 1.f);

	if (max(g_ui_color.r, max(g_ui_color.g, g_ui_color.b)) > 0.0f)
	{
		mapColor.rgb = g_ui_color.rgb * brightness;
	}

	// 4. 최종 합성
	// 지도를 밑에 깔고, 프레임의 알파를 이용해 나침반 테두리선(RGB)을 그 위에 자연스럽게 얹어줍니다.
	// frameColor의 알파 텍스처 특성에 따라 0.5f나 합성 비율을 조절해 보세요.
	float4 finalColor;
	finalColor.rgb = lerp(mapColor.rgb, frameColor.rgb, frameColor.a * 0.7f);
	finalColor.a = frameColor.a * g_ui_color.a;

	return finalColor;
}
