#include "../Particle/Particle_Common_Struct_Func.hlsl"



cbuffer CB_PER_PARTICLE : register(b5)
{
    float g_fTimeDelta;
    uint g_iNumInstances;
    uint g_iFlipbookRows;
    uint g_iFlipbookColumns;
    uint g_iTotalFrames;
    float g_fTime;
    float2 g_fPadding;
};

cbuffer CB_CIRCLE_TO_WAVE : register(b10)
{
    float3 g_vFlowDirection; // 물결이 흘러가는 방향 (정규화, XZ 평면 기준)
    float g_fBurstRatio; // ageRatio 기준, 이 시점까지 원형 확산 (예: 0.3)
 
    float g_fTransitionRatio; // 전환 구간 폭 (ageRatio 기준, 예: 0.15)
    float g_fBurstSpeed; // 원형 확산 초기 속도
    float g_fFlowSpeed; // 물결이 흘러가는 이동 속도
    float g_fWaveAmplitude; // 상하 진폭
 
    float g_fWaveFrequency; // 공간적 파장 (위치에 따른 위상차)
    float g_fWaveSpeed; // 시간에 따른 위상 변화 속도
    float2 g_fPadding2;
};
float Hash01(uint x)
{
    x = (x ^ 61u) ^ (x >> 16u);
    x *= 9u;
    x = x ^ (x >> 4u);
    x *= 0x27d4eb2du;
    x = x ^ (x >> 15u);
    return frac((float) x * 2.3283064365386963e-10); // 1 / 2^32
}
AppendStructuredBuffer<uint> gDeadList : register(u0);
RWStructuredBuffer<ParticleData> g_ParticleBuffer : register(u1);
 
[numthreads(256, 1, 1)]
void CSMain(uint id : SV_DispatchThreadID)
{
    ParticleData p = g_ParticleBuffer[id];
    if (p.alive == 0)
        return;

    p.life -= g_fTimeDelta;
    if ((p.iBehaviorType & BEHAVIOR_GRAVITY) != 0)
    {
        const float kGravity = -9.8f;
        p.velocity.y += kGravity * g_fTimeDelta; 
    }
    p.position += p.velocity * g_fTimeDelta; 
    float ageRatio = saturate(1.0f - (p.life / max(p.maxLife, 0.0001f)));
    p.size = lerp(p.startSize, p.endSize, ageRatio);
    
    

    if ((p.iBehaviorType & BEHAVIOR_CIRCLE_TO_WAVE) != 0)
    {
        float rnd1 = Hash01(id);
        float rnd2 = Hash01(id ^ 0x9E3779B9u);
        float angle = rnd1 * 6.28318530718;
        float phaseOffset = rnd2 * 6.28318530718;
 
        // 1단계: 0에서 가속했다가 목표 시점(burstRatio)에서 정확히 0으로 감속
        //        (기존 (1-ease) 방식은 스폰 즉시 최고속도로 튀어나가서 "펑!" 터지는
        //         폭죽처럼 보였음. sin 프로파일은 0 -> 최고속 -> 0 으로 부드럽게
        //         움직여서 원이 서서히 "펼쳐지는" 느낌을 줌)
        float burstT = saturate(ageRatio / max(g_fBurstRatio, 0.0001f));
        float burstProfile = sin(3.14159265f * burstT); // t=0,1에서 0, t=0.5에서 최대
        // 카메라를 마주보는 X-Y 평면(빌보드)에서 원형으로 확산 (Z=0 고정)
        float3 radialDir = float3(cos(angle), sin(angle), 0.0f);
        float3 burstVelocity = radialDir * g_fBurstSpeed * burstProfile;
 
        // 2단계: 흐름 방향(X-Y 평면) + 그 방향과 수직인 축으로 좌우 출렁임
        //        (Z축으로 출렁이면 빌보드는 카메라 쪽으로 앞뒤로만 움직여서
        //         화면상 파도처럼 안 보임 -> 같은 평면 안에서 옆으로 흔들어야 함)
        //float2 flowDirXY = normalize(g_vFlowDirection.xy + 1e-6f); // 0벡터 방지용 작은 값
        float2 flowDirXY = radialDir.xy; // 0벡터 방지용 작은 값
        float2 perpXY = float2(-flowDirXY.y, flowDirXY.x); // flowDir과 수직인 방향
 
        float elapsed = p.maxLife - p.life; // 파티클 개별 경과시간(초)
        float wavePhase = g_fWaveSpeed * elapsed + phaseOffset
                         + dot(p.position.xy, flowDirXY) * g_fWaveFrequency;
        float waveBob = sin(wavePhase);
        float3 waveVelocity = float3(flowDirXY * g_fFlowSpeed, 0.0f)
                             + float3(perpXY * (waveBob * g_fWaveAmplitude), 0.0f);
 
        // 두 단계를 부드럽게 블렌딩 (burstRatio 근처에서 전환)
        float blend = smoothstep(g_fBurstRatio - g_fTransitionRatio * 0.5f,
                                  g_fBurstRatio + g_fTransitionRatio * 0.5f,
                                  ageRatio);
 
        p.velocity = lerp(burstVelocity, waveVelocity, blend);
    }
    
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
            p.position = p.originalPosition;
            p.velocity = p.originalVelocity;
            p.alive = 1;
            p.emissive = p.originalEmissive;

        }
        else
        {
            p.alive = 0;
            p.size = 0;
            p.color = 0;
            p.ownerID = 0;
            p.frameIndex = 0;
            p.originalPosition = 0;
            p.iBehaviorType = 0;
            p.velocity = 0;
            
            gDeadList.Append(id);
        }
    }
    
    g_ParticleBuffer[id] = p;
}
