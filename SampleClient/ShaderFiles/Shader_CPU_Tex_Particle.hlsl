#include "../../Engine/ShaderFiles/ShaderDefines.hlsl"


struct VS_IN
{
    // Per-Vertex - 쿼드 메쉬 로컬 좌표 (-0.5~0.5), UV
    float3 vPosition : POSITION;
    float2 vTexcoord : TEXCOORD0;

    // Per-Instance - VTX_PARTICLE_INSTANCED_DATA와 바이트 레이아웃 일치.
    // "INSTANCE_" 접두사가 있어야 CResVertexShader::Load()의 리플렉션이
    // 이 필드들을 슬롯 1(인스턴스 버퍼)로 인식한다.
    float4 vWorld0 : INSTANCE_WORLD0;
    float4 vWorld1 : INSTANCE_WORLD1;
    float4 vWorld2 : INSTANCE_WORLD2;
    float4 vWorld3 : INSTANCE_WORLD3;
    float4 vColor : INSTANCE_COLOR0;
    float4 vInstEmissive : INSTANCE_EMISSIVE;
    float2 uvOffset : INSTANCE_UVOFFSET;
    float2 uvSize : INSTANCE_UVSIZE;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float4 vColor : COLOR0;
    float4 vEmissive : COLOR1;
};

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;

    float4x4 matWorld = float4x4(In.vWorld0, In.vWorld1, In.vWorld2, In.vWorld3);

    // matWorld엔 회전이 없다 (C++ 쪽에서 Scale * Translation만 곱함).
    // 중심 위치/스케일만 뽑아내고, 회전은 여기서 카메라 축으로 직접 만든다 (빌보드).
    float3 vCenter = float3(matWorld._41, matWorld._42, matWorld._43);
    float fScale = matWorld._11; // 균등 스케일 가정

    float3 vRight = float3(g_matView._11, g_matView._21, g_matView._31);
    float3 vUp = float3(g_matView._12, g_matView._22, g_matView._32);

    float3 vWorldPos = vCenter
                      + vRight * In.vPosition.x * fScale
                      + vUp * In.vPosition.y * fScale;

    Out.vPosition = mul(float4(vWorldPos, 1.f), g_matViewProj);
    Out.vTexcoord = In.uvOffset + In.vTexcoord * In.uvSize;
    Out.vColor = In.vColor;
    Out.vEmissive = In.vInstEmissive;
    return Out;
}

Texture2D g_ParticleTexture : register(t0);
SamplerState g_Sampler : register(s0);

struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;


};
PS_OUT PSMain(VS_OUT In)
{
 
    PS_OUT Out = (PS_OUT) 0;

    float4 texColor = g_ParticleTexture.Sample(g_Sampler, In.vTexcoord) * In.vColor;
    //if (texColor.a <= 0.01f)
    //    discard;

    float3 instEmissive = In.vEmissive.rgb * In.vEmissive.w;
    float3 FinalColor = texColor.rgb + instEmissive;

    Out.vDiffuse = float4(FinalColor, texColor.a);
    return Out;
}
