StructuredBuffer<uint> gBatchVisibleCounts : register(t0);
StructuredBuffer<uint> gDrawBatchIndices : register(t1);
RWByteAddressBuffer gIndirectArgs : register(u0);

cbuffer CB_BuildIndirectArgs : register(b0)
{
    uint gDrawCount;
    uint3 gPadding;
};

[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint drawIndex = dispatchThreadID.x;
    if (drawIndex >= gDrawCount)
        return;

    uint argsByteOffset = drawIndex * 20;
    gIndirectArgs.Store(argsByteOffset + 4, gBatchVisibleCounts[gDrawBatchIndices[drawIndex]]);
}
