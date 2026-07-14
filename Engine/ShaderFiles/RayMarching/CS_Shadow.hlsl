#include "../ShaderHeader/SH_CommonFunction.hlsli"

Texture2D<float> DepthTexture           : register(t0);

Texture2D<float> StaticShadowTexture    : register(t1);
Texture2D<float> DynamicShadowTexture   : register(t2);

RWTexture2D<float> OUTPUT               : register(u0);

static const float2 ScreenResolution    = { 1280.f, 720.f };
static const float2 ShadowMapResolution = { 1280.f * 2.f, 720.f * 2.f };

static const float  ShadowSmoothness    = 1.5f;
static const float  ShadowBrightness    = 0.45f;

static const float2 PoissonDisk[8] =
{
    float2(0.000000, 0.000000), float2(0.527837, -0.085868), float2(-0.040062, 0.536087), float2(-0.670445, -0.179949),
    float2(-0.419418, -0.616039), float2(0.440453, 0.639399), float2(-0.757088, 0.349334), float2(0.574619, -0.715851)
};

float Get_GradientNoise(float2 _PixelPos)
{
    return frac(sin(dot(_PixelPos, float2(12.9898, 78.233))) * 43758.5453123);
}

float Compute_SmoothShadow(float4 _WorldPos, float2 _TexCoord, float2 _PixelPos)
{
    float4 LightPos = mul(_WorldPos, g_matShadowLightViewProj);
    
    float2 ShadowMapUV;
    ShadowMapUV.x = (LightPos.x / LightPos.w) * +0.5f + 0.5f;
    ShadowMapUV.y = (LightPos.y / LightPos.w) * -0.5f + 0.5f;
    
    if (ShadowMapUV.x < 0.f || ShadowMapUV.x > 1.f || ShadowMapUV.y < 0.f || ShadowMapUV.y > 1.f)   return 1.f;
    
    float CurrentPixelDepth = LightPos.z;
    CurrentPixelDepth -= 0.0005f; // Depth Bias
    
    float RandomNoise = Get_GradientNoise(_PixelPos);
    float RandomAngle = RandomNoise * 2.f * PI;
    
    float CosAngle = cos(RandomAngle);
    float SinAngle = sin(RandomAngle);
    
    float2x2 RotationMat = float2x2(CosAngle, -SinAngle, SinAngle, CosAngle);
    
    // 주변 ShadowSmoothness 반경까지 Sampling
    float2 SamplingRange = 1.f / ShadowMapResolution * ShadowSmoothness;
    
    float StaticShadowFactor = 0.0f;
    float DynamicShadowFactor = 0.0f;

    [unroll]
    for (int i = 0; i < 8; ++i)
    {
        float2 RotatedOffset = mul(PoissonDisk[i], RotationMat);
        
        float2 SampleUV = ShadowMapUV + (RotatedOffset * SamplingRange);
        
        // SampleCmpLevelZero : Texture2D(ShadowMap)의 깊이와 CompareValue(CurrentPixelDepth) 를 비교했을 때 
        // CompareValue가 크면 1, 아니면 0 반환.(x값에 결과값 저장)
        StaticShadowFactor += StaticShadowTexture.SampleCmpLevelZero(ShadowSampler, SampleUV, CurrentPixelDepth).x;
        DynamicShadowFactor += DynamicShadowTexture.SampleCmpLevelZero(ShadowSampler, SampleUV, CurrentPixelDepth).x;
    }
    
    StaticShadowFactor  /= 8.f;
    DynamicShadowFactor /= 8.f;
    
    float FinalShadowFactor = min(StaticShadowFactor, DynamicShadowFactor);
    
    return lerp(ShadowBrightness, 1.0f, FinalShadowFactor);
}

[numthreads(16, 16, 1)]
void CSMain(uint3 ID : SV_DispatchThreadID ) {
    if (ID.x >= (uint) ScreenResolution.x || ID.y >= (uint) ScreenResolution.y) return; // 스레드가 해상도 넘어가면 출력X
    
    float2  DepthTexCoord = (float2(ID.xy) + 0.5f) / ScreenResolution;
    float   Depth = DepthTexture[ID.xy]; // 해당 픽셀 깊이 Read
    
    [branch]
    if (Depth >= 1.f)
    {
        OUTPUT[ID.xy] = 1.f;
        return;
    }
    
    float4 DepthWorld   = Convert_WorldPosByDepth(Depth, DepthTexCoord);
    
    float ShadowFactor  = Compute_SmoothShadow(DepthWorld, DepthTexCoord, float2(ID.xy));
    
    OUTPUT[ID.xy] = ShadowFactor;
    return;
}
