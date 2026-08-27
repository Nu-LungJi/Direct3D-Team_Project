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
};

struct GPU_SKIN_BONE_DESC
{
	float4x4 OffsetMatrix;
	uint iSkeletonBoneIndex;
	uint iPadding0;
	uint iPadding1;
	uint iPadding2;
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

StructuredBuffer<GPU_ANIM_INSTANCE_DATA> gInstances : register(t6);

// CPU가 계산한 skeleton CombinedBone palette.
StructuredBuffer<float4x4> gCPUCombinedBoneMatrices : register(t7);

// GPU-only 경로와 동일한 mesh-local bone -> skeleton bone 매핑.
StructuredBuffer<GPU_SKIN_BONE_DESC> gSkinBones : register(t8);

cbuffer CB_CPU_SKINNING_MESH : register(b5)
{
    uint gSkinBoneOffset;
    uint gVertexCount;
    uint gSkinBoneCount;
    uint gBonePaletteStride;
};

VS_OUT VSMain(VS_IN input, uint instanceId : SV_InstanceID)
{
    const uint instanceBoneOffset = instanceId * 512;

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
    return output;
}
