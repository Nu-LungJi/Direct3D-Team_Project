#include "../ShaderHeader/SH_CommonFunction.hlsli"

struct VS_IN
{
	float3 Position : POSITION;
};

struct VS_OUT
{
	float4 WorldPos : POSITION;
};

struct VS_FINAL_OUT
{
	float4 Position : SV_POSITION;
	float3 WorldPos : POSITION0;
};

VS_OUT VSMain(VS_IN IN)
{
    VS_OUT OUT;
	
	OUT.WorldPos = mul(float4(IN.Position, 1.0f), g_matWorld);
	
    return OUT;
}
VS_FINAL_OUT VSMain_Final(VS_IN IN)
{
	VS_FINAL_OUT OUT;
	float4 WorldPos = mul(float4(IN.Position, 1.0f), g_matWorld);
	OUT.WorldPos	= WorldPos.xyz;
	
	float4 ViewPos	= mul(WorldPos, g_matView);
	OUT.Position	= mul(ViewPos, g_matProj);
	
	return OUT;
}
struct GS_OUT
{
	float4	Position	: SV_POSITION;
    float3  WorldPos    : POSITION;
    uint    LayerIndex  : SV_RenderTargetArrayIndex;
};

[maxvertexcount(18)]
void GSMain(triangle VS_OUT IN[3], inout TriangleStream<GS_OUT> _OutStream)
{
	DynamicLight DLight = AffectedLight[CurrentShadowLightIndex];
    
	for (int Face = 0; Face < 6; ++Face)
	{
		for (int v = 0; v < 3; ++v)
		{
			GS_OUT OUT;
			OUT.Position = mul(float4(IN[v].WorldPos.xyz, 1.f), DLight.g_LightViewProj[Face]);
			OUT.WorldPos = IN[v].WorldPos.xyz;
			OUT.LayerIndex = (DLight.CurrentLightIndex * 6) + Face;
            
			_OutStream.Append(OUT);
		}
		_OutStream.RestartStrip();
	}
}
float PSMain(GS_OUT OUT) : SV_DEPTH
{
	
	DynamicLight DLight = AffectedLight[CurrentShadowLightIndex];
	
    float3	LightToPixel = OUT.WorldPos.xyz - DLight.Position;
    float	Distance = length(LightToPixel);
	float	Depth = Distance / DLight.LightRange;
	
	return saturate(Depth - 0.0005f);
}
