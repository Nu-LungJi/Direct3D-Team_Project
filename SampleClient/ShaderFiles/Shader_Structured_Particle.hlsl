
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
    uint texIndex;
};

StructuredBuffer<ParticleData> g_RenderBuffer : register(t0);
Texture2D g_Texture : register(t1);
SamplerState g_LinearSampler : register(s0);

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float4 vColor : COLOR0;
};

// Vertex Shader
VS_OUT VSMain(uint vID : SV_VertexID, uint instID : SV_InstanceID)
{
    VS_OUT Out = (VS_OUT) 0;

    ParticleData p = g_RenderBuffer[instID];
    if (!p.alive)
    {
        p.color.a = 0.f;
    }
    // 가상 UV 생성 (0~3번 정점)
    float2 uv = float2(vID % 2, 1 - (vID / 2));
    Out.vTexcoord = uv;

    // 빌보드 스타일의 로컬 사각형 크기 조절 (C++에서 넘겨받은 p.size 사용!)
    float3 vLocalPos = float3((uv.x - 0.5f) * p.size, (uv.y - 0.5f) * p.size, 0.0f);

    // 행렬 없이 월드 좌표 복사: 로컬 좌표 + 파티클의 실시간 월드 위치(p.position)
    float4 vWorldPos = float4(vLocalPos + p.position, 1.0f);

    // 뷰 및 프로젝션 공간 변환
    float4 vViewPos = mul(vWorldPos, g_matView);
    Out.vPosition = mul(vViewPos, g_matProj);

    // C++에서 세팅한 주황색 불꽃 컬러 전송
    Out.vColor = p.color;

    return Out;
}

// Pixel Shader
float4 PSMain(VS_OUT In) : SV_TARGET
{
    float4 vTexColor = g_Texture.Sample(g_LinearSampler, In.vTexcoord);
    
    // 최종 주황색 틴트 컬러 결합
    float4 vFinalColor = vTexColor * In.vColor;
    

    
    if (vFinalColor.a < 0.1f)
        discard;

    return vFinalColor;
}