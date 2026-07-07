struct SPAWN_DATA
{
    float3 position;
    float3 velocity;
    float life;
    float size;
    float4 color;
    float4 emissive;
};

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

cbuffer CB_SPAWN_COUNT : register(b6)
{
    uint g_iSpawnCount;
    uint g_iMaxParticles; // 추가: 버퍼 크기
    float2 pad;
};

StructuredBuffer<SPAWN_DATA> gSpawnBuffer : register(t0);
ConsumeStructuredBuffer<uint> gDeadList : register(u0);
RWStructuredBuffer<ParticleData> gParticles : register(u1);

[numthreads(256, 1, 1)]
void CSMain(uint id : SV_DispatchThreadID)
{
    if (id >= g_iSpawnCount)
        return;

    uint index = gDeadList.Consume();
    if (index >= g_iMaxParticles)   // 언더플로우로 인한 쓰레기값 방어
        return;

    SPAWN_DATA s = gSpawnBuffer[id];
    ParticleData p = (ParticleData) 0;
    p.position = s.position;
    p.velocity = s.velocity;
    p.life = s.life;
    p.maxLife = s.life;
    p.size = s.size;
    p.alive = 1;
    p.color = s.color;
    p.emissive = s.emissive;
    
    gParticles[index] = p;
}


