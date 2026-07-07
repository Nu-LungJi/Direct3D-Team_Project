#include "../ShaderDefines.hlsl"
struct VS_IN
{
    float3 Pos : POSITION;
    float4 Color : COLOR;
};

struct VS_OUT
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

VS_OUT VSMain(VS_IN vin)
{
    VS_OUT vout;
    vout.PosH = mul(float4(vin.Pos, 1.f), g_matViewProj);
    vout.Color = vin.Color;
    return vout;
}

struct PS_IN
{
    float4 PosH : SV_Position;
    float4 Color : COLOR;
};

float4 PSMain(PS_IN pin): SV_Target
{
    return pin.Color;
}