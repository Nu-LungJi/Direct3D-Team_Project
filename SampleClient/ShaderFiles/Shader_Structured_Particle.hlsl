// ==========================================
// 1. 카메라 뷰 / 프로젝션 행렬 상수버퍼
// ==========================================
cbuffer CB_Transform : register(b1)
{
    float4x4 g_matView;
    float4x4 g_matProj;
};

// ==========================================
// 2. 파티클 구조체 정의 (Compute Shader의 구조와 완벽히 일치해야 함)
// ==========================================
struct ParticleData
{
    float4x4 matWorld;
    float4 vColor;
    float4 light;
    uint4 texIndexs;
    float life;
    float3 vVelocity;
};

// C++에서 Render 시점에 묶는 읽기 통로 (SRV - t0 슬롯)
StructuredBuffer<ParticleData> g_RenderBuffer : register(t0);

// 텍스처 및 샘플러 자원
Texture2D g_Texture : register(t1);
SamplerState g_LinearSampler : register(s0);

// ==========================================
// 3. 파이프라인 입출력 구조체
// ==========================================
struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float4 vColor : COLOR0;
    float4 light : TEXCOORD1;
};

// ==========================================
// 4. Vertex Shader (정점 생성 및 공간 변환)
// ==========================================
VS_OUT VSMain(uint vID : SV_VertexID, uint instID : SV_InstanceID)
{
    VS_OUT Out = (VS_OUT) 0;

    //  Compute Shader가 계산해 둔 버퍼에서 내 인덱스(instID) 데이터를 쏙 빼옴!
    ParticleData p = g_RenderBuffer[instID];

    // SV_VertexID(0~3) 정점 번호를 사용해 사각형의 네 모서리 UV 좌표 가상 계산
    float2 uv = float2(vID % 2, 1 - (vID / 2));
    Out.vTexcoord = uv;

    // 중심점을 기준으로 크기 가공 (-0.5 ~ 0.5 크기의 로컬 사각형)
    float4 vLocalPos = float4((uv.x - 0.5f), (uv.y - 0.5f), 0.0f, 1.0f);

    // Compute가 연산해준 matWorld 행렬을 곱해 월드로 보낸 뒤, 카메라 화면 공간으로 변환
    float4 vWorldPos = mul(vLocalPos, p.matWorld);
    float4 vViewPos = mul(vWorldPos, g_matView);
    Out.vPosition = mul(vViewPos, g_matProj);

    // 픽셀 셰이더로 전달할 가공된 정보들
    Out.vColor = p.vColor;
    Out.light = p.light;

    return Out;
}

// ==========================================
// 5. Pixel Shader (색상 출력)
// ==========================================
float4 PSMain(VS_OUT In) : SV_TARGET
{
    // 텍스처 배열에서 이미지 샘플링 (0번 레이어 기본 사용)
    vector vMtrlDiffuse = g_Texture.Sample(g_LinearSampler, In.vTexcoord);
    
    // 틴트 컬러 및 조명 값 최종 연산
    float4 vFinalColor = vMtrlDiffuse * In.vColor * In.light;
    
    // 알파 테스트 (구멍 뚫린 파티클 표현용)
    if (vFinalColor.a < 0.1f)
        discard;

    return vFinalColor;
}