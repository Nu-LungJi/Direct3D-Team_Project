// 1. C++의 CB_ParticleUpdate 구조체와 메모리 일치 (b0)
cbuffer CB_ParticleUpdate : register(b0)
{
    float g_fTimeDelta; // 프레임 경과 시간
    uint g_iNumInstances; // 최대 파티클 개수 (1000)
    uint g_iBehaviorType; // 행동 플래그 (3)
    float g_fPadding; // 16바이트 정렬용 패딩
};

// 2. 파티클 개별 데이터 구조체
struct ParticleData
{
    float4x4 matWorld; // 월드 변환 행렬 (위치 포함)
    float4 vColor; // 블록 틴트 컬러
    float4 light; // 보셀 조명 정보
    uint4 texIndexs; // 텍스처 인덱스 배열
    float life; // 파티클 수명
    float3 vVelocity; // 이동 속도
};

// C++에서 묶은 UAV 쓰기 통로 (u0)
RWStructuredBuffer<ParticleData> g_ParticleBuffer : register(u0);

[numthreads(256, 1, 1)]
void CSMain(uint3 dtID : SV_DispatchThreadID)
{
    // 배열의 인덱스가 파티클 최대 개수를 초과하면 예외 처리
    if (dtID.x >= g_iNumInstances)
        return;

    // 해당 스레드가 담당할 파티클 데이터 가져오기
    ParticleData p = g_ParticleBuffer[dtID.x];

    // 1. 수명 업데이트
    p.life += g_fTimeDelta;
    
    if (p.life > 60.0f)
    {
        // 수명이 다했을 때의 처리 (예: 스케일을 0으로 만들거나 멀리 보냄)
        p.matWorld._41_42_43 = float3(9999.0f, 9999.0f, 9999.0f);
        g_ParticleBuffer[dtID.x] = p;
        return;
    }

    // 2. Y축 낙하 및 중력 물리 연산
    p.vVelocity.y -= 9.8f * g_fTimeDelta;
    p.matWorld._41_42_43 += p.vVelocity * g_fTimeDelta;

    // 계산이 끝난 데이터를 다시 구조화 버퍼에 덮어씁니다.
    g_ParticleBuffer[dtID.x] = p;
}