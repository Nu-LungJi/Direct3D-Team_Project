// Shader_Beam.hlsl
#include "../../Engine/ShaderFiles/ShaderDefines.hlsl"

cbuffer CB_BEAM : register(b0)
{
    float g_fScrollOffset;
    float3 _pad;
};

struct VS_IN
{
    float3 vPosition : POSITION;
    float2 vUV : TEXCOORD0;
    float4 vColor : INSTANCE_COLOR0;
    float4 vInstEmissive : INSTANCE_EMISSIVE;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vUV : TEXCOORD0;
    float4 vColor : COLOR0;
    float4 vEmissive : COLOR1;
};

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;
    Out.vPosition = mul(float4(In.vPosition, 1.f), g_matViewProj);
    Out.vUV = In.vUV + float2(0.f, g_fScrollOffset);
    Out.vColor = In.vColor;
    Out.vEmissive = In.vInstEmissive;
    return Out;
}

Texture2D g_BeamTexture : register(t0);
//SamplerState g_Sampler : register(s0);

// MRT(Multi-Render Target) 대응 출력 구조체
struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;
    float4 vNormal : SV_TARGET1;
    float4 vSMRO : SV_TARGET2;
    float3 vEmissive : SV_TARGET3;
};

PS_OUT PSMain(VS_OUT In)
{
    PS_OUT Out = (PS_OUT) 0;

    // 텍스처 샘플링 (정확한 변수명 vTextureColor로 통일)
    float4 vTextureColor = g_BeamTexture.Sample(LinearWrap, In.vUV);
    float4 finalColor = vTextureColor * In.vColor;
    Out.vDiffuse = finalColor;
    // 알파 테스트 혹은 특정 채널 기준 discard (여기서는 투명도나 특정 값 기준으로 처리)
    if (finalColor.a <= 0.05f)
        discard;

    // 2. Normal (빔은 이펙트이므로 기본 평면 노멀 또는 0을 줍니다. 필요시 계산)
    Out.vNormal = float4(0.f, 0.f, 0.f, 1.f);

    // 3. SMRO (Specular, Roughness, Metalness, Occlusion)
    // 비금속질에 매끄러운 느낌을 주기 위해 임의의 기본값 세팅
    Out.vSMRO = float4(0.f, 0.5f, 0.f, 1.f);

    Out.vEmissive = float4(finalColor.xyz * In.vEmissive.xyz * In.vEmissive.w, 1.0f);

    return Out;
}
