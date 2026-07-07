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
    float4 emissive;
};

// VS 전용 슬롯: 파티클 시뮬레이션 결과 (t0, VS 스테이지)
StructuredBuffer<ParticleData> g_RenderBuffer : register(t0);

Texture2D g_DiffuseTexture : register(t1);
Texture2D g_NormalTexture : register(t6);
SamplerState g_LinearSampler : register(s0);

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float4 vColor : COLOR0;
    float3 vNormal : NORMAL0; // 추가
    float3 vTangent : TANGENT0; // 추가
    float3 vBinormal : BINORMAL0; // 추가
    float4 vEmissive : EMISSIVE; // 추가
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
    Out.vNormal = In.vNormal;
    Out.vTangent = In.vTangent;
    Out.vBinormal = In.vBinormal;
    Out.vColor = p.alive ? p.color : float4(p.color.rgb, 0.0f);
    Out.vEmissive = p.emissive;

    return Out;
}

struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;
    float4 vNormal : SV_TARGET1;
    float4 vSMRO : SV_TARGET2;
    float4 vEmissive : SV_TARGET3;

};
// Pixel Shader
PS_OUT PSMain(VS_OUT In) 
{
    PS_OUT Out = (PS_OUT) 0;

    float4 vTexColor = g_DiffuseTexture.Sample(g_LinearSampler, In.vTexcoord);
    if (vTexColor.a < 0.1f)   // 알파로 discard 판정 (이전에 x채널 쓰던 것도 확인 필요)
        discard;

    float4 finalColor = vTexColor * In.vColor;
    Out.vDiffuse = finalColor;

    // 노멀맵에서 tangent space 노멀 읽기 ([0,1] → [-1,1] 복원)
    float3 tangentNormal = g_NormalTexture.Sample(g_LinearSampler, In.vTexcoord).xyz * 2.0f - 1.0f;

    float3 N = normalize(In.vNormal);
    float3 T = normalize(In.vTangent);
    float3 B = normalize(In.vBinormal);
    float3x3 TBN = float3x3(T, B, N);

    float3 worldNormal = normalize(mul(tangentNormal, TBN));

    // 다른 셰이더와 같은 인코딩 방식으로 저장 ([-1,1] → [0,1])
    Out.vNormal = float4(worldNormal * 0.5f + 0.5f, 1.0f);

    Out.vSMRO = float4(0.f, 0.5f, 0.f, 1.f);
    Out.vEmissive = float4(finalColor.xyz * In.vEmissive.xyz * In.vEmissive.w, 1.0f);
    
    return Out;
}