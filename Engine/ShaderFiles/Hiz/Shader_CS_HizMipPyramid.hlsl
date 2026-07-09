
Texture2D<float> g_InputMip : register(t0);
RWTexture2D<float> g_OutputMip : register(u0);

// MipN -> MipN+1
[numthreads(8, 8, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
    uint2 outCoord = DTid.xy;
    
    uint outWidth;
    uint outHeight;
    g_OutputMip.GetDimensions(outWidth, outHeight);

    if (outCoord.x >= outWidth || outCoord.y >= outHeight)
        return;
    
    
    uint2 inCoord = outCoord * 2;
    
    uint width;
    uint height;
    g_InputMip.GetDimensions(width, height);
    
    uint2 c0 = min(inCoord + uint2(0, 0), uint2(width - 1, height - 1));
    uint2 c1 = min(inCoord + uint2(1, 0), uint2(width - 1, height - 1));
    uint2 c2 = min(inCoord + uint2(0, 1), uint2(width - 1, height - 1));
    uint2 c3 = min(inCoord + uint2(1, 1), uint2(width - 1, height - 1));
    
    float d0 = g_InputMip.Load(uint3(c0, 0));
    float d1 = g_InputMip.Load(uint3(c1, 0));
    float d2 = g_InputMip.Load(uint3(c2, 0));
    float d3 = g_InputMip.Load(uint3(c3, 0));

    g_OutputMip[outCoord] = min(min(d0, d1), min(d2, d3));
}
