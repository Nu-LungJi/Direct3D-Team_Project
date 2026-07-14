#include "../Particle/Particle_Common_Struct_Func.hlsl"



cbuffer CB_CLEAR : register(b7)
{
    uint g_uiTargetOwnerID;
    float3 pad;
};

RWStructuredBuffer<ParticleData> g_ParticleBuffer : register(u0);
AppendStructuredBuffer<uint> gDeadList : register(u1);

[numthreads(256, 1, 1)]
void CSMain(uint id : SV_DispatchThreadID)
{
    ParticleData p = g_ParticleBuffer[id];

    if (p.alive == 1 && p.ownerID == g_uiTargetOwnerID)
    {
        p.alive = 0;
        gDeadList.Append(id);
        g_ParticleBuffer[id] = p;
    }
}
