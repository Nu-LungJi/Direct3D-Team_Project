struct SPAWN_DATA
{
    float3 position;
    float3 velocity;
    float life;
    float size;
    float4 color;
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
    uint texIndex;
    
};

cbuffer CB_SPAWN_COUNT : register(b6)
{
    uint g_iSpawnCount;
    float3 pad;
};

StructuredBuffer<SPAWN_DATA> gSpawnBuffer : register(t0);
ConsumeStructuredBuffer<uint> gDeadList : register(u0);
RWStructuredBuffer<ParticleData> gParticles : register(u1);

[numthreads(256, 1, 1)]
void CSMain(uint id : SV_DispatchThreadID)
{
    if (id >= g_iSpawnCount)      // count는 별도 cbuffer(b6)에서
        return;

    uint index = gDeadList.Consume();
    SPAWN_DATA s = gSpawnBuffer[id]; // id로 인덱싱 → 파티클마다 다른 데이터

    ParticleData p = (ParticleData) 0;
    p.position = s.position;
    p.velocity = s.velocity;
    p.life = s.life;
    p.maxLife = s.life;
    p.size = s.size;
    p.alive = 1;
    p.color = s.color;

    gParticles[index] = p;
}