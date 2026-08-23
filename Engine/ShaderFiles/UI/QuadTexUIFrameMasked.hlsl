#include "../ShaderDefines.hlsl"

Texture2D sourceTexture : register(t0);
Texture2D frameMaskTexture : register(t1);

struct PS_IN
{
    float4 posH : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 PSMain(PS_IN input) : SV_Target
{
    float4 sourceColor = sourceTexture.Sample(LinearClamp, input.uv);
    if (sourceColor.a < 0.01f)
        discard;

    // margins.xy: source-to-mask UV scale
    // margins.zw: source top-left offset in mask UV space
    float2 maskUV = input.uv * g_ui_margins.xy + g_ui_margins.zw;
    if (any(maskUV < 0.0f) || any(maskUV > 1.0f))
        discard;

    // CoreBorder is a hollow frame. Find the outer frame on both sides of
    // this row and fill the enclosed interval instead of multiplying by the
    // frame alpha directly. This keeps the card interior visible while
    // clipping pixels that spill beyond the real frame silhouette.
    float leftFrame = 0.0f;
    float rightFrame = 0.0f;

    // The ornamental side border is only a few texels wide and bends inward.
    // A coarse 32-point scan can skip it for an entire row, producing the
    // horizontal holes seen through the card. 128 samples keep the hollow
    // frame filled continuously while retaining its outer silhouette.
    [loop]
    for (int sampleIndex = 0; sampleIndex < 128; ++sampleIndex)
    {
        float sampleX = (sampleIndex + 0.5f) / 128.0f;
        float frameAlpha = frameMaskTexture.SampleLevel(
            LinearClamp, float2(sampleX, maskUV.y), 0.0f).a;
        if (sampleX <= maskUV.x)
            leftFrame = max(leftFrame, frameAlpha);
        if (sampleX >= maskUV.x)
            rightFrame = max(rightFrame, frameAlpha);
    }

    float enclosedMask = smoothstep(0.02f, 0.16f,
        min(leftFrame, rightFrame));
    clip(enclosedMask - 0.01f);

    float sourceLuminance = dot(sourceColor.rgb,
        float3(0.299f, 0.587f, 0.114f));
    if (max(g_ui_color.r, max(g_ui_color.g, g_ui_color.b)) > 0.0f)
        sourceColor.rgb = g_ui_color.rgb * sourceLuminance;

    float textureBrightness = g_ui_quadSize.x > 0.0f ?
        g_ui_quadSize.x : 1.0f;
    sourceColor.rgb *= textureBrightness;
    return float4(sourceColor.rgb,
        sourceColor.a * g_ui_color.a * enclosedMask);
}
