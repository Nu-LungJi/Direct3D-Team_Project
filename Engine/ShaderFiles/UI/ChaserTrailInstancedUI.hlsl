#include "../ShaderDefines.hlsl"

Texture2D g_ChaserTrailTexture : register(t0);

struct VS_OUT
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
	float4 Color : TEXCOORD1;
};

float4 PSMain(VS_OUT input) : SV_Target
{
	const float4 textureColor = g_ChaserTrailTexture.Sample(
		LinearWrap,
		input.TexCoord);
	const float alpha = saturate(textureColor.a * input.Color.a);
	clip(alpha - 0.005f);

	// Alpha blending prevents overlapping smoke particles from exceeding
	// the configured tint brightness. Texture RGB keeps the smoke detail.
	const float3 color = saturate(textureColor.rgb) * input.Color.rgb;
	return float4(color, alpha);
}
