#include "../ShaderDefines.hlsl"

Texture2D tex : register(t0);

struct VS_IN
{
    float3 posL : POSITION;
    float2 uv : TEXCOORD;
};

struct PS_IN
{
    float4 posH : SV_POSITION;
    float2 uv : TEXCOORD;
};

PS_IN VSMain(VS_IN vin)
{
    PS_IN output;

    output.posH = mul(float4(vin.posL, 1.f), g_matWVP);
    output.uv = vin.uv;

    return output;
}

// Pixel Shader
float4 PSMain(PS_IN input) : SV_Target
{
	float4 texColor = tex.Sample(LinearClamp, input.uv);
	
	float feather = 0.005f;

	float fadeX = smoothstep(0.0f, feather, input.uv.x) * (1.0f - smoothstep(1.0f - feather, 1.0f, input.uv.x));
	float fadeY = smoothstep(0.0f, feather, input.uv.y) * (1.0f - smoothstep(1.0f - feather, 1.0f, input.uv.y));
	float edgeAlpha = fadeX * fadeY;
	
	return float4(texColor.rgb, g_ui_color.a * edgeAlpha);
}
