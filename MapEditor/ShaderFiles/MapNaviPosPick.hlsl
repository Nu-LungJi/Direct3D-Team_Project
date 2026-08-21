cbuffer CB_MapMeshPositionPicking : register(b0)
{
	row_major float4x4 g_MatWorld;
	row_major float4x4 g_MatWVP;
};

struct VS_IN
{
	float3 position : POSITION;
};

struct VS_OUT
{
	float4 position : SV_POSITION;
	float3 worldPosition : WORLD_POSITION;
};

VS_OUT VSMain(VS_IN input)
{
	VS_OUT output;

	const float4 localPosition = float4(input.position,1.f);

	output.position = mul(localPosition, g_MatWVP);
	output.worldPosition = mul(localPosition, g_MatWorld).xyz;

	return output;
}

float4 PSMain(VS_OUT input) : SV_TARGET
{
	return float4(input.worldPosition, 1.f);
}
