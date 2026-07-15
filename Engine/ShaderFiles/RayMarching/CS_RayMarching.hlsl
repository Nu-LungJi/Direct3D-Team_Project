#include "../ShaderHeader/SH_CommonFunction.hlsli"

Texture2D<float>    DepthTexture     : register(t0);
Texture2D<float>    ShadowMapTexture : register(t1);
Texture2D<float>    BlueNoiseTexture : register(t2);
Texture3D<float>    VolumeTexture    : register(t3);

RWTexture2D<float4> OUTPUT : register(u0);

const static float2 ScreenResolution    = { 1280.f, 720.f };
const static float2 NoiseResolution     = { 256.f, 256.f };
const static int    FogMaxStep          = 64;

float GetVolumeFogDensity(float3 _Point)    
{
    //float FogHeight = exp(-_Point.y * 0.05f);   
    float FogHeight = max(0.5f, 10.f - _Point.y * 2.f);
    
    float DistanceFromCam = length(_Point - g_vCamPos);
    
    float FogStartPos = 20.f, FogEndPos = 50.f;
    
    float NearFadeFactor = saturate((DistanceFromCam - FogStartPos) / (FogEndPos - FogStartPos));
    
    float Noise = 1.f; // VolumeTexture.SampleLevel(LinearWrap, _Point * 0.1f, 0.f).r;
    float FogDensityScale = 0.005f;
    
    return FogHeight * Noise * FogDensityScale; // * NearFadeFactor;
}

[numthreads(16, 16, 1)]
void CSMain(uint3 ID : SV_DispatchThreadID)
{
    if (ID.x >= (uint) ScreenResolution.x || ID.y >= (uint) ScreenResolution.y) return; // 스레드가 해상도 넘어가면 출력X
    
    float2  TexCoord = (float2(ID.xy) + 0.5f) / ScreenResolution;
    float   Depth = DepthTexture[ID.xy]; // 해당 픽셀 깊이 Read

    // Convert WorldPosition
    float4 WorldPos = Convert_WorldPosByDepth(Depth, TexCoord);
    
    // Define Ray Data
    float3  RayOrigin    = g_vCamPos;
    float3  RayVector    = WorldPos.xyz - RayOrigin;
    float3  RayDirection = normalize(RayVector);
    float   RayLength    = length(RayVector);
    if (Depth == 0.f) RayLength = 50.f; // 빈 공간 (> Camera Far) 최대 길이로 설정
    
    float RayStepSize = RayLength / FogMaxStep; // StepSize = 1 Step Distance
    
    // BlueNoise : Jittering
    float2  NoiseTexCoord = float2(ID.xy) / NoiseResolution;
    float   BlueNoise = BlueNoiseTexture.SampleLevel(LinearWrap, NoiseTexCoord, 0);
    
    // Initialize Fog Data
    float3  LightAccumulation = float3(0.f, 0.f, 0.f);
    float   LightTransmittance = 1.f; // 빛 투과율 (1.f : 완전 투과 ~ 0.f : 불투과)

    // God Ray
    //float4 ShadowOrigin = mul(float4(RayOrigin, 1.f), g_matShadowLightViewProj);
    //float4 ShadowDirection = mul(float4(RayDirection, 1.f), g_matShadowLightViewProj);
    
    float3  LightColor = float3(1.0f, 0.9f, 0.7f);

    [unroll]
    for (int i = 0; i < FogMaxStep; ++i)
    {
        // Noise Offset
        float Offset = RayStepSize * (i + BlueNoise);
        
        [branch]
        if (Offset >= RayLength - 0.01f)    break;
        
        float3  CurrentPosition = RayOrigin + RayDirection * Offset;
        float   VolumeDensity   = GetVolumeFogDensity(CurrentPosition);
        
        [branch]
        if (VolumeDensity > 0.f)
        {
            float4 ShadowSpacePos = mul(float4(CurrentPosition, 1.0f), g_matShadowLightViewProj);
            float2 ShadowMapUV;
            ShadowMapUV.x = (ShadowSpacePos.x) * +0.5f + 0.5f;
            ShadowMapUV.y = (ShadowSpacePos.y) * -0.5f + 0.5f;
            
            float CurrentPixelDepth = ShadowSpacePos.z;
            
            float Shadow = 1.0f;    // 최대 밝기 (1.f = 그림자가 안 지는 픽셀의 값)
            
            [branch]
            if (ShadowMapUV.x >= 0.0f && ShadowMapUV.x <= 1.0f && ShadowMapUV.y >= 0.0f && ShadowMapUV.y <= 1.0f)
            {   
                float ShadowFactor = ShadowMapTexture.SampleCmpLevelZero(ShadowSampler, ShadowMapUV, CurrentPixelDepth + 0.002f);
                //Shadow = ShadowFactor;//lerp(0.15f, 1.0f, ShadowFactor);
                Shadow = pow(ShadowFactor, 3.0f);
            }
            
            float Extinction = VolumeDensity;                       // 빛 흡수도
            float Scattering = VolumeDensity * LightColor * Shadow; // 빛 산란도
            
            float SampledTransmittance = exp(-Extinction * RayStepSize);
            
            LightAccumulation += Scattering * LightTransmittance * RayStepSize;
            
            LightTransmittance *= SampledTransmittance;
            
            [branch]
            if (LightTransmittance < 0.01f)
            {  
                LightTransmittance = 0.f;
                break;
            }
        }
        //Offset += RayStepSize;  
    }
    
    OUTPUT[ID.xy] = float4(LightAccumulation, LightTransmittance);
    return;
}
