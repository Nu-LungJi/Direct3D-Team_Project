struct VS_IN
{
    float3 pos : POSITION; // (-1~1)
    float2 uv : TEXCOORD;
};

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};


Texture2D gTex : register(t0);
SamplerState samLinear : register(s0);

VS_OUT VSMain(VS_IN vin)
{
    VS_OUT o;
    o.pos = float4(vin.pos.xy, 1.0f, 1.0f);
    o.uv = vin.uv;
    return o;
}

float4 PSMain(VS_OUT pin) : SV_Target
{
    return gTex.Sample(samLinear, pin.uv);
}