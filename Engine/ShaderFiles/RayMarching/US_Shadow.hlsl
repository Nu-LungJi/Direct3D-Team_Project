#include "../ShaderHeader/SH_CommonFunction.hlsli"

struct VS_IN
{
    float3 Position : POSITION;
};

struct VS_OUT
{
    float4 WorldPos : POSITION;
};

VS_OUT VSMain(VS_IN IN)
{
    VS_OUT OUT;
    OUT.WorldPos = mul(float4(IN.Position, 1.0f), g_matWorld);
    return OUT;
}

struct GS_OUT
{
    float4  Position    : SV_POSITION;
    float3  WorldPos    : POSITION;
    uint    LayerIndex  : SV_RenderTargetArrayIndex;
};

[maxvertexcount(18)]
void GSMain(triangle VS_OUT IN[3], inout TriangleStream<GS_OUT> _OutStream)
{
    DynamicLight DLight = AffectedLight[CurrentLightIndex];
    
    for (int Face = 0; Face < 6; ++Face)
    {
        GS_OUT OUT;
        OUT.LayerIndex = Face;
        for (int v = 0; v < 3; ++v)
        {
            OUT.Position = mul(IN[v].WorldPos, DLight.g_LightViewProj[Face]);
            OUT.WorldPos = IN[v].WorldPos.xyz;
            _OutStream.Append(OUT);
        }
        
        _OutStream.RestartStrip();
    }
}

float PSMain(GS_OUT OUT) : SV_DEPTH
{
    DynamicLight Light = AffectedLight[CurrentLightIndex];
    
    float3 LightToPixel = OUT.WorldPos.xyz - Light.Position;
    float Distance = length(LightToPixel);
    
    return Distance / Light.LightRange;
}
