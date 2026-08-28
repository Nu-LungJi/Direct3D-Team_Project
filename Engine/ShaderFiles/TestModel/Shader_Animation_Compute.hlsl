struct GPU_BONE_DESC { float4x4 BindLocalMatrix; float3 BindScale; float4 BindRotation; float3 BindTranslation; float fBindPadding; int iParentBoneIndex; uint iDepth; uint iPadding0; uint iPadding1; };
struct GPU_ANIM_DESC { float4x4 PreTransformMatrix; uint iChannelOffset; uint iChannelCount; uint iBoneChannelMapOffset; uint iBoneCount; float fDuration; float3 Padding; };
struct GPU_CHANNEL_DESC { uint iBoneIndex; uint iKeyFrameOffset; uint iKeyFrameCount; uint Padding; };
struct GPU_KEYFRAME_DESC { float3 vScale; float fTrackPosition; float4 vRotation; float3 vTranslation; float Padding; };
struct GPU_ANIM_INSTANCE_DATA { float4x4 WorldMatrix; uint iAnimIndex; uint iFlags; float fTrackPosition; uint RootBoneIndex; uint iPrevAnimIndex; float fPrevTrackPosition; float fBlendWeight; uint bBlending; uint iMorphTargetIndex; float fMorphWeight; uint iMorphPadding0; uint iMorphPadding1; };

StructuredBuffer<GPU_BONE_DESC> gBones : register(t0);
StructuredBuffer<GPU_ANIM_DESC> gAnimations : register(t1);
StructuredBuffer<GPU_CHANNEL_DESC> gChannels : register(t2);
StructuredBuffer<GPU_KEYFRAME_DESC> gKeyFrames : register(t3);
StructuredBuffer<uint> gBoneChannelMap : register(t4);
StructuredBuffer<uint> gUnusedSkinBones : register(t5);
StructuredBuffer<GPU_ANIM_INSTANCE_DATA> gInstances : register(t6);
RWStructuredBuffer<float4x4> gFinalBoneMatrices : register(u0);

float4x4 QuaternionMatrix(float4 q)
{
    float x2 = q.x + q.x, y2 = q.y + q.y, z2 = q.z + q.z;
    float xx = q.x * x2, xy = q.x * y2, xz = q.x * z2, yy = q.y * y2, yz = q.y * z2, zz = q.z * z2;
    float wx = q.w * x2, wy = q.w * y2, wz = q.w * z2;
    return float4x4( 1-yy-zz, xy+wz , xz-wy , 0 , 
                    xy-wz , 1-xx-zz , yz+wx , 0 , 
                    xz+wy , yz-wx , 1-xx-yy , 0 ,
                    0,      0,      0,      1);
}

struct LOCAL_POSE
{
    float3 vScale;
    float4 vRotation;
    float3 vTranslation;
};

float4x4 MakeLocalMatrix(
    LOCAL_POSE pose,
    uint boneIndex,
    GPU_ANIM_DESC animation)
{
    float4x4 result = mul(float4x4(pose.vScale.x, 0, 0, 0, 0, pose.vScale.y, 0, 0, 0, 0, pose.vScale.z, 0, 0, 0, 0, 1),QuaternionMatrix(normalize(pose.vRotation)));
    result[3] = float4(pose.vTranslation, 1);
    
    if (gBones[boneIndex].iParentBoneIndex < 0)
    {
        result = mul(result, animation.PreTransformMatrix);
    }

    
    return result;
}

LOCAL_POSE SampleLocalPose(uint boneIndex, uint RootBoneIndex, GPU_ANIM_DESC animation, float time)
{
    LOCAL_POSE result;
    uint channelIndex = gBoneChannelMap[animation.iBoneChannelMapOffset+boneIndex];
    if(channelIndex==0xffffffff)
    {
        result.vScale = gBones[boneIndex].BindScale;
        result.vRotation = gBones[boneIndex].BindRotation;
        result.vTranslation = gBones[boneIndex].BindTranslation;
        return result;
    }

    GPU_CHANNEL_DESC channel = gChannels[channelIndex];

    if(channel.iKeyFrameCount==0)
    {
        result.vScale = gBones[boneIndex].BindScale;
        result.vRotation = gBones[boneIndex].BindRotation;
        result.vTranslation = gBones[boneIndex].BindTranslation;
        return result;
    }

    uint keyIndex=channel.iKeyFrameCount-1;

    for (uint i = 0; i + 1 < channel.iKeyFrameCount; ++i)
    {
        if (time < gKeyFrames[channel.iKeyFrameOffset + i + 1].fTrackPosition)
        {
            keyIndex = i;
            break;
        }
    }


    GPU_KEYFRAME_DESC a = gKeyFrames[channel.iKeyFrameOffset+keyIndex];
    GPU_KEYFRAME_DESC b = gKeyFrames[channel.iKeyFrameOffset+min(keyIndex+1,channel.iKeyFrameCount-1)];

    float t = saturate((time-a.fTrackPosition)/max(b.fTrackPosition-a.fTrackPosition,0.00001));
    result.vScale = lerp(a.vScale,b.vScale,t);

    float4 rotationB = b.vRotation;
    if (dot(a.vRotation, rotationB) < 0.0f)
        rotationB = -rotationB;
    result.vRotation = normalize(lerp(a.vRotation,rotationB,t));
    result.vTranslation = lerp(a.vTranslation,b.vTranslation,t);
    if (boneIndex == RootBoneIndex)
    {
		result.vTranslation = 0.0f;

		float4 yaw = normalize(float4(0.0f, result.vRotation.y, 0.0f, result.vRotation.w));

		 // yaw의 역쿼터니언
		float4 invYaw = float4(-yaw.xyz, yaw.w);

		 // q * inverse(yaw)
		float4 q = result.vRotation;
		result.vRotation = normalize(float4(
        q.w * invYaw.x + q.x * invYaw.w + q.y * invYaw.z - q.z * invYaw.y,
        q.w * invYaw.y - q.x * invYaw.z + q.y * invYaw.w + q.z * invYaw.x,
        q.w * invYaw.z + q.x * invYaw.y - q.y * invYaw.x + q.z * invYaw.w,
        q.w * invYaw.w - q.x * invYaw.x - q.y * invYaw.y - q.z * invYaw.z
		));

	}
    return result;
}

LOCAL_POSE BlendPose(LOCAL_POSE a, LOCAL_POSE b, float weight)
{
    LOCAL_POSE result;
    result.vScale = lerp(a.vScale, b.vScale, weight);
    float4 rotationB = b.vRotation;
    if (dot(a.vRotation, rotationB) < 0.0f)
        rotationB = -rotationB;
    result.vRotation = normalize(lerp(a.vRotation, rotationB, weight));
    result.vTranslation = lerp(a.vTranslation, b.vTranslation, weight);
    return result;
}

[numthreads(512,1,1)]
void CSMain(uint3 groupId:SV_GroupID,uint3 threadId:SV_GroupThreadID)
{
    uint instanceIndex = groupId.x, boneIndex=threadId.x, outputIndex = instanceIndex * 512 + boneIndex;

    GPU_ANIM_INSTANCE_DATA instance = gInstances[instanceIndex];

    GPU_ANIM_DESC animation = gAnimations[instance.iAnimIndex];
    // Bone의 개수가 넘어 갔을 경우 512개
    if (boneIndex >= animation.iBoneCount)
    {
        gFinalBoneMatrices[outputIndex] = float4x4(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);
        return;
    }

    LOCAL_POSE localPose = SampleLocalPose(boneIndex, instance.RootBoneIndex, animation, instance.fTrackPosition);
    if (instance.bBlending != 0)
    {
        GPU_ANIM_DESC previousAnimation = gAnimations[instance.iPrevAnimIndex];
        LOCAL_POSE previousPose = SampleLocalPose(boneIndex, instance.RootBoneIndex, previousAnimation, instance.fPrevTrackPosition);
        localPose = BlendPose(previousPose, localPose, saturate(instance.fBlendWeight));
    }
    int parentIndex = gBones[boneIndex].iParentBoneIndex;
    float4x4 local = MakeLocalMatrix(localPose, boneIndex, animation);

    float4x4 combined = local;


    while(parentIndex>=0){
        LOCAL_POSE parentPose = SampleLocalPose((uint) parentIndex, instance.RootBoneIndex, animation, instance.fTrackPosition);
        if (instance.bBlending != 0)
        {
            GPU_ANIM_DESC previousAnimation = gAnimations[instance.iPrevAnimIndex];
            LOCAL_POSE previousParentPose = SampleLocalPose((uint) parentIndex, instance.RootBoneIndex, previousAnimation, instance.fPrevTrackPosition);
            parentPose = BlendPose(previousParentPose, parentPose, saturate(instance.fBlendWeight));
        }
        float4x4 parentLocal =
            MakeLocalMatrix(parentPose, (uint) parentIndex, animation);
        combined = mul(combined, parentLocal);
        parentIndex = gBones[parentIndex].iParentBoneIndex;
    }
    
   
    gFinalBoneMatrices[outputIndex]=combined;
}
