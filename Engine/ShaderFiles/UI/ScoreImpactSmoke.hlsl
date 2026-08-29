#include "../ShaderDefines.hlsl"

Texture2D smokeTexture : register(t0);

struct PS_IN
{
	float4 posH : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 PSMain(PS_IN input) : SV_Target
{
	const float2 uv = g_ui_texCoord + input.uv * g_ui_uvSize;
	const float4 smokeSample = smokeTexture.Sample(LinearWrap, uv);

	// RGB is authored over a light-gray background. The BC3 alpha channel is
	// the actual smoke mask, so using luminance creates a visible square tile.
	const float mask = saturate(smokeSample.a);
	clip(mask - 0.012f);
	const float density = smoothstep(0.025f, 0.68f, mask);
	const float softDetail = pow(mask, 0.68f);
	const float alpha = density * softDetail * g_ui_color.a;
	clip(alpha - 0.006f);

	const float3 smokeColor = g_ui_color.rgb *
		lerp(0.42f, 1.18f, softDetail);
	// BS_ADDITIVE already multiplies source RGB by source alpha.
	return float4(smokeColor, alpha);
}
