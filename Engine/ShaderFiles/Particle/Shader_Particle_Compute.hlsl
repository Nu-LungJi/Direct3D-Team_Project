#define BEHAVIOR_NONE   0
#define BEHAVIOR_DISTORTION (1u << 0) 

cbuffer CB_PER_PARTICLE : register(b5)
{
    float g_fTimeDelta;
    uint g_iNumInstances;
    uint g_iBehaviorType;
    uint g_iFlipbookRows;
    uint g_iFlipbookColumns;
    uint g_iTotalFrames;
    float g_fPadding;
    float g_fPadding2;
};

struct ParticleData
{
    float3 position;
    float pad1;
    float3 velocity;
    float life;
    float maxLife;
    float size;
    float startSize;
    float EndSize;
    uint alive;
    uint loop;
    float4 color;
    float4 emissive;
    uint frameIndex;
    uint ownerID;
    float pad2;
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
    float ageRatio = saturate(1.0f - (p.life / max(p.maxLife, 0.0001f)));
    p.size = lerp(p.startSize, p.EndSize, ageRatio);
    
    if (g_iTotalFrames > 0)
    {
        uint frame = (uint) (ageRatio * g_iTotalFrames);
        p.frameIndex = min(frame, g_iTotalFrames - 1);
    }
    else
    {
        p.frameIndex = 0;
    }

    if (p.life <= 0)
    {
        if (p.loop == 1)
        {
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
