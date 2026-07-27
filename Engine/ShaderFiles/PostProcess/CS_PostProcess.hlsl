#include "../ShaderHeader/SH_CommonFunction.hlsli"

Texture2D<float4> OriginalTexture	: register(t0);
Texture2D<float4> BlurPassTexture	: register(t1);
Texture2D<float4> LUT_Texture		: register(t2);

static const float CenterWeight		= { 0.227027f };
static const float BlurOffsets[2]	= { 1.3846154f, 3.2307692f };
static const float BlurWeights[2]	= { 0.3162162f, 0.0702703f };

static const float BrightThreshold	= { 0.25f };
static const float BloomIntensity	= { 0.25f };

// LUT ColorGrading Global Variable
static const float LUT_Size = 16.f;

// ToneMapping Global Variable
static const float3x3 AGX_InMatrix = float3x3(
    0.84242023, 0.07783290, 0.07974687,
    0.04561999, 0.84068596, 0.11369405,
    0.01731649, 0.07604894, 0.90663457
);

static const float3x3 AGX_OutMatrix = float3x3(
    1.19687984, -0.05289685, -0.14398300,
    -0.05886190, 1.15190313, -0.09304123,
    -0.01497570, -0.08150601, 1.09648171
);
