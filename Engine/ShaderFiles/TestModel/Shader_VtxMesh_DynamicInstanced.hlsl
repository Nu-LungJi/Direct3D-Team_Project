#include "../ShaderHeader/SH_CommonFunction.hlsli"

// CModel_Instance_Manager가 사용하는 CPU/GPU 공용 인스턴스 구조와 동일한 레이아웃이다.
struct GPU_ANIM_INSTANCE_DATA
{
    float4x4 WorldMatrix;
    uint iAnimIndex;
    uint iFlags;
    float fTrackPosition;
    uint iRootBoneIndex;
    uint iPrevAnimIndex;
    float fPrevTrackPosition;
    float fBlendWeight;
    uint bBlending;
};

StructuredBuffer<GPU_ANIM_INSTANCE_DATA> gInstances : register(t6);

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float4 vTangent : TANGENT;
    float4 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
};

VS_OUT VSMain(VS_IN input, uint instanceId : SV_InstanceID)
{
    VS_OUT output;
    const float4x4 world = gInstances[instanceId].WorldMatrix;
    const float4x4 worldView = mul(world, g_matView);
    const float4x4 worldViewProjection = mul(worldView, g_matProj);

    output.vPosition = mul(float4(input.vPosition, 1.f), worldViewProjection);
    output.vNormal = normalize(mul(float4(input.vNormal, 0.f), world));
    output.vTangent = normalize(mul(float4(input.vTangent, 0.f), world));
    output.vBinormal = normalize(mul(float4(input.vBinormal, 0.f), world));
    output.vTexcoord = input.vTexcoord;
    output.vWorldPos = mul(float4(input.vPosition, 1.f), world);
    output.vProjPos = output.vPosition;
    return output;
}
