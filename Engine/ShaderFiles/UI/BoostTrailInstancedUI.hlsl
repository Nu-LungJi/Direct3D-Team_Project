#include "../ShaderDefines.hlsl"

Texture2D g_BoostTrailTexture : register(t0);

struct VS_IN
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD0;

	float4 World0 : INSTANCE_WORLD0;
	float4 World1 : INSTANCE_WORLD1;
	float4 World2 : INSTANCE_WORLD2;
	float4 World3 : INSTANCE_WORLD3;
	float4 Color : INSTANCE_COLOR0;
	float2 UVOffset : INSTANCE_UVOFFSET;
	float2 UVSize : INSTANCE_UVSIZE;
};

struct VS_OUT
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
	float4 Color : TEXCOORD1;
};

VS_OUT VSMain(VS_IN input)
{
	VS_OUT output = (VS_OUT)0;
	float4x4 world = float4x4(
		input.World0,
		input.World1,
		input.World2,
		input.World3);
	output.Position = mul(mul(float4(input.Position, 1.f), world), g_matWVP);
	output.TexCoord = input.UVOffset + input.TexCoord * input.UVSize;
	output.Color = input.Color;
	return output;
}

float4 PSMain(VS_OUT input) : SV_Target
{
	float4 textureColor = g_BoostTrailTexture.Sample(
		LinearWrap,
		input.TexCoord);
	float brightness = dot(
		textureColor.rgb,
		float3(0.299f, 0.587f, 0.114f));
	clip(brightness - 0.01f);
	brightness = pow(saturate(brightness), 0.5f);
	return float4(
		input.Color.rgb * brightness,
		brightness * input.Color.a);
}
