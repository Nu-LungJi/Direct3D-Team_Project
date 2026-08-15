#include "../../Engine/ShaderFiles/ShaderDefines.hlsl"

Texture2D g_DiffuseTexture		: register(t0);
Texture2D g_NormalTexture		: register(t1);
Texture2D g_SMROTexture			: register(t2);
Texture2D g_MaskTexture			: register(t3);
Texture2D g_EmissiveLayer0Texture	: register(t4);
Texture2D g_EmissiveLayer1Texture	: register(t5);

Texture2D DefaultNoiseTexture	: register(t13);
static const float DissolveEdgeWidth = 0.025f;

cbuffer CB_DRAGON_MATERIAL : register(b10)
{
	float4 TintR;
	float4 TintG;
	float4 TintB;
	
	float4 EmissiveLayerColor;
	float4 EmissiveLayerChannel;
}

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
	vector vDiffuse : SV_TARGET0;
	vector vNormal : SV_TARGET1;
	vector vSMRO : SV_TARGET2;
	vector vEmissive : SV_TARGET3;
};

float3x3 Make_TBNMatrix(float3 _Normal, float3 _Tangent)
{
	float3 Normal = normalize(_Normal);
	float3 Tangent = normalize(_Tangent);

	Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    
	float3 BiNormal = normalize(cross(Normal, Tangent));
    
	return float3x3(Tangent, BiNormal, Normal);
}

float3 Compute_WorldNormal(Texture2D _NormalTex, float2 _TexCoord, float4 _InNormal, float4 _InTangent)
{
	float3 LocalNormal = _NormalTex.Sample(LinearWrap, _TexCoord).rgb;
	LocalNormal = normalize(LocalNormal * 2.f - 1.f);
	float3x3 TBN = Make_TBNMatrix(_InNormal.xyz, _InTangent.xyz);

	float3 N = normalize(_InNormal.xyz);
	float3 T = normalize(_InTangent.xyz);
    
	T = normalize(T - dot(T, N) * N);
	float3 B = normalize(cross(N, T));
    
	float3 worldNormal = LocalNormal.x * T + LocalNormal.y * B + LocalNormal.z * N;

	return normalize(worldNormal);
}

float3 Apply_DissolveEffect(Texture2D _NoiseTex, float3 _BaseEmissive, float2 _TexCoord, float _EdgeWidth)
{
	float DissolveFactor = _NoiseTex.Sample(LinearWrap, _TexCoord).r - DissolveIntensity;
	clip(DissolveFactor);
	
	float DissolveEdge = 1.f - smoothstep(0.f, _EdgeWidth, DissolveFactor);
    
	float3 DissolveEmissive = DissolveColor * DissolveEdge;
    
	return _BaseEmissive + DissolveEmissive;
}

PS_OUT PSMain(PS_IN IN)
{
	PS_OUT Out;
    
	float4	Diffuse = g_DiffuseTexture.Sample(LinearWrap, IN.vTexcoord) * float4(AlbedoColor, ObjectAlpha);
	float3	MaskTex = g_MaskTexture.Sample(LinearWrap, IN.vTexcoord).rgb;
    
	float3 DiffuseOutput = Diffuse.rgb;
	DiffuseOutput = lerp(DiffuseOutput, DiffuseOutput * TintB.rgb, saturate(MaskTex.b * TintB.a));
	DiffuseOutput = lerp(DiffuseOutput, DiffuseOutput * TintG.rgb, saturate(MaskTex.g * TintG.a));
	DiffuseOutput = lerp(DiffuseOutput, DiffuseOutput * TintR.rgb, saturate(MaskTex.r * TintR.a));
	
	float3	Normal	= Compute_WorldNormal(g_NormalTexture, IN.vTexcoord, IN.vNormal, IN.vTangent) * NormalIntensity;
	float3	MRO		= g_SMROTexture.Sample(LinearWrap, IN.vTexcoord);
    
	float	FinalMetallic	= MRO.r * MetallicIntensity;
	float	FinalRoughness	= MRO.g * RoughnessIntensity;
	float	FinalAO			= MRO.b * AmbientIntensity;
	
	float3 EmissiveLayer0 = g_EmissiveLayer0Texture.Sample(LinearWrap, IN.vTexcoord).rgb;
	EmissiveLayer0 *= EmissiveColor * EmissiveIntensity;
	
	float4	EmissiveLayer1Tex = g_EmissiveLayer1Texture.Sample(LinearWrap, IN.vTexcoord);
	float	EmissiveLayer1Value = saturate(dot(EmissiveLayer1Tex, EmissiveLayerChannel));

	float3	EmissiveLayer1 = EmissiveLayer1Value * EmissiveLayerColor.rgb * EmissiveLayerColor.a;
	float3 Emissive = EmissiveLayer0 + EmissiveLayer1;

	float3 FinalEmissive = Apply_DissolveEffect(DefaultNoiseTexture, Emissive, IN.vTexcoord, DissolveEdgeWidth);
	
	Out.vDiffuse  = float4(DiffuseOutput.rgb, Diffuse.a);
	Out.vNormal   = float4(Normal * 0.5f + 0.5f, 1.f);
	Out.vSMRO     = float4(FinalMetallic, FinalRoughness, FinalAO, 1.f);
	Out.vEmissive = float4(FinalEmissive, 1.f);
    
	return Out;
}

PS_OUT PSMain_OnlyEmissive(PS_IN IN)
{
	PS_OUT OUT;
	
	float3 FinalEmissive = g_EmissiveLayer0Texture.Sample(LinearWrap, IN.vTexcoord).rgb * EmissiveColor * EmissiveIntensity;
	
	OUT.vDiffuse = float4(0.f, 0.f, 0.f, 0.f);
	OUT.vNormal = float4(0.f, 0.f, 0.f, 0.f);
	OUT.vSMRO = float4(0.f, 0.f, 0.f, 0.f);
	OUT.vEmissive = float4(FinalEmissive, 1.f);
	
	return OUT;
}
