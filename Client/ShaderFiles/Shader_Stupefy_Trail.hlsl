#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"

cbuffer CB_SCROLL : register(b10)
{
	float g_fScrollOffset;
	float g_fAccumulationTime;
	uint g_iCurrentFrame;
	uint g_iFlipbookRows;
	uint g_iFlipbookColumns;
	float3 g_fPadding;
}

struct VS_IN
{
	float3 vPosition : POSITION;
	float2 vUV : TEXCOORD0;
	float4 vColor : COLOR0;
	float4 vEmissive : COLOR1;
};

struct VS_OUT
{
	float4 vPosition : SV_POSITION;
	float2 vUV : TEXCOORD0;
	float4 vColor : COLOR0;
	float4 vEmissive : COLOR1;
};

VS_OUT VSStupefyTrail(VS_IN In)
{
	VS_OUT Out = (VS_OUT)0;
	Out.vPosition = mul(float4(In.vPosition, 1.f), g_matViewProj);
	Out.vUV = In.vUV;
	Out.vColor = In.vColor;
	Out.vEmissive = In.vEmissive;
	return Out;
}

Texture2D g_DiffuseTexture : register(t1);
Texture2D g_DistortionTexture : register(t3);
Texture2D g_NoiseTexture : register(t4);

struct PS_OUT
{
	float4 vDiffuse : SV_TARGET0;
};

PS_OUT PSStupefyTrail(VS_OUT In)
{
	PS_OUT Out = (PS_OUT)0;
	float2 uv = In.vUV;
	float2 flowUV1 = float2(uv.x * 1.65f - g_fAccumulationTime * 1.15f,
		uv.y * 1.8f + g_fAccumulationTime * 0.18f);
	float2 flowUV2 = float2(uv.x * 3.1f + g_fAccumulationTime * 0.52f,
		uv.y * 3.4f - g_fAccumulationTime * 0.13f);
	float2 flow1 = g_DistortionTexture.Sample(LinearWrap, flowUV1).rg * 2.f - 1.f;
	float2 flow2 = g_NoiseTexture.Sample(LinearWrap, flowUV2).rg * 2.f - 1.f;

	float longitudinal = uv.x * 6.5f;
	float irregularA = sin(longitudinal * 0.31f - g_fAccumulationTime * 1.7f);
	float irregularB = sin(longitudinal * 0.73f + g_fAccumulationTime * 1.15f + 1.9f);
	float irregularC = sin(longitudinal * 1.17f - g_fAccumulationTime * 0.82f + 4.1f);
	float mixedWave = irregularA * 0.52f + irregularB * 0.31f + irregularC * 0.17f;
	// Three luminous threads share a broad flow but have 120-degree phase
	// offsets. They repeatedly trade vertical order, producing an actual braid
	// inside the ribbon instead of three parallel lines.
	// Legacy trail U grows with travelled distance (0.5 U per world unit).
	// At the current 120 u/s and 0.26 s lifetime, 0.145 keeps the complete
	// visible wake below three spatial braid turns.
	float braidPhase = longitudinal * 0.145f - g_fAccumulationTime * 1.28f;
	float braidWarp = irregularA * 0.34f + irregularB * 0.19f +
		irregularC * 0.11f + flow1.x * 0.24f;
	float amplitudeNoise = saturate(flow2.y * 0.5f + 0.5f);
	float braidAmplitude = 0.17f + amplitudeNoise * 0.085f + irregularB * 0.018f;
	float centerWave = sin(braidPhase + braidWarp) * braidAmplitude +
		irregularC * 0.029f;
	float upperWave = sin(braidPhase + 2.094395f + braidWarp * 0.61f +
		irregularB * 0.17f) * (braidAmplitude * 0.92f) + flow2.x * 0.034f;
	float lowerWave = sin(braidPhase + 4.188790f - braidWarp * 0.43f +
		irregularA * 0.14f) * (braidAmplitude * 1.07f) - flow1.y * 0.036f;
	float y = uv.y - 0.5f;
	float widthVariation = 0.86f + 0.14f * sin(longitudinal * 0.43f + g_fAccumulationTime * 1.3f + flow1.y);
	float coreBand = exp(-abs(y - centerWave) * (30.f + widthVariation * 8.f));
	float upperBand = exp(-abs(y - upperWave) * (16.f + widthVariation * 5.f));
	float lowerBand = exp(-abs(y - lowerWave) * (17.f + (1.f - widthVariation) * 8.f));
	float outerAura = exp(-abs(y - centerWave) * 3.45f);
	// The broad layers deliberately move at different rates.  This keeps the
	// projectile readable while making the wake feel like rolling smoke instead
	// of several bright wires travelling together.
	float plumeCenterA = flow1.y * 0.13f + mixedWave * 0.035f;
	float plumeCenterB = -flow2.x * 0.11f + irregularB * 0.045f;
	float driftingPlumeA = exp(-abs(y - plumeCenterA) * 2.05f);
	float driftingPlumeB = exp(-abs(y - plumeCenterB) * 2.7f);

	float2 silkUV = float2(uv.x - g_fAccumulationTime * 0.72f,
		uv.y + flow1.y * 0.045f + flow2.x * 0.018f);
	float4 silk = g_DiffuseTexture.Sample(LinearWrap, silkUV);
	float silkMask = max(silk.a, max(silk.r, max(silk.g, silk.b)));
	float breakupNoise = g_NoiseTexture.Sample(LinearWrap, flowUV1 * 0.68f).r;
	float breakupDetail = g_DistortionTexture.Sample(LinearWrap,
		float2(uv.x * 0.82f + g_fAccumulationTime * 0.19f,
		uv.y * 1.27f - g_fAccumulationTime * 0.09f)).b;
	float breakup = smoothstep(0.18f, 0.78f,
		breakupNoise * 0.68f + breakupDetail * 0.32f);
	float smokeDensity = smoothstep(0.08f, 0.88f,
		breakupNoise * 0.46f + breakupDetail * 0.54f + flow1.y * 0.1f);
	float pulseNoise = g_NoiseTexture.Sample(LinearWrap,
		float2(uv.x * 1.2f - g_fAccumulationTime * 0.9f, 0.37f)).r;
	float pulse = 0.9f + pulseNoise * 0.18f + irregularC * 0.045f;

	float3 whiteCore = float3(0.84f, 0.94f, 1.f) * coreBand * 2.35f;
	float3 violetWisp = float3(0.62f, 0.31f, 1.f) * upperBand * 2.65f;
	float3 cyanWisp = float3(0.18f, 0.68f, 1.f) * lowerBand * 2.85f;
	float3 auraTint = lerp(float3(0.08f, 0.48f, 1.f), float3(0.58f, 0.17f, 0.96f),
		saturate(uv.y));
	float3 aura = auraTint * outerAura * (0.58f + smokeDensity * 0.44f);
	float3 plumeTintA = lerp(float3(0.12f, 0.55f, 1.f), float3(0.47f, 0.2f, 0.98f),
		saturate(flow2.y * 0.5f + 0.5f));
	float3 plumeTintB = lerp(float3(0.15f, 0.82f, 1.f), float3(0.72f, 0.24f, 0.92f),
		saturate(flow1.x * 0.5f + 0.5f));
	float3 plumeColor = (plumeTintA * driftingPlumeA * 0.76f +
		plumeTintB * driftingPlumeB * 0.52f) * (0.28f + smokeDensity * 0.72f);
	float3 color = (whiteCore + violetWisp + cyanWisp + aura + plumeColor) * pulse;
	float alphaMask = saturate(coreBand + upperBand * 0.66f + lowerBand * 0.7f +
		outerAura * smokeDensity * 0.4f + driftingPlumeA * breakup * 0.34f +
		driftingPlumeB * smokeDensity * 0.25f);
	float edgeFeather = smoothstep(0.f, 0.22f, saturate(1.f - abs(y) * 2.f));
	float alpha = alphaMask * lerp(0.42f, 0.92f, silkMask) * lerp(0.46f, 1.f, breakup) *
		edgeFeather * In.vColor.a;

	clip(alpha - 0.006f);
	Out.vDiffuse = float4(color, alpha);
	return Out;
}
