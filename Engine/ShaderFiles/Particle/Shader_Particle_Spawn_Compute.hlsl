#include "../Particle/Particle_Common_Struct_Func.hlsl"


cbuffer CB_SPAWN_COUNT : register(b12)
{
	uint g_iSpawnCount;
	uint g_iMaxParticles; // 추가: 버퍼 크기
	float2 pad;
};

StructuredBuffer<SPAWN_DATA> gSpawnBuffer : register(t6);
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
    p.startSize = s.size;
    p.endSize = s.endSize;
    p.rotation = s.rotation;
    p.size = s.size;
    p.alive = 1;
    p.color = s.color;
    p.originalVelocity = s.originalVelocity;
    p.originalEmissive = s.originalEmissive;
    p.emissive = s.emissive;
    p.endEmissive = s.endEmissive;
    p.ownerID = s.ownerID;
    p.iBehaviorType = s.iBehaviorType;
    p.loop = s.loop;
    p.originalPosition = s.originalPosition;
    
    gParticles[index] = p;
}


