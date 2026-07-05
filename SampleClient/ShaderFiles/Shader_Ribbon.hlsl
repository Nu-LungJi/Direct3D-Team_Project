// Shader_Beam.hlsl
// CBeam_CPU 렌더링용 - 정점이 이미 CPU에서 월드 공간(카메라를 향한 리본 형태)으로
// 계산되어 있으므로, VS는 ViewProj만 곱하면 된다 (월드 행렬/인스턴싱 없음).
// TRIANGLESTRIP + Draw()(비인덱스) 방식.
#include "./ShaderDefines.hlsl"

// UV 스크롤용 - fScrollSpeed를 쓰려면 C++ Render()에서 이 슬롯에 바인딩해야 한다.
// (지금 CBeam_CPU::Render()엔 아직 이 상수 버퍼를 채우는 코드가 없음 - 필요하면 추가 필요)
cbuffer CB_BEAM : register(b0)
{
    float g_fScrollOffset; // 누적된 시간 * fScrollSpeed (C++에서 계산해서 넘김)
    float3 _pad;
};

struct VS_IN
{
    float3 vPosition : POSITION;
    float2 vUV       : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vUV       : TEXCOORD0;
};

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out = (VS_OUT)0;

    Out.vPosition = mul(float4(In.vPosition, 1.f), g_matViewProj);

    // V(세로) 축으로 길이값이 이미 들어있으니(BuildBeamGeometry의 vUV = {0/1, t}),
    // U축이나 V축에 스크롤 오프셋을 더해 흐르는 느낌을 낸다.
    Out.vUV = In.vUV + float2(0.f, g_fScrollOffset);

    return Out;
}

Texture2D    g_BeamTexture : register(t0);
SamplerState g_Sampler     : register(s0);

float4 PSMain(VS_OUT In) : SV_TARGET
{
    float4 vTexColor = g_BeamTexture.Sample(g_Sampler, In.vUV);

    if (vTexColor.x <= 0.5f)
        discard;
    vTexColor.xyz = float3(1, 0, 0);
    return vTexColor;
}
