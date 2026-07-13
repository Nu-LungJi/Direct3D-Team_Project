#include "../ShaderDefines.hlsl"

struct GPU_ANIM_INSTANCE_DATA
{
    float4x4 WorldMatrix;
    uint iAnimIndex;
    uint iFlags;
    float fTrackPosition;
    float fPadding;
};

StructuredBuffer<GPU_ANIM_INSTANCE_DATA> g_AnimationInstances : register(t6);
StructuredBuffer<float4x4> g_FinalBoneMatrices : register(t7);

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
    uint4 vBlendIndices : BLENDINDICES;
    float4 vBlendWeights : BLENDWEIGHT;
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

VS_OUT VSMain(VS_IN In, uint instanceId : SV_InstanceID)
{
    VS_OUT Out;
    float fWeightW = 1.f - (In.vBlendWeights.x + In.vBlendWeights.y + In.vBlendWeights.z);
    uint boneOffset = instanceId * 512;
    float4x4 BoneMatrix =
        g_FinalBoneMatrices[boneOffset + In.vBlendIndices.x] * In.vBlendWeights.x +
        g_FinalBoneMatrices[boneOffset + In.vBlendIndices.y] * In.vBlendWeights.y +
        g_FinalBoneMatrices[boneOffset + In.vBlendIndices.z] * In.vBlendWeights.z +
        g_FinalBoneMatrices[boneOffset + In.vBlendIndices.w] * fWeightW;

    float4 vSkinnedPosition = mul(float4(In.vPosition, 1.f), BoneMatrix);
    float4 vSkinnedNormal = mul(float4(In.vNormal, 0.f), BoneMatrix);
    float4x4 worldMatrix = g_AnimationInstances[instanceId].WorldMatrix;
    float4 vWorldPosition = mul(vSkinnedPosition, worldMatrix);

    Out.vPosition = mul(vWorldPosition, g_matViewProj);
    Out.vNormal = normalize(mul(vSkinnedNormal, worldMatrix));
    Out.vTangent = normalize(mul(float4(In.vTangent, 0.f), worldMatrix));
    Out.vBinormal = normalize(mul(float4(In.vBinormal, 0.f), worldMatrix));
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = vWorldPosition;
    Out.vProjPos = Out.vPosition;
    return Out;
}
