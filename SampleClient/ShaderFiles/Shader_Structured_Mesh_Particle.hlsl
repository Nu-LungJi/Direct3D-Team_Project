#include "./ShaderDefines.hlsl"

struct ParticleData
{
    float3 position;
    float pad1;
    float3 velocity;
    float life;
    float maxLife;
    float size;
    uint alive;
    uint loop;
    float4 color;
};

// VS 전용 슬롯: 파티클 시뮬레이션 결과 (t0, VS 스테이지)
StructuredBuffer<ParticleData> g_RenderBuffer : register(t0);

Texture2D g_DiffuseTexture : register(t1);
SamplerState g_LinearSampler : register(s0);

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float4 vColor : COLOR0;
};

// Vertex Shader
VS_OUT VSMain(VS_IN In, uint instID : SV_InstanceID)
{
    VS_OUT Out = (VS_OUT) 0;

    ParticleData p = g_RenderBuffer[instID];

    // 죽은 파티클은 스케일 0으로 축소 + 알파 0
    float scale = p.alive ? p.size : 0.0f;

    // 행렬 없이 로컬 정점을 스케일 + 파티클 월드 위치로 이동
    float3 vWorldPos = In.vPosition * scale + p.position;

    // View x Proj는 CB_PER_PASS(b1)에서 바로 사용
    Out.vPosition = mul(float4(vWorldPos, 1.0f), g_matViewProj);
    Out.vTexcoord = In.vTexcoord;
    Out.vColor = p.alive ? p.color : float4(p.color.rgb, 0.0f);

    return Out;
}

// Pixel Shader
float4 PSMain(VS_OUT In) : SV_TARGET
{
    float4 vTexColor = g_DiffuseTexture.Sample(g_LinearSampler, In.vTexcoord);

    float4 vFinalColor = vTexColor * In.vColor;

    if (vFinalColor.a < 0.1f)
        discard;

    return vFinalColor;
}