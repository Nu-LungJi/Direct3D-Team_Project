cbuffer CB_INIT_PARTICLE : register(b0)
{
    uint g_iMaxParticles;
    float3 pad;
};

AppendStructuredBuffer<uint> gDeadList : register(u0);

[numthreads(256, 1, 1)]
void CSMain(uint id : SV_DispatchThreadID)
{
    if (id >= g_iMaxParticles)
        return;
    gDeadList.Append(id);
}