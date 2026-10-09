#include "../../../../Engine/ShaderFiles/ShaderHeader/SH_CommonFunction.hlsli"

Texture2D g_DiffuseTexture	: register(t0);
Texture2D g_NormalTexture	: register(t1);
Texture2D g_SMROTexture		: register(t2);
Texture2D g_EmissiveTexture : register(t3);

struct PS_IN
{
	float4 vPosition	: SV_POSITION;
	float4 vNormal		: NORMAL;
	float4 vTangent		: TANGENT;
	float4 vBinormal	: BINORMAL;
	float2 vTexcoord	: TEXCOORD0;
	float4 vWorldPos	: TEXCOORD1;
	float4 vProjPos		: TEXCOORD2;
};

struct PS_OUT
{
	vector vDiffuse		: SV_TARGET0;
	vector vNormal		: SV_TARGET1;
	vector vSMRO		: SV_TARGET2;
	vector vEmissive	: SV_TARGET3;
	float4 vSSRSurface	: SV_TARGET4;
};

PS_OUT PSMain(PS_IN IN)
{
	PS_OUT OUT;

	float4 albedo = g_DiffuseTexture.Sample(LinearWrap, IN.vTexcoord);
	float3 N = Compute_WorldNormal(g_NormalTexture, IN.vTexcoord, IN.vNormal, IN.vTangent);

	float3 mro = g_SMROTexture.Sample(LinearWrap, IN.vTexcoord).rgb;
	float metallic = mro.r * MetallicIntensity;
	float roughness = clamp(mro.g * RoughnessIntensity, 0.03f, 0.18f);
	float ao = mro.b * AmbientIntensity;
	
	float3 geometricNormal = normalize(IN.vNormal.xyz);
	float floorMask = smoothstep(0.80f, 0.95f, dot(geometricNormal, float3(0.f, 1.f, 0.f)));
	
	OUT.vDiffuse = float4(albedo.rgb, 1.f);
	OUT.vNormal = float4(N * 0.5f + 0.5f, 1.f);
	OUT.vSMRO = float4(metallic, roughness, ao, 1.f);
	OUT.vEmissive = float4(0.f, 0.f, 0.f, 1.f);
	
	OUT.vSSRSurface = float4(floorMask, roughness, 0.f, 0.f);
	return OUT;
}
