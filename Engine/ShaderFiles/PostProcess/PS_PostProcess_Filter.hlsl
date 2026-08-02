#include "../ShaderDefines.hlsl"
// Color LUT + ToneMapping + Vignetting + Noise

// 지금까지 씬에 그려진 텍스쳐 바인딩
Texture2D SceneColorTexture : register(t0);
Texture2D LUT_Texture       : register(t1); // LUT 필터 3D 텍스쳐

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

static const float Min_Luminance = -12.47393f;
static const float Max_Luminance = 4.026069f;

static const float DistortionIntensity	= 0.f; // 왜곡 강도
static const float ChromaticIntensity	= 0.f; // 색수차 강도
static const float VignetteIntensity	= 0.f; // 비네팅 강도
static const float VignetteSmoothness	= 0.f; // 비네팅

// Distortion
float2 Distortion(float2 _UV)
{
    float2 Coord = _UV * 2.f - 1.f;
    
    float Coord2 = dot(Coord, Coord);
    
    float2 DistortedUV = Coord * (1.f + DistortionIntensity * Coord2);
    
    return (DistortedUV + 1.f) * 0.5f;
}

// Chromatic Aberration
float3 ChromaticAberration(float2 _UV)
{
    float2 UVFromCenter = _UV - 0.5f;
    float2 DistanceFromCenter = length(UVFromCenter);
    
    float2 Seperation = UVFromCenter * (DistanceFromCenter * ChromaticIntensity);
    
    float R = SceneColorTexture.Sample(LinearClamp, _UV - Seperation).r;
    float G = SceneColorTexture.Sample(LinearClamp, _UV).g;
    float B = SceneColorTexture.Sample(LinearClamp, _UV + Seperation).b;

    return float3(R, G, B);
}

// Vignetting 
float3 Vignetting(float3 _Color, float2 _UV)
{
    float2 UVFromCenter = _UV - 0.5f;
    
    float DistanceFromCenter = dot(UVFromCenter, UVFromCenter);
    float Vignette = DistanceFromCenter * VignetteIntensity;
    Vignette = saturate(1.f - Vignette * VignetteSmoothness);
    
    return _Color * pow(Vignette, 2.f);
}

// LUT ColorGrading
float3 LUT_Filtering(float3 _Color)
{
    float3 Color = saturate(_Color);
    
    float BlueValue = Color.b * (LUT_Size - 1.f);
    
    float AdjustTile01 = floor(BlueValue);
    float AdjustTile02 = ceil(BlueValue);
    
    float2 LUT_UVOffset = Color.rg * ((LUT_Size - 1.f) / LUT_Size) + (0.5f / LUT_Size);
    
    float2 TexCoordA, TexCoordB;
    TexCoordA.x = (AdjustTile01 + LUT_UVOffset.x) / LUT_Size;
    TexCoordA.y = LUT_UVOffset.y;
    
    TexCoordB.x = (AdjustTile02 + LUT_UVOffset.x) / LUT_Size;
    TexCoordB.y = LUT_UVOffset.y;
    
    float3 LUT_ColorA = LUT_Texture.SampleLevel(LinearClamp, TexCoordA, 0.f).rgb;
    float3 LUT_ColorB = LUT_Texture.SampleLevel(LinearClamp, TexCoordB, 0.f).rgb;
    
    //float3 LUT_Mapping = _Color * ((LUT_Size - 1.f) / LUT_Size) + (0.5f / LUT_Size);
    //return LUT_Texture.Sample(SamplerClamp, LUT_Mapping);
    
    return lerp(LUT_ColorA, LUT_ColorA, frac(BlueValue));
}

// ToneMapping : Reinhard / ACESFilmic / AGXFilmic
float3 ToneMap_Reinhard(float3 _Color)
{
    return _Color.xyz / (_Color.xyz + 1.f);
}

float3 ToneMap_ACESFilm(float3 _Color)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    
    return saturate((_Color * (a * _Color + b)) / (_Color * (c * _Color + d) + e));
}

float3 ToneMap_Uchimura(float3 _Color)
{
	const float DisplayMaxBrightness = 1.f;
	const float Contrast = 1.f;
	const float LinearAreaStart = 0.22f;
	const float LinearAreaLength = 0.44f;
	const float BlackConstrast = 1.33f;
	const float BlackLift = 0.f;
	
	float3 Color = max(0.f, _Color);
	
	float	l0 = ((DisplayMaxBrightness - LinearAreaStart) * LinearAreaLength) / Contrast;
	float	L0 = LinearAreaStart - LinearAreaStart / Contrast;
	float	L1 = LinearAreaStart + (1.f - LinearAreaStart) / Contrast;
	float	S0 = LinearAreaStart + l0;
	float	S1 = LinearAreaStart + Contrast * l0;
	float	C2 = (Contrast * DisplayMaxBrightness) / (DisplayMaxBrightness - S1);
	float	CP = -C2 / DisplayMaxBrightness;

	float3	w0 = 1.f - smoothstep(0.f, LinearAreaStart, Color);
	float3	w2 = step(LinearAreaStart + l0, Color);
	float3	w1 = 1.f - w0 - w2;

	float3	T = LinearAreaStart * pow(Color / LinearAreaStart, BlackConstrast) + BlackLift;
	float3	L = LinearAreaStart + Contrast * (Color - LinearAreaStart);
	float3	S = DisplayMaxBrightness - (DisplayMaxBrightness - S1) * exp(CP * (Color - S0));

	return saturate(T * w0 + L * w1 + S * w2);
}

float3 ToneMap_UnCharted(float3 _Color)
{	
	float A = 0.22f;
	float B = 0.30f;
	float C = 0.10f;
	float D = 0.20f;
	float E = 0.01f;
	float F = 0.30f;

	return ((_Color * (A * _Color + C * B) + D * E) / (_Color * (A * _Color + B) + D * F)) - (E / F);
}

float3 AGXFilmic(float3 _Color)
{
	float3 x = _Color;
	float3 x2 = x * x;
	float3 x4 = x2 * x2;

	float3 result = 15.5f * x4 * x2 - 40.14f * x4 * x + 31.96f * x4
        - 6.868f * x2 * x + 0.4298f * x2 + 0.1191f * x - 0.00232f;

	return saturate(result);
}

float3 ToneMap_AGXFilm(float3 _Color)
{
	float3 AGXColor = mul(_Color, AGX_InMatrix);
	
	float MaxEV = log2(Max_Luminance);
	float MinEV = log2(Min_Luminance);
	
	AGXColor = clamp(AGXColor, Min_Luminance, Max_Luminance);\
	
	float3 LogColor = (log2(AGXColor) - MinEV) / (MaxEV - MinEV);
	LogColor = saturate(LogColor);
	
	float3 FilmColor = AGXFilmic(LogColor);
	FilmColor = saturate(FilmColor);
	
	return saturate(mul(FilmColor, AGX_OutMatrix));
}

float4 PSMain(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
    // UV Distortion
    float2 DistortedCoord = Distortion(TexCoord);
    
    // Chromatic Aberration
    float3 FinalColor = ChromaticAberration(DistortedCoord);
    
    // ToneMapping
    FinalColor = ToneMap_ACESFilm(FinalColor);
    //FinalColor = ToneMap_Reinhard(FinalColor);
    //FinalColor = ToneMap_Uchimura(FinalColor);

    //FinalColor = ToneMap_UnCharted(FinalColor * 2.f);
	//float		W = 11.2f;
	//float3	WhiteScale = 1.f / ToneMap_UnCharted(float3(W, W, W));
	//FinalColor = FinalColor * WhiteScale;
	
    //FinalColor = ToneMap_AGXFilm(FinalColor); // 일단 사용X
    
    // LUT ColorGrading
    FinalColor = LUT_Filtering(FinalColor);
    
    // Vignette
    FinalColor = Vignetting(FinalColor, TexCoord);
	
	FinalColor = saturate(FinalColor);
	// Gamma Correction
	return float4(pow(FinalColor, 1.f / 2.2f), 1.f);
}
