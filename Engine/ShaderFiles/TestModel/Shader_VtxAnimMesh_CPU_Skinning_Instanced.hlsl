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
    uint iMorphTargetIndex;
    float fMorphWeight;
    uint iMorphPadding0;
    uint iMorphPadding1;
};

struct GPU_SKIN_BONE_DESC
{
	float4x4 OffsetMatrix;
	uint iSkeletonBoneIndex;
	uint iPadding0;
	uint iPadding1;
	uint iPadding2;
};

struct MORPH_VERTEX_DELTA
{
    uint iVertexIndex;
    float3 vPositionDelta;
    float3 vNormalDelta;
    float3 vTangentDelta;
    float3 vBinormalDelta;
};

struct GPU_MORPH_TARGET_RANGE
{
    uint iDeltaOffset;
    uint iDeltaCount;
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
	nointerpolation float fDissolveIntensity : TEXCOORD3;
};

StructuredBuffer<GPU_ANIM_INSTANCE_DATA> gInstances : register(t6);

// CPU가 계산한 skeleton CombinedBone palette.
StructuredBuffer<float4x4> gCPUCombinedBoneMatrices : register(t7);

// GPU-only 경로와 동일한 mesh-local bone -> skeleton bone 매핑.
StructuredBuffer<GPU_SKIN_BONE_DESC> gSkinBones : register(t8);

// 모든 Morph Target의 sparse delta를 이어 붙인 배열.
StructuredBuffer<MORPH_VERTEX_DELTA> gMorphDeltas : register(t9);

// Target별로 gMorphDeltas 안에서 사용할 [offset, count] 범위를 보관한다.
StructuredBuffer<GPU_MORPH_TARGET_RANGE> gMorphTargetRanges : register(t10);

cbuffer CB_CPU_SKINNING_MESH : register(b5)
{
    uint gSkinBoneOffset;
    uint gVertexCount;
    uint gSkinBoneCount;
    uint gBonePaletteStride;
    uint gMorphTargetCount;
    uint3 gMorphPadding;
};

bool FindMorphDelta(uint morphTargetIndex, uint vertexId, out MORPH_VERTEX_DELTA morphDelta)
{
    morphDelta = (MORPH_VERTEX_DELTA)0;

    const GPU_MORPH_TARGET_RANGE range = gMorphTargetRanges[morphTargetIndex];
    uint left = range.iDeltaOffset;
    uint right = range.iDeltaOffset + range.iDeltaCount;

    while (left < right)
    {
        const uint middle = left + ((right - left) >> 1);
        const uint middleVertexIndex = gMorphDeltas[middle].iVertexIndex;

        if (middleVertexIndex < vertexId)
            left = middle + 1;
        else
            right = middle;
    }

    const uint rangeEnd = range.iDeltaOffset + range.iDeltaCount;
    if (left >= rangeEnd || gMorphDeltas[left].iVertexIndex != vertexId)
        return false;

    morphDelta = gMorphDeltas[left];
    return true;
}

void ApplyMorph(inout VS_IN input, uint vertexId, uint instanceId)
{
    const GPU_ANIM_INSTANCE_DATA instanceData = gInstances[instanceId];
    if (instanceData.iMorphTargetIndex >= gMorphTargetCount || abs(instanceData.fMorphWeight) <= 0.0001f)
        return;

    MORPH_VERTEX_DELTA morphDelta;
    if (!FindMorphDelta(instanceData.iMorphTargetIndex, vertexId, morphDelta))
        return;

    input.vPosition += morphDelta.vPositionDelta * instanceData.fMorphWeight;
    input.vNormal += morphDelta.vNormalDelta * instanceData.fMorphWeight;
    input.vTangent += morphDelta.vTangentDelta * instanceData.fMorphWeight;
    input.vBinormal += morphDelta.vBinormalDelta * instanceData.fMorphWeight;
}

VS_OUT VSMain(VS_IN input, uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
    const uint instanceBoneOffset = instanceId * 512;

    // Morph는 bind pose 공간의 delta이므로 Bone Skinning보다 먼저 적용한다.
    ApplyMorph(input, vertexId, instanceId);

    const float weightW = 1.f -(input.vBlendWeights.x + input.vBlendWeights.y + input.vBlendWeights.z);

    const GPU_SKIN_BONE_DESC skinBoneX =
        gSkinBones[gSkinBoneOffset + input.vBlendIndices.x];
    const GPU_SKIN_BONE_DESC skinBoneY =
        gSkinBones[gSkinBoneOffset + input.vBlendIndices.y];
    const GPU_SKIN_BONE_DESC skinBoneZ =
        gSkinBones[gSkinBoneOffset + input.vBlendIndices.z];
    const GPU_SKIN_BONE_DESC skinBoneW =
        gSkinBones[gSkinBoneOffset + input.vBlendIndices.w];

    const float4x4 BoneMatrix =mul(skinBoneX.OffsetMatrix, gCPUCombinedBoneMatrices[instanceBoneOffset + skinBoneX.iSkeletonBoneIndex]) * input.vBlendWeights.x +
        mul(skinBoneY.OffsetMatrix, gCPUCombinedBoneMatrices[instanceBoneOffset + skinBoneY.iSkeletonBoneIndex]) * input.vBlendWeights.y +
        mul(skinBoneZ.OffsetMatrix,gCPUCombinedBoneMatrices[instanceBoneOffset + skinBoneZ.iSkeletonBoneIndex]) * input.vBlendWeights.z +
        mul(skinBoneW.OffsetMatrix, gCPUCombinedBoneMatrices[instanceBoneOffset + skinBoneW.iSkeletonBoneIndex]) * weightW;

	const float4 skinnedPosition = mul(float4(input.vPosition, 1.f), BoneMatrix);
	//const float4 skinnedPosition = float4(input.vPosition, 1.f);
	const float4 skinnedNormal = mul(float4(input.vNormal, 0.f), BoneMatrix);
	const float4 skinnedTangent = mul(float4(input.vTangent, 0.f), BoneMatrix);
    const float4 skinnedBinormal = mul(float4(input.vBinormal, 0.f), BoneMatrix);
    const float4x4 worldMatrix = gInstances[instanceId].WorldMatrix;
    const float4 worldPosition = mul(skinnedPosition, worldMatrix);

    VS_OUT output;
    output.vPosition = mul(worldPosition, g_matViewProj);
    output.vNormal = normalize(mul(skinnedNormal, worldMatrix));
    output.vTangent = normalize(mul(skinnedTangent, worldMatrix));
    output.vBinormal = normalize(mul(skinnedBinormal, worldMatrix));
    output.vTexcoord = input.vTexcoord;
    output.vWorldPos = worldPosition;
    output.vProjPos = output.vPosition;
	output.fDissolveIntensity = asfloat(gInstances[instanceId].iFlags);
    return output;
}
