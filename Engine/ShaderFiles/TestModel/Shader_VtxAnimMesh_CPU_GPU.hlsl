#include "../ShaderDefines.hlsl"

struct GPU_ANIM_INSTANCE_DATA
{
    float4x4 WorldMatrix;
    uint iAnimIndex;
    uint iFlags;
    float fTrackPosition;
    uint RootBoneIndex;
    uint iPrevAnimIndex;
    float fPrevTrackPosition;
    float fBlendWeight;
    uint bBlending;
    uint4 vMorphIndices;
    float4 vMorphWeights;
};

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

struct SKINNED_VERTEX
{
    float4 vPosition;
    float4 vNormal;
    float4 vTangent;
    float4 vBinormal;
};

StructuredBuffer<GPU_ANIM_INSTANCE_DATA> gAnimationInstances : register(t6);
StructuredBuffer<SKINNED_VERTEX> gSkinnedVertices : register(t7);

cbuffer CB_CPU_GPU_SKINNING : register(b5)
{
    uint gSkinBoneOffset;
    uint gVertexCount;
    uint2 gPadding;
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

VS_OUT VSMain(VS_IN input, uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
    const SKINNED_VERTEX skinned = gSkinnedVertices[instanceId * gVertexCount + vertexId];
    const float4x4 worldMatrix = gAnimationInstances[instanceId].WorldMatrix;
    VS_OUT output;
    const float4 worldPosition = mul(skinned.vPosition, worldMatrix);
    output.vPosition = mul(worldPosition, g_matViewProj);
    output.vNormal = normalize(mul(skinned.vNormal, worldMatrix));
    output.vTangent = normalize(mul(skinned.vTangent, worldMatrix));
    output.vBinormal = normalize(mul(skinned.vBinormal, worldMatrix));
    output.vTexcoord = input.vTexcoord;
    output.vWorldPos = worldPosition;
    output.vProjPos = output.vPosition;
    return output;
}
