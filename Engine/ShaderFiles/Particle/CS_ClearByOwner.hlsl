#include "../Particle/Particle_Common_Struct_Func.hlsl"



cbuffer CB_CLEAR : register(b13)
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
        p.color = float4(0, 0, 0, 0); // 혹시 discard 우회해도 최소한 안 보이게
        p.emissive = float4(0, 0, 0, 0);
        p.life = 0;
        p.size = 0;
        gDeadList.Append(id);
        g_ParticleBuffer[id] = p;
    }
   // if (p.alive == 1 && p.ownerID == g_uiTargetOwnerID && p.loop == 1)
   // {
   //     p.alive = 0;
   //     gDeadList.Append(id);
   //     g_ParticleBuffer[id] = p;
   // }

}
