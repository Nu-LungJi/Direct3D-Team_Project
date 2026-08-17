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


StructuredBuffer<GPU_ANIM_INSTANCE_DATA> g_AnimationInstances : register(t6);
StructuredBuffer<float4x4> g_FinalBoneMatrices : register(t7);

struct GPU_SKIN_BONE_DESC
{
    float4x4 OffsetMatrix;
    uint iSkeletonBoneIndex;
    uint iPadding0;
    uint iPadding1;
    uint iPadding2;
};

StructuredBuffer<GPU_SKIN_BONE_DESC> gSkinBones : register(t8);

struct GPU_MORPH_VERTEX_DELTA
{
    float4 vPositionDelta;
    float4 vNormalDelta;
    float4 vTangentDelta;
    float4 vBinormalDelta;
};
StructuredBuffer<GPU_MORPH_VERTEX_DELTA> gMorphDeltas : register(t9);

cbuffer CB_GPU_SKIN_MESH : register(b5)
{
    uint gSkinBoneOffset;
    uint gVertexCount;
    uint gSkinBoneCount;
    uint gBonePaletteStride;
    uint gMorphTargetCount;
    uint gMorphVertexCount;
    uint2 gMorphPadding;
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

VS_OUT VSMain(VS_IN In, uint instanceId : SV_InstanceID, uint vertexId : SV_VertexID)
{
    VS_OUT Out;
    if (gMorphTargetCount > 0 && vertexId < gMorphVertexCount)
    {
        [unroll]
        for (uint slot = 0; slot < 4; ++slot)
        {
            const float weight = g_AnimationInstances[instanceId].vMorphWeights[slot];
            const uint targetIndex = g_AnimationInstances[instanceId].vMorphIndices[slot];
            if (abs(weight) <= 1.e-4f || targetIndex >= gMorphTargetCount)
                continue;
            const GPU_MORPH_VERTEX_DELTA delta =
                gMorphDeltas[targetIndex * gMorphVertexCount + vertexId];
            In.vPosition += delta.vPositionDelta.xyz * weight;
            In.vNormal += delta.vNormalDelta.xyz * weight;
            In.vTangent += delta.vTangentDelta.xyz * weight;
            In.vBinormal += delta.vBinormalDelta.xyz * weight;
        }
    }
    float fWeightW = 1.f - (In.vBlendWeights.x + In.vBlendWeights.y + In.vBlendWeights.z);
    uint boneOffset = instanceId * 512;
    GPU_SKIN_BONE_DESC skinBoneX = gSkinBones[gSkinBoneOffset + In.vBlendIndices.x];
    GPU_SKIN_BONE_DESC skinBoneY = gSkinBones[gSkinBoneOffset + In.vBlendIndices.y];
    GPU_SKIN_BONE_DESC skinBoneZ = gSkinBones[gSkinBoneOffset + In.vBlendIndices.z];
    GPU_SKIN_BONE_DESC skinBoneW = gSkinBones[gSkinBoneOffset + In.vBlendIndices.w];
    float4x4 BoneMatrix =
        mul(skinBoneX.OffsetMatrix, g_FinalBoneMatrices[boneOffset + skinBoneX.iSkeletonBoneIndex]) * In.vBlendWeights.x +
        mul(skinBoneY.OffsetMatrix, g_FinalBoneMatrices[boneOffset + skinBoneY.iSkeletonBoneIndex]) * In.vBlendWeights.y +
        mul(skinBoneZ.OffsetMatrix, g_FinalBoneMatrices[boneOffset + skinBoneZ.iSkeletonBoneIndex]) * In.vBlendWeights.z +
        mul(skinBoneW.OffsetMatrix, g_FinalBoneMatrices[boneOffset + skinBoneW.iSkeletonBoneIndex]) * fWeightW;

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
