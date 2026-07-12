#define BEHAVIOR_NONE   0
#define BEHAVIOR_SHRINK (1u << 0) 
// 1. C++의 CB_ParticleUpdate 구조체와 메모리 일치 (b0)

cbuffer CB_PER_PARTICLE : register(b5)
{
    float g_fTimeDelta; // 프레임 경과 시간
    uint g_iNumInstances; // 최대 파티클 개수 (1000)
    uint g_iBehaviorType; // 행동 플래그 (3)
    uint g_iFlipbookRows; // Row
    uint g_iFlipbookColumns; // col
    uint g_iTotalFrames; // frame
    float g_fPadding; // 16바이트 정렬용 패딩
    float g_fPadding2; // 16바이트 정렬용 패딩
};

// 2. 파티클 개별 데이터 구조체
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
    float3 pad2;
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

    if ((g_iBehaviorType & BEHAVIOR_SHRINK) != 0)
    {
        float lifeRatio = saturate(p.life / max(p.maxLife, 0.0001f));
        p.size = p.startSize * lifeRatio;
    }

    // ---- 플립북 프레임 계산 (추가) ----
    if (g_iTotalFrames > 0)
    {
        float ageRatio = saturate(1.0f - (p.life / max(p.maxLife, 0.0001f))); // 0.0(갓 태어남) ~ 1.0(죽기 직전)
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
