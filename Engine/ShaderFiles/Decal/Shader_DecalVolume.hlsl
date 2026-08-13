#include "../ShaderDefines.hlsl"

Texture2D g_DepthTexture : register(t0);
Texture2D g_NormalTexture : register(t1);
Texture2D g_DecalMaskTexture : register(t2);

cbuffer CB_DECAL_VOLUME : register(b11)
{
    matrix g_matInvDecalWorld;
    float4 g_vDecalAlbedo;
    float4 g_vDecalEmissiveIntensity;
    float4 g_vDecalParams;
};

struct VS_IN
{
    float3 position : POSITION;
};

struct VS_OUT
{
    float4 position : SV_POSITION;
};

struct PS_OUT
{
    float4 diffuse : SV_TARGET0;
    float4 emissive : SV_TARGET1;
};

VS_OUT VSMain(VS_IN input)
{
    VS_OUT output;
    output.position = mul(float4(input.position, 1.f), g_matWVP);
    return output;
}

float3 ReconstructWorldPosition(float2 screenUV, float depth)
{
    float4 ndcPosition = float4(screenUV.x * 2.f - 1.f, 1.f - screenUV.y * 2.f, depth, 1.f);

    float4 worldPosition = mul(ndcPosition, g_matInvViewProj);
    return worldPosition.xyz / max(worldPosition.w, 0.000001f);
}

PS_OUT PSMain(VS_OUT input)
{
    uint width;
    uint height;
    g_DepthTexture.GetDimensions(width, height);

    uint2 pixel = min(uint2(input.position.xy), uint2(max(width, 1u) - 1u, max(height, 1u) - 1u));
    float2 screenUV = (float2(pixel) + 0.5f) / float2(width, height);
    float depth = g_DepthTexture.Load(int3(pixel, 0)).r;

    clip(0.999999f - depth);

	// 픽셀의 월드좌표 -> DecalVolume의 로컬로 변환
    float3 worldPosition = ReconstructWorldPosition(screenUV, depth);
    float3 localPosition = mul(float4(worldPosition, 1.f), g_matInvDecalWorld).xyz;

    clip(0.5f - abs(localPosition));

    float3 surfaceNormal = normalize(g_NormalTexture.Load(int3(pixel, 0)).xyz * 2.f - 1.f);
    float3 projectionAxis = normalize(g_matWorld[1].xyz);

    float normalThreshold = saturate(g_vDecalParams.y);
    float normalFade = smoothstep(normalThreshold, min(normalThreshold + 0.15f, 1.f), abs(dot(surfaceNormal, projectionAxis)));

    float2 decalUV = float2(localPosition.x + 0.5f, 0.5f - localPosition.z);
    float3 maskSample = g_DecalMaskTexture.Sample(LinearClamp, decalUV).rgb;
    float mask = max(maskSample.r, max(maskSample.g, maskSample.b));

    float edgeSoftness = clamp(g_vDecalParams.z, 0.001f, 0.49f);
    float sideDistance = max(abs(localPosition.x), abs(localPosition.z));
    float edgeFade = 1.f - smoothstep(
        0.5f - edgeSoftness,
        0.5f,
        sideDistance);

    float alpha = saturate(mask * g_vDecalParams.x * normalFade * edgeFade);
    clip(alpha - 0.001f);

    PS_OUT output;
    output.diffuse = float4(g_vDecalAlbedo.rgb, alpha);
    output.emissive = float4(g_vDecalEmissiveIntensity.rgb * g_vDecalEmissiveIntensity.a, alpha);
	
    return output;
}
