#include "../ShaderDefines.hlsl"

Texture2D tex : register(t0); // 빨강/파랑 회전 링

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

float2 RotateUV(float2 uv, float angle)
{
	uv -= 0.5f;
	float s, c;
	sincos(angle, s, c);
	return float2(uv.x * c - uv.y * s, uv.x * s + uv.y * c) + 0.5f;
}

float4 PSMain(PS_IN input) : SV_Target
{
	float time = g_ui_texCoord.y;

	float speedMultiplier = 4.0f;
	float clipValue = 0.3333f;

	float outerSpeed = time * speedMultiplier;
	float innerSpeed = -time * (speedMultiplier * 0.8f); // 마침표 삭제됨

	float2 outerUV = RotateUV(input.uv, outerSpeed);
	float2 innerUV = RotateUV(input.uv, innerSpeed);

	float outerRing = tex.Sample(LinearClamp, outerUV).r;
	float innerRing = tex.Sample(LinearClamp, innerUV).b;

	float finalAlpha = saturate(outerRing + innerRing);

	if (finalAlpha < clipValue)
	{
		discard;
	}

	float3 finalColor = g_ui_color.rgb;
    
	return float4(finalColor, finalAlpha * g_ui_color.a);
}
