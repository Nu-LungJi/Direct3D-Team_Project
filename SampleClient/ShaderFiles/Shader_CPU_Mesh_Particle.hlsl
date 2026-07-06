#include "./ShaderDefines.hlsl"

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;

    // 인스턴스 스트림 (슬롯1, m_pResInstancedBuffer)
    float4 vInstRow0 : INSTANCE_WORLD0;
    float4 vInstRow1 : INSTANCE_WORLD1;
    float4 vInstRow2 : INSTANCE_WORLD2;
    float4 vInstRow3 : INSTANCE_WORLD3;
    float4 vInstColor : INSTANCE_COLOR;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float4 vColor : COLOR0;
};

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;

    matrix matWorld = matrix(In.vInstRow0, In.vInstRow1, In.vInstRow2, In.vInstRow3);
    float4 vWorldPos = mul(float4(In.vPosition, 1.0f), matWorld);

    Out.vPosition = mul(vWorldPos, g_matViewProj);
    Out.vTexcoord = In.vTexcoord;
    Out.vColor = In.vInstColor;

    return Out;
}

Texture2D g_DiffuseTexture : register(t1);
SamplerState g_LinearSampler : register(s0);

float4 PSMain(VS_OUT In) : SV_TARGET
{
    float4 vTexColor = g_DiffuseTexture.Sample(g_LinearSampler, In.vTexcoord);
    float4 vFinalColor = vTexColor * In.vColor;

    if (vFinalColor.a < 0.1f)
        discard;
    
    
    return vFinalColor;
}