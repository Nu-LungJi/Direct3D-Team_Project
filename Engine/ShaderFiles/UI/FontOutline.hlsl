Texture2D FontTexture : register(t0);
SamplerState FontSampler : register(s0);

struct PS_IN
{
	float4 color : COLOR0;
	float2 uv : TEXCOORD0;
};

float4 PSMain(PS_IN input) : SV_Target
{
	uint textureWidth = 0;
	uint textureHeight = 0;
	FontTexture.GetDimensions(textureWidth, textureHeight);

	const float2 texelSize = rcp(float2(
		max(textureWidth, 1u),
		max(textureHeight, 1u)));
	const float2 outlineStep = texelSize * 1.f;

	const float centerAlpha = FontTexture.Sample(FontSampler, input.uv).a;
	float expandedAlpha = centerAlpha;
	expandedAlpha = max(expandedAlpha, FontTexture.Sample(FontSampler, input.uv + float2(-outlineStep.x, -outlineStep.y)).a);
	expandedAlpha = max(expandedAlpha, FontTexture.Sample(FontSampler, input.uv + float2(              0.f, -outlineStep.y)).a);
	expandedAlpha = max(expandedAlpha, FontTexture.Sample(FontSampler, input.uv + float2( outlineStep.x, -outlineStep.y)).a);
	expandedAlpha = max(expandedAlpha, FontTexture.Sample(FontSampler, input.uv + float2(-outlineStep.x,               0.f)).a);
	expandedAlpha = max(expandedAlpha, FontTexture.Sample(FontSampler, input.uv + float2( outlineStep.x,               0.f)).a);
	expandedAlpha = max(expandedAlpha, FontTexture.Sample(FontSampler, input.uv + float2(-outlineStep.x,  outlineStep.y)).a);
	expandedAlpha = max(expandedAlpha, FontTexture.Sample(FontSampler, input.uv + float2(              0.f,  outlineStep.y)).a);
	expandedAlpha = max(expandedAlpha, FontTexture.Sample(FontSampler, input.uv + float2( outlineStep.x,  outlineStep.y)).a);

	// Preserve the requested color at the glyph center and make only the
	// alpha coverage introduced by the dilation black.
	const float fillRatio = centerAlpha / max(expandedAlpha, 0.00001f);
	const float3 resultColor = input.color.rgb * saturate(fillRatio);
	const float resultAlpha = saturate(expandedAlpha) * input.color.a;

	return float4(resultColor, resultAlpha);
}
