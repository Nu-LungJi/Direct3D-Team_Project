#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"

cbuffer CB_PER_PARTICLE : register(b5)
{
    float g_fTimeDelta;
    uint g_iNumInstances;
    uint g_iFlipbookRows;
    uint g_iFlipbookColumns;
    uint g_iTotalFrames;
    float g_fTime;
    float2 g_fPadding;
};

StructuredBuffer<ParticleData> g_RenderBuffer : register(t4);

// Existing slots are preserved so the current C++ binding code can be reused.
Texture2D AlbedoMap   : register(t0);
Texture2D NormalMap   : register(t1);
Texture2D SMROMap     : register(t2);
Texture2D EmissiveMap : register(t3);
Texture2D NoiseMap    : register(t5);

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal   : NORMAL;
    float3 vTangent  : TANGENT;
    float3 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition    : SV_POSITION;
    float2 vTexcoord    : TEXCOORD0;
    float4 vColor       : COLOR0;
    float3 vNormal      : NORMAL0;
    float3 vTangent     : TANGENT0;
    float3 vBinormal    : BINORMAL0;
    float4 vEmissive    : EMISSIVE0;
    float4 vEndEmissive : EMISSIVE1;
    float3 vWorldPos    : TEXCOORD1;
    float life          : TEXCOORD2;
    float maxLife       : TEXCOORD3;
};

VS_OUT VSMain(VS_IN In, uint instID : SV_InstanceID)
{
    VS_OUT Out = (VS_OUT)0;
    ParticleData p = g_RenderBuffer[instID];

    float2 finalUV = In.vTexcoord;
    float scale = p.alive ? p.size : 0.0f;

    if (g_iTotalFrames > 1 && g_iFlipbookColumns > 0 && g_iFlipbookRows > 0)
    {
        uint frame = min(p.frameIndex, g_iTotalFrames - 1);
        uint col = frame % g_iFlipbookColumns;
        uint row = frame / g_iFlipbookColumns;
        float2 uvSize = float2(
            1.0f / g_iFlipbookColumns,
            1.0f / g_iFlipbookRows);

        finalUV = float2(col, row) * uvSize + In.vTexcoord * uvSize;
    }

    float3 localPos = In.vPosition * scale;
    float3 rotatedLocal = RotateXYZ(localPos, p.rotation);
    float3 worldPos = rotatedLocal + p.position;

    Out.vPosition = mul(float4(worldPos, 1.0f), g_matViewProj);
    Out.vTexcoord = finalUV;
    Out.vWorldPos = worldPos;
    Out.vNormal = normalize(RotateXYZ(In.vNormal, p.rotation));
    Out.vTangent = normalize(RotateXYZ(In.vTangent, p.rotation));
    Out.vBinormal = normalize(RotateXYZ(In.vBinormal, p.rotation));
    Out.vColor = p.alive ? p.color : float4(p.color.rgb, 0.0f);
    Out.vEmissive = p.emissive;
    Out.vEndEmissive = p.endEmissive;
    Out.life = p.life;
    Out.maxLife = p.maxLife;

    return Out;
}

float LightningHash(float value)
{
    return frac(sin(value * 12.9898f) * 43758.5453f);
}

float4 PSMain(VS_OUT In) : SV_TARGET
{
    float maxLife = max(In.maxLife, 0.0001f);
    float age01 = 1.0f - saturate(In.life / maxLife);

    // Fast attack, short hold and hard decay.
    float fadeIn = smoothstep(0.0f, 0.035f, age01);
    float fadeOut = 1.0f - smoothstep(0.55f, 1.0f, age01);
    float lifeEnvelope = fadeIn * fadeOut;

    // Two differently scaled/panned samples reduce visible repetition.
    float2 uv0 = In.vTexcoord * float2(1.5f, 4.0f)
        + float2(0.11f, 0.73f) * g_fTime;
    float2 uv1 = In.vTexcoord * float2(3.7f, 7.0f)
        + float2(-0.19f, 1.21f) * g_fTime;

    float4 noise0 = NoiseMap.Sample(LinearWrap, uv0);
    float4 noise1 = NoiseMap.Sample(LinearWrap, uv1);
    float noiseMask = saturate(noise0.r * 0.65f + noise1.g * 0.55f);

    // Albedo is only an optional mask. A white fallback texture is valid,
    // so the effect does not require a diffuse/base-color texture.
    float4 shapeTex = AlbedoMap.Sample(LinearWrap, In.vTexcoord);
    float shapeLuma = dot(shapeTex.rgb, float3(0.299f, 0.587f, 0.114f));
    float shapeMask = saturate(max(shapeTex.a, shapeLuma));

    // Frame-stepped random flash plus a subtle high-frequency pulse.
    float flickerFrame = floor(g_fTime * 30.0f * 1.150848f);
    float randomFlicker = LightningHash(flickerFrame);
    float microFlicker = 0.82f + 0.18f * sin(g_fTime * 53.0f);
    float flicker = lerp(0.62f, 1.0f, randomFlicker) * microFlicker;

    float outerMask = smoothstep(0.08f, 0.48f, noiseMask) * shapeMask;
    float coreMask = smoothstep(0.58f, 0.93f, noiseMask) * shapeMask;

    float4 particleEmissive = lerp(In.vEmissive, In.vEndEmissive, age01);

    // Particle color controls the bolt tint. Low/black input falls back to blue.
    float3 fallbackBlue = float3(0.05f, 0.25f, 1.0f);
    float particleColorAmount = max(In.vColor.r, max(In.vColor.g, In.vColor.b));
    float3 particleColor = lerp(
        fallbackBlue,
        In.vColor.rgb,
        step(0.01f, particleColorAmount));

    float3 outerColor = particleColor;
    float3 coreColor = lerp(particleColor, float3(1.0f, 1.0f, 1.0f), 0.9f);

    float3 emissiveTexture = EmissiveMap.Sample(LinearWrap, In.vTexcoord).rgb;
    float3 finalColor =
        outerColor * outerMask * 8.0f
        + coreColor * coreMask * 28.0f
        + emissiveTexture * EmissiveIntensity
        + particleEmissive.rgb * particleEmissive.a;

    finalColor *= flicker * lifeEnvelope;

    float alpha = saturate(max(outerMask, coreMask))
        * In.vColor.a
        * ObjectAlpha
        * lifeEnvelope;

    if (alpha < 0.01f)
        discard;

    return float4(finalColor, alpha);
}
