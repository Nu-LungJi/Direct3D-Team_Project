// 1. C++의 CB_ParticleUpdate 구조체와 메모리 순서/크기(16바이트 정렬) 일치
cbuffer CB_ParticleUpdate : register(b0)
{
    float g_fTimeDelta; // 프레임 경과 시간
    uint g_iNumInstances; // 최대 파티클 개수 (1000)
    uint g_iBehaviorType; // 행동 플래그 (3)
    float g_fPadding; // 16바이트 정렬용 패딩
};

// 2. 파티클 개별 데이터 구조체 (C++의 VTX_FIRE_INSTANCED_DATA 또는 VTX_DROP_BLOCK_INSTANCED_DATA와 일치)
struct ParticleData
{
    float4x4 matWorld; // 월드 변환 행렬
    float4 vColor; // 블록 틴트 컬러
    float4 light; // 보셀 조명 정보
    uint4 texIndexs; // 텍스처 인덱스 배열
    float life; // 파티클 수명
    float3 vVelocity; // 이동 속도 (필요 시 추가)
};

// C++에서 0번 슬롯에 묶은 UAV 쓰기 통로 (pContext->CSSetUnorderedAccessViews(0, 1, ...))
RWStructuredBuffer<ParticleData> g_ParticleBuffer : register(u0);

// 스레드는 256개 단위로 한 그룹을 이룹니다.
[numthreads(256, 1, 1)]
void CS_MAIN(uint3 dtID : SV_DispatchThreadID)
{
    // 배열의 인덱스가 파티클 최대 개수를 초과하면 예외 처리
    if (dtID.x >= g_iNumInstances)
        return;

    // 해당 스레드가 담당할 파티클 데이터 가져오기
    ParticleData p = g_ParticleBuffer[dtID.x];

    // --- [여기에 파티클 로직 구현] ---
    
    // 1. 수명 업데이트
    p.life += g_fTimeDelta;
    
    // 만약 수명이 다했다면 화면 밖으로 치우거나 비활성화 처리 (수동 관리)
    if (p.life > 60.0f)
    {
        // 예시: 스케일을 0으로 만들거나 멀리 보냄
        if(loop 원상보구)
        p.matWorld._41_42_43 = float3(9999.0f, 9999.0f, 9999.0f);
        g_ParticleBuffer[dtID.x] = p;
        return;
    }

    // 2. 예시: 간단한 Y축 낙하 및 물리 연산 (C++의 Update 루프를 대체)
    // 실제 드롭 아이템의 물리 로직에 맞춰 속도와 위치를 수정하세요.
    p.vVelocity.y -= 9.8f * g_fTimeDelta; // 중력 적용
    
    // 월드 행렬의 4행(_41, _42, _43)은 위치 좌표(Translation)를 뜻합니다.
    p.matWorld._41_42_43 += p.vVelocity * g_fTimeDelta;

    // ----------------------------------

    // 계산이 끝난 데이터를 다시 GPU 구조화 버퍼에 덮어씁니다.
    g_ParticleBuffer[dtID.x] = p;
}
// 글로벌 뷰/프로젝션 행렬 상수버퍼 (기존 엔진에서 전역 바인딩해주는 가상 버퍼)
cbuffer CB_Transform : register(b1)
{
    float4x4 g_matView;
    float4x4 g_matProj;
};

// 위에서 정의한 파티클 구조체와 동일해야 합니다.
struct ParticleData
{
    float4x4 matWorld;
    float4 vColor;
    float4 light;
    uint4 texIndexs;
    float life;
    float3 vVelocity;
};

// C++에서 0번 슬롯에 묶은 SRV 읽기 통로 (pContext->VSSetShaderResources(0, 1, ...))
StructuredBuffer<ParticleData> g_RenderBuffer : register(t0);

// 텍스처 및 샘플러 세팅
Texture2DArray g_TextureArray : register(t1);
SamplerState g_LinearSampler : register(s0);

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float4 vColor : COLOR0;
    float4 light : TEXCOORD1;
};

VS_OUT VS_MAIN(uint vID : SV_VertexID, uint instID : SV_InstanceID)
{
    VS_OUT Out = (VS_OUT) 0;

    // 1. 번호표(instID)를 사용해 구조화 뷔페 테이블에서 해당 파티클 데이터 쏙 빼오기!
    ParticleData p = g_RenderBuffer[instID];

    // 2. SV_VertexID(0~3)에 따라 사각형의 네 모서리 UV 좌표 가상 계산
    // vID = 0 -> (0, 1) [좌하]
    // vID = 1 -> (1, 1) [우하]
    // vID = 2 -> (0, 0) [좌상]
    // vID = 3 -> (1, 0) [우상]
    float2 uv = float2(vID % 2, 1 - (vID / 2));
    Out.vTexcoord = uv;

    // 3. 중심점을 기준으로 사각형 정점 위치 만들기 (-0.5 ~ 0.5 크기의 빌보드 사각형)
    float4 vLocalPos = float4((uv.x - 0.5f), (uv.y - 0.5f), 0.0f, 1.0f);

    // 4. 공간 변환 (Local -> World -> View -> Projection)
    float4 vWorldPos = mul(vLocalPos, p.matWorld);
    float4 vViewPos = mul(vWorldPos, g_matView);
    Out.vPosition = mul(vViewPos, g_matProj);

    // 5. 픽셀 셰이더로 보낼 부가 정보 패스
    Out.vColor = p.vColor;
    Out.light = p.light;

    return Out;
}

float4 PS_MAIN(VS_OUT In) : SV_TARGET
{
    // 텍스처 배열에서 이미지 샘플링 (예시로 0번 레이어 사용, 인덱스는 p.texIndexs 활용 가능)
    float4 vTexColor =g_TextureArray .Sample(g_LinearSampler, float3(In.vTexcoord, 0));
    
    // 틴트 컬러 및 조명 값 연산
    float4 vFinalColor = vTexColor * In.vColor * In.light;
    
    // 투명도 컷아웃(알파 테스트) 처리 필요 시
    if (vFinalColor.a < 0.1f)
        discard;

    return vFinalColor;
}