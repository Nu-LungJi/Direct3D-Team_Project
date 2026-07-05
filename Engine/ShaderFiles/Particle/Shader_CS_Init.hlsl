RWStructuredBuffer<uint> gDeadList : register(u0);

[numthreads(256, 1, 1)]
void CSMain(uint id : SV_DispatchThreadID)
{
    uint maxParticles = 1000;
    if (id >= maxParticles)
        return;

    gDeadList[id] = id;
}