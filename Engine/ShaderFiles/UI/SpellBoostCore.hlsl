#include "../ShaderDefines.hlsl"

Texture2D tex : register(t0);

struct PS_IN
{
	float4 posH : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 PSMain(PS_IN input) : SV_Target
{
	float2 atlasUV = g_ui_texCoord + input.uv * g_ui_uvSize;
	float4 source = tex.Sample(LinearWrap, atlasUV);

	// Keep the center crisp, then gradually erase only the outer halo.
	float radialDistance = length(input.uv - float2(0.5f, 0.5f)) * 2.f;
	float radialFade = 1.f - smoothstep(0.30f, 0.94f, radialDistance);
	float finalAlpha = source.a * radialFade * g_ui_color.a;

	if (finalAlpha < 0.003f)
		discard;

	return float4(g_ui_color.rgb, finalAlpha);
}
