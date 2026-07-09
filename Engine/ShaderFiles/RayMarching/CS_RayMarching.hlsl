#include "../ShaderDefines.hlsl"

Texture2D<float>    DepthTexture     : register(t0);
Texture2D<float>    ShadowMapTexture : register(t1);
Texture2D<float2>   BlueNoiseTexture : register(t2);
Texture3D<float>    VolumeTexture    : register(t3);

RWTexture2D<float4> OUTPUT : register(u0);

const static float2 ScreenResolution = { 1280.f, 720.f };
const static float2 NoiseResolution = { };
const static int    FogMaxStep = 32;

float Map(float3 _Point)
{
    float3 repeatedP = frac(_Point / 5.0f) * 5.0f - 2.5f;
    return length(repeatedP) - 1.5f;
}

float3 Compute_Normal(float3 _Point)        // 충돌 지점 Normal 계산
{
    float3 Step = float3(0.001f, 0.0f, 0.0f);
    return normalize(float3(
        Map(_Point + Step.xyy) - Map(_Point - Step.xyy),
        Map(_Point + Step.yxy) - Map(_Point - Step.yxy),
        Map(_Point + Step.yyx) - Map(_Point - Step.yyx)
    ));
}

float Compute_Shadow(float3 _Point)
{
    float4 shadowSpacePos = mul(float4(_Point, 1.0f), g_ShadowViewProj);
    shadowSpacePos.xyz /= shadowSpacePos.w;
    
    float2 shadowUV = shadowSpacePos.xy * 0.5f + 0.5f;
    shadowUV.y = 1.0f - shadowUV.y;
    
    float depth = shadowSpacePos.z;
    
    if (shadowUV.x < 0.0f || shadowUV.x > 1.0f || shadowUV.y < 0.0f || shadowUV.y > 1.0f)
        return 1.0f;
        
    return ShadowMapTexture.SampleCmpLevelZero(ShadowSampler, shadowUV, depth - 0.005f);
}

float GetVolumeFogDensity(float3 _Point)
{
    float FogHeight = max(0.f, 1.f - _Point * 0.2f);
    
    float Noise = VolumeTexture.SampleLevel(LinearWrap, _Point * 0.1f, 0.f).r;

    return FogHeight * Noise;
}



[numthreads(16, 16, 1)]
void CSMain(uint3 ID : SV_DispatchThreadID)
{
    if (ID.x >= (uint) ScreenResolution.x || ID.y >= (uint) ScreenResolution.y) return; // 스레드가 해상도 넘어가면 출력X
    
    float2  UV = (float2(ID.xy) + 0.5f) / ScreenResolution;
    
    float   PixelDepth = DepthTexture[ID.xy];               // 해당 픽셀 깊이 Read
    
    float2 NDC = UV * 2.f - 1.f;
    NDC.y = -NDC.y;
    float4  ScreenPos = float4(NDC, PixelDepth, 1.f);       // ProjSpace : xy 좌표(NDC)  + z 좌표(픽셀 깊이)
    float4  ViewPos = mul(ScreenPos, g_matinvProj);         // ViewSpace
    ViewPos /= ViewPos.w;
    float3  WorldPos = mul(ViewPos, g_matInvView).xyz;      // WorldSpace
    
    // Define Ray Data
    float3  RayOrigin = g_vCamPos;
    float3  RayDirection = normalize(WorldPos - RayOrigin);
    
    float3  RayLength = length(WorldPos - RayOrigin);
    if (PixelDepth == 0.f)  RayLength = 50.f;               // 빈 공간 (> Camera Far) 최대 길이로 설정
    
    // BlueNoise : Jittering
    float2  NoiseUV = float2(ID.xy) / NoiseResolution;
    float   Jitter = BlueNoiseTexture.SampleLevel(LinearWrap, NoiseUV, 0).r;
    
    // Ready Fog Data
    float3  LightAccumulation = float3(0.f, 0.f, 0.f);
    float   LightTransmittance = 1.f;                       // 빛 투과율 (1.f : 완전 투과 ~ 0.f : 불투과)
    
    float   StepSize = RayLength / (int) FogMaxStep;        // StepSize = 1 Step Distance
    
    float   t = StepSize * Jitter;
    
    float3  LightColor = float3(1.0f, 0.9f, 0.7f);
    
    for (int i = 0; i < FogMaxStep; ++i)
    {
        if (t >= PixelDepth)    break;
        
        float3  p = RayOrigin + RayDirection * t;
        float   VolumeDensity = GetVolumeFogDensity(p);
        
        if (VolumeDensity > 0.f)
        {
            float Shadow = Compute_Shadow(p);
            float3 CurrentLight = LightColor * Shadow;
            
            float Extinction = VolumeDensity;                   // 빛 흡수도
            float Scattering = VolumeDensity * CurrentLight;    // 빛 산란도
            
            float SampledTransmittance = exp(-Extinction * StepSize);
            
            LightAccumulation += Scattering * LightTransmittance * StepSize;
            
            LightTransmittance *= SampledTransmittance;
            
            if (LightTransmittance < 0.01f)
            {
                LightTransmittance = 0.f;
                break;
            }
        }
        t += StepSize;
    }
    float3 SceneDiffuse = OUTPUT[ID.xy].rgb;
    
    float3 FinalColor = (SceneDiffuse * LightTransmittance) + LightAccumulation;

    OUTPUT[ID.xy] = float4(FinalColor, 1.f);
}
