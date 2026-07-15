cbuffer CB_MapPicking : register(b0)
{
    row_major float4x4 g_MatWVP;
    uint g_PickID;
    uint3 g_Padding;
};

struct VS_IN
{
    float3 position : POSITION;
};

struct VS_OUT
{
    float4 position : SV_POSITION;
    nointerpolation uint pickID : PICK_ID;
};

VS_OUT VSMain(VS_IN input)
{
    VS_OUT output;
    output.position = mul(float4(input.position, 1.f), g_MatWVP);
    output.pickID = g_PickID;
    return output;
}

uint PSMain(VS_OUT input) : SV_TARGET
{
    return input.pickID;
}
