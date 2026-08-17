#include "../../Engine/ShaderFiles/ShaderDefines.hlsl"

Texture2D g_DiffuseTexture		: register(t0);
Texture2D g_NormalTexture		: register(t1);
Texture2D g_SMROTexture			: register(t2);
Texture2D g_EmissiveTexture		: register(t3);
Texture2D g_MaterialMaskTexture : register(t4);

Texture2D g_MarbleNoiseTexture	: register(t5);
Texture2D g_RiverNoiseTexture	: register(t6);
Texture2D g_CausticNoiseTexture : register(t7);
Texture2D g_DetailNoiseTexture	: register(t8);

Texture2D DefaultNoiseTexture	: register(t13);
static const float DissolveEdgeWidth = 0.025f;
static const float DRAGON_ALPHA_CLIP = 0.3333f;

struct PS_IN
{
	float4 vPosition : SV_POSITION;
	float4 vNormal : NORMAL;
	float4 vTangent : TANGENT;
	float4 vBinormal : BINORMAL;
	float2 vTexcoord : TEXCOORD0;
	float4 vWorldPos : TEXCOORD1;
	float4 vProjPos : TEXCOORD2;
};

struct PS_OUT
{
	vector vDiffuse  : SV_TARGET0;
	vector vNormal	 : SV_TARGET1;
	vector vSMRO	 : SV_TARGET2;
	vector vEmissive : SV_TARGET3;
};

struct PS_FX_OUT
{
	float4 vColor : SV_Target0;
};

float3 Compute_WorldNormal(PS_IN IN)
{
	float3 LocalNormal = g_NormalTexture.Sample(LinearWrap, IN.vTexcoord).xyz * 2.f - 1.f;

	float3 N = normalize(IN.vNormal.xyz);
	float3 T = normalize(IN.vTangent.xyz - N * dot(IN.vTangent.xyz, N));
	float3 B = normalize(IN.vBinormal.xyz);
    
	return normalize(LocalNormal.x * T + LocalNormal.y * B + LocalNormal.z * N);
}

float3 Apply_DissolveEffect(float3 _BaseEmissive, float2 _TexCoord)
{
	if (DissolveIntensity <= 0.0001f) return _BaseEmissive;
	
	float DissolveFactor = DefaultNoiseTexture.Sample(LinearWrap, _TexCoord).r - DissolveIntensity;
	clip(DissolveFactor);
	
	float	DissolveEdge = 1.f - smoothstep(0.f, DissolveEdgeWidth, DissolveFactor);

	return _BaseEmissive + DissolveColor * DissolveEdge;
}

float3 Dragon_SRGBToLinear(float3 Color)
{
	Color = saturate(Color);

	float3 Low = Color / 12.92f;
	float3 High = pow((Color + 0.055f) / 1.055f, 2.4f);

	return lerp(Low, High, step(0.04045f, Color));
}

PS_OUT PSMain_DragonWing(PS_IN IN)
{
	PS_OUT OUT;
	
	float2 UV = IN.vTexcoord;
	
	float4	Diffuse = g_DiffuseTexture.Sample(LinearWrap, UV);
	float	FinalAlpha = Diffuse.a * saturate(ObjectAlpha);
	clip(FinalAlpha - DRAGON_ALPHA_CLIP);
	
	float3	WorldNormal = Compute_WorldNormal(IN);
	float3	SMRO = g_SMROTexture.Sample(LinearWrap, UV).rgb;
	float4	MaterialMask = g_MaterialMaskTexture.Sample(LinearWrap, UV);
	
	float	DiffuseLuma = dot(Diffuse.rgb, float3(0.2126f, 0.7152f, 0.0722f));
	float	PhysicalBoneMask = smoothstep(0.45f, 0.90f, MaterialMask.b);
	float	BrightBoneMask = smoothstep(0.38f, 0.72f, DiffuseLuma);
	BrightBoneMask *= PhysicalBoneMask;
	
	float	MetallicBoneMask = smoothstep(0.55f, 0.90f, SMRO.r);

	float	BoneCorrectionMask = BrightBoneMask * lerp(0.55f, 1.0f, MetallicBoneMask);
	
	float3	CorrectedBoneColor = Diffuse.rgb * float3(0.82f, 0.86f, 0.88f);
	
	float3	FinalDiffuse = lerp(Diffuse.rgb, CorrectedBoneColor, BoneCorrectionMask);

	float	RedThreadMask = saturate(MaterialMask.r);

	float	ThreadLuma = dot(FinalDiffuse, float3(0.2126f, 0.7152f, 0.0722f));

	float3	NeutralThreadColor = ThreadLuma * float3(0.95f, 0.99f, 1.00f);

	FinalDiffuse = lerp(FinalDiffuse, NeutralThreadColor, RedThreadMask * 0.9f);
	FinalDiffuse *= AlbedoColor;
	float3 FinalEmissive = Apply_DissolveEffect(float3(0.0f, 0.0f, 0.0f), UV);
	
	OUT.vDiffuse	= float4(FinalDiffuse, FinalAlpha);
	OUT.vNormal		= float4(WorldNormal * 0.5f + 0.5f, 1.f);
	OUT.vSMRO		= float4(SMRO.r * 0.2f, SMRO.g, SMRO.b, 1.f);
	OUT.vEmissive	= float4(FinalEmissive, 1.f);
	
	return OUT;
}

PS_OUT PSMain_DragonBody(PS_IN IN)
{
	PS_OUT OUT;
	
	float4	Diffuse = g_DiffuseTexture.Sample(LinearWrap, IN.vTexcoord);
	float	FinalAlpha = Diffuse.a * saturate(ObjectAlpha);
	clip(FinalAlpha - DRAGON_ALPHA_CLIP);
	
	float3	WorldNormal = Compute_WorldNormal(IN);
	float3	SMRO = g_SMROTexture.Sample(LinearWrap, IN.vTexcoord).rgb;
	float4	MaterialMask = g_MaterialMaskTexture.Sample(LinearWrap, IN.vTexcoord);
	
	float	OuterNonBone = smoothstep(0.12f, 0.82f, 1.f - MaterialMask.b);
	float	OuterCavity = smoothstep(0.06f, 0.46f, MaterialMask.g);
	OuterCavity *= OuterNonBone;
	
	float InnerNonBone = smoothstep(0.25f, 0.90f, 1.f - MaterialMask.b);
	float InnerCavity = smoothstep(0.22f, 0.68f, MaterialMask.g);
	InnerCavity *= InnerNonBone;
	InnerCavity = pow(saturate(InnerCavity), 1.90f);
	
	float ThinCrackMask = smoothstep(0.16f, 0.64f, MaterialMask.r);
	ThinCrackMask *= InnerNonBone;
	
	float RedExcess = Diffuse.r - max(Diffuse.g, Diffuse.b);

	float RedSeed = smoothstep(0.025f, 0.20f, saturate(RedExcess));
	float BaseEnergyMask = saturate(InnerCavity + ThinCrackMask * 0.18f);
	float HotCoreMask = InnerCavity * lerp(0.15f, 1.f, RedSeed);
	HotCoreMask = pow(saturate(HotCoreMask), 1.75f);
	
	float3 DarkCrimson = float3(0.16f, 0.f, 0.004f);
	float3 DeepRed = float3(0.82f, 0.204f, 0.218f);
	float3 HotPink = float3(1.f, 0.075f, 0.01f);		
	
	float3 FinalEmissive = DarkCrimson * BaseEnergyMask;
	FinalEmissive += DeepRed * InnerCavity * 1.25f;
	FinalEmissive += HotPink * HotCoreMask * 2.25f;
	FinalEmissive += BaseEnergyMask * EmissiveColor * EmissiveIntensity;
	FinalEmissive = Apply_DissolveEffect(FinalEmissive, IN.vTexcoord);
	
	float	DiffuseLuma = dot(Diffuse.rgb, float3(0.2126f, 0.7152f, 0.0722f));
	float	PhysicalBoneMask = smoothstep(0.45f, 0.90f, MaterialMask.b);
	float	BrightBoneMask = smoothstep(0.36f, 0.72f, DiffuseLuma);
	BrightBoneMask *= PhysicalBoneMask;
	
	float	MetallicBoneMask = smoothstep(0.55f, 0.90f, SMRO.r);
	float	BoneCorrectionMask = BrightBoneMask * lerp(0.55f, 1.0f, MetallicBoneMask);
	float3	CorrectedBoneColor = Diffuse.rgb * float3(0.82f, 0.86f, 0.88f);
	
	float3 BodyBaseColor = lerp(Diffuse.rgb, CorrectedBoneColor, BoneCorrectionMask);
	
	float3 FinalDiffuse = BodyBaseColor * AlbedoColor;
	FinalDiffuse *= lerp(1.f, 0.f, OuterCavity);
	
	OUT.vDiffuse	= float4(FinalDiffuse, FinalAlpha);
	OUT.vNormal		= float4(WorldNormal * 0.5f + 0.5f, 1.f);
	OUT.vSMRO		= float4(SMRO.r * 0.2f, SMRO.g * RoughnessIntensity, SMRO.b * AmbientIntensity, 1.f);
	OUT.vEmissive	= float4(FinalEmissive, 1.f);
	
	return OUT;
}


PS_FX_OUT PSMain_EtherealWing(PS_IN IN)
{
	PS_FX_OUT OUT;
	
	float2 UV = IN.vTexcoord;
	
	float MainNoise = g_RiverNoiseTexture.Sample(LinearWrap, UV * float2(2.0f, 0.85f) + g_fTimeAccumulation * float2(0.045f, -0.20f)).r;
	float DetailNoise = g_DetailNoiseTexture.Sample(LinearWrap, UV * 4.8f + g_fTimeAccumulation * float2(0.08f, -0.48f)).r;
	
	float2 Distortion = float2(MainNoise, DetailNoise) - 0.5f;
	float2 WarpedUV = UV + Distortion * 0.018f;
	
	float AuthoredMask = saturate(g_MaterialMaskTexture.Sample(LinearWrap, UV).r);

	float marble = g_MarbleNoiseTexture.Sample(LinearWrap, WarpedUV * float2(1.0f, 2.0f) + g_fTimeAccumulation * float2(-0.057143f, -0.87619f)).r;
	float caustic = g_CausticNoiseTexture.Sample(LinearWrap, WarpedUV * float2(3.105596f, 0.814346f) + g_fTimeAccumulation * float2(0.f, -1.090998f)).r;

	float broadFlow = saturate(marble * 0.58f + MainNoise * 0.42f);
	float flameRidges = smoothstep(0.34f, 0.78f, caustic * 0.70f + DetailNoise * 0.30f);
	float flicker = lerp(0.72f, 1.20f, saturate(broadFlow * 0.65f + flameRidges * 0.35f));
	
	float ShapedMask = pow(AuthoredMask, 4.f);
	float Opacity = smoothstep(0.12f, 0.65f, ShapedMask);
	Opacity *= lerp(0.82f, 1.0f, broadFlow * 0.6f + flameRidges * 0.4f);

	float rootHeat = smoothstep(0.38f, 0.92f, AuthoredMask);
	float middleHeat = smoothstep(0.10f, 0.58f, AuthoredMask);
	float core = pow(saturate(AuthoredMask), 1.35f);

	float3 darkRed = float3(0.16f, 0.000f, 0.004f);
	float3 deepRed = float3(0.82f, 0.004f, 0.018f);
	float3 hotPink = float3(1.00f, 0.075f, 0.110f);

	float3 flameColor = lerp(darkRed, deepRed, middleHeat);
	flameColor = lerp(flameColor, hotPink, rootHeat);

	float emissiveStrength = 1.15f;
	emissiveStrength += core * 2.35f;
	emissiveStrength += flameRidges * 1.15f;
	emissiveStrength *= flicker;
	emissiveStrength += 0.08f;

	OUT.vColor = float4(flameColor * emissiveStrength, Opacity);
	
	return OUT;
}
