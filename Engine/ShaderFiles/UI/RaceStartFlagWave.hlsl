#include "../ShaderDefines.hlsl"

Texture2D flagTexture : register(t0);
Texture2D flagMaskTexture : register(t1);
Texture2D scrollingCloudsTexture : register(t2);

struct PS_IN
{
    float4 posH : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 PSMain(PS_IN input) : SV_Target
{
    const float time = g_ui_texCoord.x;
	const bool subtleResultBanner = g_ui_texCoord.y > 0.5f;

    // The flag is fixed at its pole and bends more strongly toward its tip.
    // FlipX is already reflected in input.uv by VS_QuadTexUI, so the same
    // expression works for both FlagR and FlagL.
	const float distanceFromCenter = abs(input.uv.x - 0.5f) * 2.0f;
	const float freeEdgeWeight = subtleResultBanner ?
		smoothstep(0.08f, 0.92f, distanceFromCenter) :
		smoothstep(0.04f, 0.96f, input.uv.x);
    const float2 cloudUV0 = float2(
        input.uv.x * 1.35f + time * 0.11f,
        input.uv.y * 1.60f - time * 0.035f);
    const float2 cloudUV1 = float2(
        input.uv.x * 2.10f - time * 0.055f,
        input.uv.y * 1.15f + time * 0.065f);
    const float cloud0 = scrollingCloudsTexture.Sample(LinearWrap, cloudUV0).r;
    const float cloud1 = scrollingCloudsTexture.Sample(LinearWrap, cloudUV1).r;
    const float cloudWave = cloud0 * 0.68f + cloud1 * 0.32f - 0.5f;
    const float ribbonWave =
        sin(input.uv.x * 11.0f - time * 3.2f) * 0.65f +
        sin(input.uv.x * 19.0f + time * 2.15f) * 0.35f;

    float2 warpedUV = input.uv;
	const float horizontalStrength = subtleResultBanner ? 0.0015f : 0.010f;
	const float cloudVerticalStrength = subtleResultBanner ? 0.0040f : 0.035f;
	const float ribbonVerticalStrength = subtleResultBanner ? 0.0015f : 0.010f;
    warpedUV.x += cloudWave * horizontalStrength * freeEdgeWeight;
    warpedUV.y +=
        (cloudWave * cloudVerticalStrength +
		ribbonWave * ribbonVerticalStrength) * freeEdgeWeight;

    if (any(warpedUV < 0.0f) || any(warpedUV > 1.0f))
        discard;

    float4 flagColor = flagTexture.Sample(LinearClamp, warpedUV);

    // The source mask contains a soft grey glow outside the real flag shape.
    // Taking the minimum of nearby samples erodes that halo by about one
    // texel, then the higher threshold keeps the opaque black source
    // background from leaking around the silhouette.
    uint maskWidth = 0;
    uint maskHeight = 0;
    flagMaskTexture.GetDimensions(maskWidth, maskHeight);
    const float2 maskTexel = 1.0f / float2(
        max(maskWidth, 1u), max(maskHeight, 1u));
	float maskAlpha = flagColor.a;
	if (!subtleResultBanner)
	{
		float maskValue = flagMaskTexture.Sample(LinearClamp, warpedUV).r;
		maskValue = min(maskValue, flagMaskTexture.Sample(
			LinearClamp, warpedUV + float2(maskTexel.x, 0.0f)).r);
		maskValue = min(maskValue, flagMaskTexture.Sample(
			LinearClamp, warpedUV - float2(maskTexel.x, 0.0f)).r);
		maskValue = min(maskValue, flagMaskTexture.Sample(
			LinearClamp, warpedUV + float2(0.0f, maskTexel.y)).r);
		maskValue = min(maskValue, flagMaskTexture.Sample(
			LinearClamp, warpedUV - float2(0.0f, maskTexel.y)).r);
		maskAlpha = smoothstep(0.32f, 0.62f, maskValue);
	}
    clip(maskAlpha - 0.005f);

    // A very small rolling light change makes the cloth deformation readable
    // without washing out the original chequered flag colors.
	const float clothLight = subtleResultBanner ?
		lerp(0.985f, 1.015f, cloud0) :
		lerp(0.90f, 1.08f, cloud0);
    flagColor.rgb *= clothLight;

    const float luminance = dot(flagColor.rgb, float3(0.299f, 0.587f, 0.114f));
    if (max(g_ui_color.r, max(g_ui_color.g, g_ui_color.b)) > 0.0f)
        flagColor.rgb = g_ui_color.rgb * luminance * clothLight;

    return float4(
        flagColor.rgb,
        flagColor.a * maskAlpha * g_ui_color.a);
}
