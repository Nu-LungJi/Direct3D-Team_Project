


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
    p.size = s.size;
    p.alive = 1;
    p.color = s.color;
    p.emissive = s.emissive;
    gParticles[index] = p;
}


