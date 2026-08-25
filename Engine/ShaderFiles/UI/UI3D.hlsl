#include "../ShaderDefines.hlsl"

Texture2D tex : register(t0);
//Texture2D maskTex  : register(t1);
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
	output.uv = vin.uv;

	return output;
}

// Pixel Shader
float4 PSMain(PS_IN input) : SV_Target
{
	// The RTT is often viewed at an oblique angle.  Average four samples over
	// the pixel footprint to suppress the stair-step shimmer that a single
	// bilinear lookup leaves on thin UI lines and text.
	const float2 uvDx = ddx(input.uv);
	const float2 uvDy = ddy(input.uv);
	const float2 sampleDx = uvDx * 0.25f;
	const float2 sampleDy = uvDy * 0.25f;
	float4 texColor =
		tex.Sample(LinearClamp, input.uv - sampleDx - sampleDy) +
		tex.Sample(LinearClamp, input.uv + sampleDx - sampleDy) +
		tex.Sample(LinearClamp, input.uv - sampleDx + sampleDy) +
		tex.Sample(LinearClamp, input.uv + sampleDx + sampleDy);
	texColor *= 0.25f;

	// The RTT is presented as a physical rectangular panel.  Transparent
	// pixels therefore reveal a dark panel surface instead of making the
	// entire world quad disappear.  UI colors themselves remain untouched.
	const float3 panelColor = float3(0.008f, 0.006f, 0.015f);
	float sourceAlpha = saturate(texColor.a);
	float3 resultColor = texColor.rgb + panelColor * (1.f - sourceAlpha);
	float panelAlpha = max(sourceAlpha, 0.92f) * g_ui_color.a;

	// Analytic anti-aliasing for the physical quad silhouette.  fwidth keeps
	// the transition approximately one screen pixel wide at any distance or
	// viewing angle, while leaving the panel interior untouched.
	const float2 distanceToEdge2D = min(input.uv, 1.f - input.uv);
	const float distanceToEdge = min(distanceToEdge2D.x, distanceToEdge2D.y);
	const float edgeFilterWidth = max(fwidth(input.uv.x), fwidth(input.uv.y)) * 1.25f;
	const float edgeCoverage = smoothstep(0.f, max(edgeFilterWidth, 0.00001f), distanceToEdge);
	panelAlpha *= edgeCoverage;

	return float4(resultColor, panelAlpha);
}
