#include "../ShaderDefines.hlsl"

Texture2D g_RippleTexture : register(t0);

struct PS_IN
{
	float4 posH : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 PSMain(PS_IN input) : SV_Target
{
	const float3 source = g_RippleTexture.Sample(LinearClamp, input.uv).rgb;
	const float luminance = dot(source, float3(0.299f, 0.587f, 0.114f));
	const float glowMask = pow(saturate(luminance), 0.82f);
	const float innerLine = smoothstep(0.56f, 0.92f, luminance);

	if (glowMask < 0.004f)
		discard;

	const float3 glowColor = g_ui_color.rgb *
		(lerp(0.52f, 1.08f, glowMask) + innerLine * 0.72f);
	const float alpha = saturate(glowMask * 0.88f + innerLine * 0.22f) *
		g_ui_color.a;
	return float4(glowColor, alpha);
}
