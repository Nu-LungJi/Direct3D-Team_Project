#include "../../Engine/ShaderFiles/ShaderDefines.hlsl"

struct ParticleData
{
    float3 position;
    float pad1;
    float3 velocity;
    float life;
    float maxLife;
    float size;
    uint alive;
    uint loop;
    float4 color;
    float4 emissive;
};

StructuredBuffer<ParticleData> g_RenderBuffer : register(t0);
Texture2D g_Texture : register(t1);
//SamplerState g_LinearSampler : register(s0);

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float4 vColor : COLOR0;
    float4 vEmissive : COLOR1;
};

// Vertex Shader
VS_OUT VSMain(uint vID : SV_VertexID, uint instID : SV_InstanceID)
{
    VS_OUT Out = (VS_OUT) 0;

    ParticleData p = g_RenderBuffer[instID];
    if (!p.alive)
    {
        p.color.a = 0.f;
    }
    // 가상 UV 생성 (0~3번 정점)
    float2 uv = float2(vID % 2, 1 - (vID / 2));
    Out.vTexcoord = uv;

    // 빌보드 스타일의 로컬 사각형 크기 조절 (C++에서 넘겨받은 p.size 사용!)
    float3 vLocalPos = float3((uv.x - 0.5f) * p.size, (uv.y - 0.5f) * p.size, 0.0f);

    // 행렬 없이 월드 좌표 복사: 로컬 좌표 + 파티클의 실시간 월드 위치(p.position)
    float4 vWorldPos = float4(vLocalPos + p.position, 1.0f);

    // 뷰 및 프로젝션 공간 변환
    float4 vViewPos = mul(vWorldPos, g_matView);
    Out.vPosition = mul(vViewPos, g_matProj);

    // C++에서 세팅한 주황색 불꽃 컬러 전송
    Out.vColor = p.color;
    Out.vEmissive = p.emissive;

    return Out;
}

struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;
};
// Pixel Shader
PS_OUT PSMain(VS_OUT In)
{
    
    PS_OUT Out = (PS_OUT) 0;

    // 텍스처 샘플링 (정확한 변수명 vTextureColor로 통일)
    float4 vTextureColor = g_Texture.Sample(LinearWrap, In.vTexcoord);
    
    // 알파 테스트 혹은 특정 채널 기준 discard (여기서는 투명도나 특정 값 기준으로 처리)
    if (vTextureColor.a <= 0.05f)
        discard;
    if (vTextureColor.x < 0.4f)
    {
        discard;
    }
    float4 vFinalColor = In.vColor;

    // 1. Diffuse (알베도/표면 색상)
    Out.vDiffuse = vFinalColor;


    // 이펙트가 PBR 라이팅 단계에서 빛나게 하려면 에미시브에 색상 값을 강하게 넣어줍니다.
    Out.vDiffuse = float4(vFinalColor.xyz + In.vEmissive.xyz * In.vEmissive.w, 1.f);
    
    
  
    return Out;
}
