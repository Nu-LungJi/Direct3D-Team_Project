#include "../ShaderDefines.hlsl"

struct VTXANIMMESH
{
    float3 vPosition;
    float3 vNormal;
    float3 vTangent;
    float3 vBinormal;
    float2 vTexcoord;
    uint4 vBlendIndices;
    float4 vBlendWeights;
};

struct SKINNED_VERTEX
{
    float4 vPosition;
    float4 vNormal;
    float4 vTangent;
    float4 vBinormal;
};

struct GPU_SKIN_BONE_DESC
{
    float4x4 OffsetMatrix;
    uint iSkeletonBoneIndex;
    uint iPadding0;
    uint iPadding1;
    uint iPadding2;
};

StructuredBuffer<VTXANIMMESH> gInputVertices : register(t0);
StructuredBuffer<float4x4> gFinalBoneMatrices : register(t1);
StructuredBuffer<GPU_SKIN_BONE_DESC> gSkinBones : register(t2);
RWStructuredBuffer<SKINNED_VERTEX> gOutputVertices : register(u0);

cbuffer CB_GPU_SKIN_MESH : register(b5)
{
    uint gSkinBoneOffset;
    uint gVertexCount;
    uint2 gSkinMeshPadding;
};

[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint vertexCount;
    uint vertexStride;
    gInputVertices.GetDimensions(vertexCount, vertexStride);

    const uint vertexIndex = dispatchThreadId.x;
    const uint instanceIndex = dispatchThreadId.y;
    if (vertexIndex >= vertexCount)
        return;

    const VTXANIMMESH input = gInputVertices[vertexIndex];
    // GPU-only VS와 동일한 weight 계산을 사용한다.
    const float weightW = 1.f -
        (input.vBlendWeights.x + input.vBlendWeights.y + input.vBlendWeights.z);
    const uint boneOffset = instanceIndex * 512;
    const GPU_SKIN_BONE_DESC skinBoneX = gSkinBones[gSkinBoneOffset + input.vBlendIndices.x];
    const GPU_SKIN_BONE_DESC skinBoneY = gSkinBones[gSkinBoneOffset + input.vBlendIndices.y];
    const GPU_SKIN_BONE_DESC skinBoneZ = gSkinBones[gSkinBoneOffset + input.vBlendIndices.z];
    const GPU_SKIN_BONE_DESC skinBoneW = gSkinBones[gSkinBoneOffset + input.vBlendIndices.w];

    const float4x4 skinMatrix =
        mul(skinBoneX.OffsetMatrix, gFinalBoneMatrices[boneOffset + skinBoneX.iSkeletonBoneIndex]) * input.vBlendWeights.x +
        mul(skinBoneY.OffsetMatrix, gFinalBoneMatrices[boneOffset + skinBoneY.iSkeletonBoneIndex]) * input.vBlendWeights.y +
        mul(skinBoneZ.OffsetMatrix, gFinalBoneMatrices[boneOffset + skinBoneZ.iSkeletonBoneIndex]) * input.vBlendWeights.z +
        mul(skinBoneW.OffsetMatrix, gFinalBoneMatrices[boneOffset + skinBoneW.iSkeletonBoneIndex]) * weightW;

    SKINNED_VERTEX output;
    output.vPosition = mul(float4(input.vPosition, 1.f), skinMatrix);
    output.vNormal = mul(float4(input.vNormal, 0.f), skinMatrix);
    output.vTangent = mul(float4(input.vTangent, 0.f), skinMatrix);
    output.vBinormal = mul(float4(input.vBinormal, 0.f), skinMatrix);
    gOutputVertices[instanceIndex * vertexCount + vertexIndex] = output;
}
