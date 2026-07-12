#include "../ShaderHeader/SH_CommonFunction.hlsli"

Texture2D<float>    DepthTexture     : register(t0);
Texture2D<float>    ShadowMapTexture : register(t1);
Texture2D<float>    BlueNoiseTexture : register(t2);
Texture3D<float4>   VolumeTexture    : register(t3);

RWTexture2D<float4> OUTPUT : register(u0);

const static float2 ScreenResolution    = { 1280.f, 720.f };
const static float2 NoiseResolution     = { 256.f, 256.f };
const static int    FogMaxStep          = 32;
const static float  FogDensity          = 0.005f;

float GetVolumeFogDensity(float3 _Point)    
{
    float MinimumHeight = 0.5f;
    //float FogHeight = exp(-_Point.y * 0.05f);   
    float FogHeight = max(0.5f, 10.f - _Point.y * 2.f);
    
    float DistanceFromCam = length(_Point - g_vCamPos);
    
    float FogStartPos = 0.f, FogEndPos = 10.f;
    
    float NearFadeFactor = saturate((DistanceFromCam - FogStartPos) / (FogEndPos - FogStartPos));
    
    float4 NoiseSet = VolumeTexture.SampleLevel(LinearWrap, _Point * 0.03f, 0.f);
    
    float MainNoise = NoiseSet.r;
    float SubNoise = NoiseSet.g * 0.6f + NoiseSet.b * 0.3f + NoiseSet.a * 0.1f;
    float FinalNoise = saturate(MainNoise * SubNoise * 1.5f);
    FinalNoise = 1.f;
    return FogHeight * FinalNoise * FogDensity * NearFadeFactor;
}
float Compute_ShadowBrightness(float4 _Position)
{
    // ViewSpace Pos From ShadowCam
    float4 ShadowSpacePos = mul(_Position, g_matShadowLightViewProj);
    float2 ShadowMapUV;
    ShadowMapUV.x = (ShadowSpacePos.x) * +0.5f + 0.5f;
    ShadowMapUV.y = (ShadowSpacePos.y) * -0.5f + 0.5f;
            
    float DepthFromShadowCam = ShadowSpacePos.z;
            
    float ShadowBrightness = 1.f; // 최대 밝기 (1.f = 그림자가 안 지는 픽셀의 값)
            
    [branch]
    if (ShadowMapUV.x >= 0.0f && ShadowMapUV.x <= 1.0f && ShadowMapUV.y >= 0.0f && ShadowMapUV.y <= 1.0f)
    {
        // Compare Depth (DepthFromShadowCam : ShadowMapTexture Depth)
        // (DepthFromShadowCam < ShadowMapTexture Depth) : 1 ~ No Shadow
        // (DepthFromShadowCam > ShadowMapTexture Depth) : 0 ~ Cascade Shadow
        float ShadowFactor = ShadowMapTexture.SampleCmpLevelZero(ShadowSampler, ShadowMapUV, DepthFromShadowCam + 0.002f);

        //lerp(0.15f, 1.0f, ShadowFactor);
        ShadowBrightness = pow(ShadowFactor, 3.0f); // ShadowBrightness : 그림자의 밝기(대부분 1.f or 0.f)
    }
    return ShadowBrightness;
}

[numthreads(16, 16, 1)]
void CSMain(uint3 ID : SV_DispatchThreadID)
{
    if (ID.x >= (uint) ScreenResolution.x || ID.y >= (uint) ScreenResolution.y) return; // 스레드가 해상도 넘어가면 출력X
    
    float2  DepthTexCoord = (float2(ID.xy) + 0.5f) / ScreenResolution;
    float   Depth = DepthTexture[ID.xy]; // 해당 픽셀 깊이 Read

    // Convert WorldPosition
    float4  WorldPos = Convert_WorldPosByDepth(Depth, DepthTexCoord);
    
    // Define Ray Data
    float3  RayOrigin    = g_vCamPos;
    float3  RayVector    = WorldPos.xyz - RayOrigin;
    float3  RayDirection = normalize(RayVector);
    float   RayLength    = length(RayVector);
    
    [branch]
    if (Depth == 0.f) RayLength = 50.f; // 빈 공간 (> Camera Far) 최대 길이로 설정
    
    float   RayStepSize = RayLength / FogMaxStep; // StepSize = 1 Step Distance
    
    // BlueNoise : Jittering
    float2  NoiseTexCoord = float2(ID.xy) / NoiseResolution;
    float   BlueNoise = BlueNoiseTexture.SampleLevel(LinearWrap, NoiseTexCoord, 0);
    
    // Initialize Fog Data
    float3  LightAccumulation = float3(0.f, 0.f, 0.f);
    float   LightTransmittance = 1.f; // 빛 투과율 (1.f : 완전 투과 ~ 0.f : 불투과)

    float3  LightColor = float3(1.0f, 1.f, 1.f);

    [unroll]
    for (int i = 0; i < FogMaxStep; ++i)
    {
        // Move Forward Ray Per RayStepSize With Noise
        float RayProgress = RayStepSize * (i + BlueNoise);
        
        [branch]
        if (RayProgress >= RayLength - 0.01f) break;
        
        float3 CurrentPosition = RayOrigin + RayDirection * RayProgress;
        float  VolumeDensity   = GetVolumeFogDensity(CurrentPosition);
        
        [branch]
        if (VolumeDensity > 0.f)
        {
            float ShadowBrightness = Compute_ShadowBrightness(float4(CurrentPosition, 1.f));
            
            float Extinction = VolumeDensity;                                   // 빛 흡수도 (밀도)
            float Scattering = VolumeDensity * LightColor * ShadowBrightness;   // 빛 산란도 ((밀도 * 색상) * 그림자 Factor)
            
            float SampledTransmittance = exp(-Extinction * RayStepSize);        // SampledTransmittance : 이번 칸을 지나며 살아남은 빛의 비율 (0.f ~ 1.f)
            
            LightAccumulation += Scattering * LightTransmittance * RayStepSize; 
            
            LightTransmittance *= SampledTransmittance;                         // 깊이 기반 감쇄 (전진할 수록 감쇄)
            
            [branch]
            if (LightTransmittance < 0.01f)                                     // Max Fog Transmittance = Fog가 최대 밀도면 탈출
            {
                LightTransmittance = 0.f;
                break;
            }
        }
    }
    
    OUTPUT[ID.xy] = float4(LightAccumulation, LightTransmittance);
    return;
}
