#include "./ShaderDefines.hlsl"

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;

    // 인스턴스 스트림 (슬롯1, m_pResInstancedBuffer)
    float4 vInstRow0 : INSTANCE_WORLD0;
    float4 vInstRow1 : INSTANCE_WORLD1;
    float4 vInstRow2 : INSTANCE_WORLD2;
    float4 vInstRow3 : INSTANCE_WORLD3;
    float4 vInstColor : INSTANCE_COLOR;
    float4 vInstEmissive : INSTANCE_EMISSIVE;
    
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float4 vColor : COLOR0;
    float3 vNormal : NORMAL0; // 추가
    float3 vTangent : TANGENT0; // 추가
    float3 vBinormal : BINORMAL0; // 추가
    float4 vEmissive : COLOR1;
};

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;

    matrix matWorld = matrix(In.vInstRow0, In.vInstRow1, In.vInstRow2, In.vInstRow3);
    float4 vWorldPos = mul(float4(In.vPosition, 1.0f), matWorld);

    Out.vPosition = mul(vWorldPos, g_matViewProj);
    Out.vTexcoord = In.vTexcoord;
    Out.vNormal = normalize(mul(In.vNormal, (float3x3) matWorld));
    Out.vTangent = normalize(mul(In.vTangent, (float3x3) matWorld));
    Out.vBinormal = normalize(mul(In.vBinormal, (float3x3) matWorld));
    Out.vColor = In.vInstColor;
    Out.vEmissive = In.vInstEmissive;
    return Out;
}

Texture2D g_DiffuseTexture : register(t1);
Texture2D g_NormalTexture : register(t6);

SamplerState g_LinearSampler : register(s0);

struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;
    float4 vNormal : SV_TARGET1;
    float4 vSMRO : SV_TARGET2;
    float4 vEmissive : SV_TARGET3;

};
PS_OUT PSMain(VS_OUT In)
{
    
    PS_OUT Out = (PS_OUT) 0;

    float4 texColor = g_DiffuseTexture.Sample(g_LinearSampler, In.vTexcoord);
    //if (texColor.a < 0.1f)
    //    discard;

    Out.vDiffuse = texColor * In.vColor;

    // 노멀맵에서 tangent space 노멀을 가져와 world space로 변환
    float3 tangentNormal = g_NormalTexture.Sample(g_LinearSampler, In.vTexcoord).xyz * 2.0f - 1.0f;

    float3x3 TBN = float3x3(normalize(In.vTangent), normalize(In.vBinormal), normalize(In.vNormal));
    float3 worldNormal = normalize(mul(tangentNormal, TBN));

    Out.vNormal = float4(worldNormal * 0.5f + 0.5f, 1.0f); // [-1,1] → [0,1] 인코딩 (다른 셰이더와 인코딩 방식 맞춰야 함)

    Out.vSMRO = float4(0.f, 0.5f, 0.f, 1.f);
    
    Out.vEmissive = float4(texColor.xyz *In.vEmissive.xyz * In.vEmissive.w,1.0f);

    
    return Out;
}