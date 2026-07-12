struct ParticleData
{
    float3 position;
    float pad1;
    float3 velocity;
    float life;
    float maxLife;
    float size;
    float startSize;
    uint alive;
    uint loop;
    float4 color;
    float4 emissive;
    uint frameIndex;
    uint ownerID;
    float2 pad2;
};

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
