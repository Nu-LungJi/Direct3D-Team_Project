#include "../ShaderDefines.hlsl"

Texture2D logoTexture : register(t0);
Texture2D logoBlurTexture : register(t1);
Texture2D logoDetailsTexture : register(t2);
Texture2D ribbonOffsetTexture : register(t3);

struct PS_IN
{
    float4 posH : SV_POSITION;
    float2 uv : TEXCOORD;
};

float LogoRipple(
    float2 uv,
    float2 origin,
    float progress,
    float startProgress,
    float maxRadius,
    float width,
    float ribbonWave)
{
    const float localProgress = saturate(
        (progress - startProgress) / max(1.0f - startProgress, 0.01f));
    const float started = step(startProgress, progress);
    const float life =
        smoothstep(0.0f, 0.08f, localProgress) *
        (1.0f - smoothstep(0.72f, 1.0f, localProgress));
    const float2 delta = (uv - origin) * float2(1.65f, 1.0f);
    const float distanceFromOrigin =
        length(delta) + ribbonWave * 0.11f;
    const float radius = lerp(0.015f, maxRadius, localProgress);
    const float ringDistance = abs(distanceFromOrigin - radius);
    return (1.0f - smoothstep(width * 0.25f, width, ringDistance)) *
        life * started;
}

float4 PSMain(PS_IN input) : SV_Target
{
    const float time = g_ui_texCoord.x;
    const float delay = g_ui_texCoord.y;
    const float duration = max(g_ui_uvSize.x, 0.01f);
    const float progress = saturate((time - delay) / duration);
    const float active = step(delay, time) * step(time, delay + duration);
    const float effectTime = max(time - delay, 0.0f);
    const float effectFade = active *
        (1.0f - smoothstep(0.5f, 1.0f, progress));
    // Keep both effects strong for 1.5 seconds, then fade over the final 1.5 seconds.
    const float rippleProgress = frac(effectTime / 1.35f);

    float4 base = logoTexture.Sample(LinearClamp, input.uv);
    float4 blur = logoBlurTexture.Sample(LinearClamp, input.uv);
    float4 details = logoDetailsTexture.Sample(LinearClamp, input.uv);

    float2 ribbonUV0 = input.uv * float2(1.35f, 1.8f) +
        float2(time * 0.045f, -time * 0.028f);
    float2 ribbonUV1 = input.uv.yx * float2(1.9f, 1.15f) +
        float2(-time * 0.032f, time * 0.021f);
    float4 ribbon0 = ribbonOffsetTexture.Sample(LinearWrap, ribbonUV0);
    float4 ribbon1 = ribbonOffsetTexture.Sample(LinearWrap, ribbonUV1);
    float ribbonNoise = saturate(
        ribbon0.r * 0.55f + ribbon0.a * 0.25f + ribbon1.g * 0.20f);
	// Treat the ribbon normal channels as signed offsets. Each expanding light
	// wave gets a slightly different, continuously moving edge.
	float2 ribbonOffset0 = ribbon0.rg * 2.0f - 1.0f;
	float2 ribbonOffset1 = ribbon1.rg * 2.0f - 1.0f;
	float ribbonWave =
		ribbonOffset0.x * 0.070f +
		ribbonOffset0.y * 0.025f +
		ribbonOffset1.x * 0.035f;

    // A broad ribbon-distorted highlight crosses the logo from left to right
    // over the full three-second playback, underneath the local ripples.
    const float sweepPosition = lerp(-0.18f, 1.18f, progress);
    const float sweepCoordinate = input.uv.x + ribbonWave * 0.13f;
    const float sweepDistance = abs(sweepCoordinate - sweepPosition);
    const float sweepBroad =
        (1.0f - smoothstep(0.035f, 0.16f, sweepDistance)) *
        lerp(0.58f, 1.0f, ribbonNoise);
    const float sweepSharp =
        1.0f - smoothstep(0.006f, 0.038f, sweepDistance);

	// Four staggered origins replace the old single left-to-right sweep. Their
	// rings overlap across the lettering, making light appear to spread from
	// several places instead of travelling in one uniform direction.
	float rippleBroad = 0.0f;
	float rippleSharp = 0.0f;
	rippleBroad += LogoRipple(input.uv, float2(0.20f, 0.43f), rippleProgress,
		0.00f, 0.48f, 0.075f, ribbonWave);
	rippleBroad += LogoRipple(input.uv, float2(0.43f, 0.61f), rippleProgress,
		0.12f, 0.42f, 0.070f, -ribbonWave);
	rippleBroad += LogoRipple(input.uv, float2(0.68f, 0.39f), rippleProgress,
		0.25f, 0.46f, 0.072f, ribbonWave * 0.8f);
	rippleBroad += LogoRipple(input.uv, float2(0.84f, 0.58f), rippleProgress,
		0.38f, 0.38f, 0.065f, -ribbonWave * 0.9f);
	rippleSharp += LogoRipple(input.uv, float2(0.20f, 0.43f), rippleProgress,
		0.00f, 0.48f, 0.024f, ribbonWave);
	rippleSharp += LogoRipple(input.uv, float2(0.43f, 0.61f), rippleProgress,
		0.12f, 0.42f, 0.022f, -ribbonWave);
	rippleSharp += LogoRipple(input.uv, float2(0.68f, 0.39f), rippleProgress,
		0.25f, 0.46f, 0.023f, ribbonWave * 0.8f);
	rippleSharp += LogoRipple(input.uv, float2(0.84f, 0.58f), rippleProgress,
		0.38f, 0.38f, 0.021f, -ribbonWave * 0.9f);
	rippleBroad = saturate(rippleBroad) * lerp(0.52f, 1.0f, ribbonNoise);
	rippleSharp = saturate(rippleSharp);

    const float pulse = effectFade *
        (0.82f + sin(effectTime * 3.2f) * 0.18f);

    // The converted base logo PNG has the clean transparent silhouette.
    // Blur/details still contain opaque canvases, so every auxiliary layer is
    // clipped by the base PNG alpha instead of using its own alpha channel.
    const float detailLuminance = max(details.r, max(details.g, details.b));
    const float logoMask = saturate(base.a);
    const float detailsMask = smoothstep(0.025f, 0.12f, detailLuminance);

    float3 color = base.rgb;
    color += blur.rgb * pulse * logoMask * 0.72f;
    color += details.rgb * detailsMask * pulse * logoMask * 0.48f;
    color += float3(1.0f, 0.88f, 0.48f) *
        sweepBroad * logoMask * effectFade * 0.72f;
    color += float3(1.0f, 0.98f, 0.84f) *
        sweepSharp * logoMask * effectFade * 0.58f;
    color += float3(1.0f, 0.91f, 0.58f) *
        rippleBroad * logoMask * effectFade * 0.90f;
    color += float3(1.0f, 0.98f, 0.86f) *
        rippleSharp * logoMask * effectFade * 0.68f;

    const float outputAlpha = logoMask;
    clip(outputAlpha - 0.004f);

    return float4(color, outputAlpha * g_ui_color.a);
}
