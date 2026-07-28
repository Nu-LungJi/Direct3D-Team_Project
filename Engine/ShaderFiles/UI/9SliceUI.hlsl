#include "../ShaderDefines.hlsl"

Texture2D tex : register(t0);

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

// 1차원 축(X 또는 Y)에 대한 9-Slice UV 계산 로직
float Map9Slice(float uv, float quadSize, float texSize, float margin1, float margin2)
{
	// 현재 픽셀의 Quad 상의 실제 위치
	float pixelPos = uv * quadSize;

	// 1. 첫 번째 영역 (왼쪽 또는 위쪽 고정 마진)
	if (pixelPos < margin1)
	{
		return pixelPos / texSize;
	}
	// 2. 세 번째 영역 (오른쪽 또는 아래쪽 고정 마진)
	else if (pixelPos > quadSize - margin2)
	{
		float fromEnd = quadSize - pixelPos;
		return 1.0f - (fromEnd / texSize);
	}
	// 3. 두 번째 영역 (중앙 늘어나는 영역)
	else
	{
		float centerQuadSize = quadSize - margin1 - margin2;
		float centerTexSize = texSize - margin1 - margin2;

		// 중앙 영역 내에서의 비율 (0.0 ~ 1.0)
		float ratio = (pixelPos - margin1) / centerQuadSize;

		// 텍스처 상의 중앙 영역 좌표로 변환
		return (margin1 + ratio * centerTexSize) / texSize;
	}
}

PS_IN VSMain(VS_IN vin)
{
    PS_IN output;

    output.posH = mul(float4(vin.posL, 1.f), g_matWVP);
    output.uv = vin.uv;

    return output;
}

float4 PSMain(PS_IN input) : SV_Target
{
	// 입력된 0~1 UV를 9-Slice 규칙에 맞게 변환
	float2 uv9;
	uv9.x = Map9Slice(input.uv.x, g_ui_quadSize.x, g_ui_texSize.x, g_ui_margins.x, g_ui_margins.z); // Left(x), Right(z)
	uv9.y = Map9Slice(input.uv.y, g_ui_quadSize.y, g_ui_texSize.y, g_ui_margins.y, g_ui_margins.w); // Top(y), Bottom(w)

	if (g_ui_uvSize.y == 1.f)
	{
		if (input.uv.x > g_ui_uvSize.x)
		{
			discard;
		}
	}
	else
	{
		if (input.uv.x < 1.f - g_ui_uvSize.x)
		{
			discard;
		}
	}



	// 변환된 UV 좌표로 텍스처 샘플링
	float4 texColor = tex.Sample(LinearWrap, uv9);

	if (texColor.a < 0.1f)
	{
		discard;
	}

	float brightness = dot(texColor.rgb, float3(0.299, 0.587, 0.114));
	brightness = pow(brightness, 1.f);

	if (max(g_ui_color.r, max(g_ui_color.g, g_ui_color.b)) > 0.0f)
	{
		texColor.rgb = g_ui_color.rgb * brightness;
	}

	return float4(texColor.rgb, texColor.a * g_ui_color.a);
}
