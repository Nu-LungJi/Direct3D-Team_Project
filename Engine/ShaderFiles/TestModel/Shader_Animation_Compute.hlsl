struct GPU_BONE_DESC { float4x4 BindLocalMatrix; int iParentBoneIndex; uint iDepth; uint iPadding0; uint iPadding1; };
struct GPU_ANIM_DESC { uint iChannelOffset; uint iChannelCount; uint iBoneChannelMapOffset; uint iBoneCount; float fDuration; float3 Padding; };
struct GPU_CHANNEL_DESC { uint iBoneIndex; uint iKeyFrameOffset; uint iKeyFrameCount; uint Padding; };
struct GPU_KEYFRAME_DESC { float3 vScale; float fTrackPosition; float4 vRotation; float3 vTranslation; float Padding; };
struct GPU_ANIM_INSTANCE_DATA { float4x4 WorldMatrix; uint iAnimIndex; uint iFlags; float fTrackPosition; float fPadding; };

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

float4x4 SampleLocal(uint boneIndex, GPU_ANIM_DESC animation, float time)
{
    uint channelIndex = gBoneChannelMap[animation.iBoneChannelMapOffset+boneIndex];
    if(channelIndex==0xffffffff)
        return gBones[boneIndex].BindLocalMatrix;

    GPU_CHANNEL_DESC channel = gChannels[channelIndex];

    if(channel.iKeyFrameCount==0)
        return gBones[boneIndex].BindLocalMatrix;

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
    float3 scale = lerp(a.vScale,b.vScale,t);
    // FXC does not provide an HLSL slerp intrinsic.  Use shortest-path NLERP.
    float4 rotationB = b.vRotation;
    if (dot(a.vRotation, rotationB) < 0.0f)
        rotationB = -rotationB;
    float4x4 result = mul(float4x4(scale.x,0,0,0, 0,scale.y,0,0, 0,0,scale.z,0, 0,0,0,1),QuaternionMatrix(normalize(lerp(a.vRotation,rotationB,t))));
    result[3] = float4(lerp(a.vTranslation,b.vTranslation,t),1);

    return result;
}

[numthreads(512,1,1)]
void CSMain(uint3 groupId:SV_GroupID,uint3 threadId:SV_GroupThreadID)
{
    uint instanceIndex = groupId.x, boneIndex=threadId.x, outputIndex = instanceIndex * 512 + boneIndex;

    GPU_ANIM_INSTANCE_DATA instance = gInstances[instanceIndex];

    GPU_ANIM_DESC animation = gAnimations[instance.iAnimIndex];
    // Bone의 개수가 넘어 갔을 경우 512개
    if ( boneIndex >= animation.iBoneCount){
        gFinalBoneMatrices[outputIndex] = float4x4(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);
        return;
    }

    float4x4 combined = SampleLocal(boneIndex,animation,instance.fTrackPosition);

    int parentIndex = gBones[boneIndex].iParentBoneIndex;
    while(parentIndex>=0){
        combined=mul(combined,SampleLocal((uint)parentIndex,animation,instance.fTrackPosition));
        parentIndex=gBones[parentIndex].iParentBoneIndex;
    }

    gFinalBoneMatrices[outputIndex]=combined;
}
