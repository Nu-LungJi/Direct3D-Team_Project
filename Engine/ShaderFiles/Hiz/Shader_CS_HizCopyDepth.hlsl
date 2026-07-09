
Texture2D<float> g_InputDepth : register(t0); // 원본 depth SRV
RWTexture2D<float> g_OutputMip0 : register(u0); // Mip0 UAV


// 원본depth -> Mip0에 그대로 복사
[numthreads(8, 8, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
    uint2 coord = DTid.xy;
    
    uint width;
    uint height;
    g_OutputMip0.GetDimensions(width, height);
    
    if(coord.x >= width || coord.y >= height)
        return;
    
    float depth = g_InputDepth.Load(uint3(coord, 0));
    g_OutputMip0[coord] = depth;
}
