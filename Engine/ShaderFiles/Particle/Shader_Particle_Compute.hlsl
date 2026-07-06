// 1. C++의 CB_ParticleUpdate 구조체와 메모리 일치 (b0)
cbuffer CB_PER_PARTICLE : register(b5)
{
    float g_fTimeDelta; // 프레임 경과 시간
    uint g_iNumInstances; // 최대 파티클 개수 (1000)
    uint g_iBehaviorType; // 행동 플래그 (3)
    float g_fPadding; // 16바이트 정렬용 패딩
};

// 2. 파티클 개별 데이터 구조체
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
AppendStructuredBuffer<uint> gDeadList : register(u0);
RWStructuredBuffer<ParticleData> g_ParticleBuffer : register(u1);

[numthreads(256, 1, 1)]
void CSMain(uint id : SV_DispatchThreadID)
{
    ParticleData p = g_ParticleBuffer[id];
    if (p.alive == 0)
        return;

    p.life -= g_fTimeDelta;
    p.position += p.velocity * g_fTimeDelta;

    if (p.life <= 0)
    {
        if (p.loop == 1)
        {
            // 되살리기: 수명만 리셋하고 위치/속도는 그대로 유지
            p.life = p.maxLife;
            p.position = float3(0, 0, 0);  
        }
        else
        {
            p.alive = 0;
            gDeadList.Append(id);
        }
    }

    g_ParticleBuffer[id] = p;
}