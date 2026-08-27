#include "../ShaderDefines.hlsl"

static const float2 RandomRotationCS[8] =
{
	float2(1.00000000f, 0.00000000f),
    float2(0.70710678f, 0.70710678f),
    float2(0.00000000f, 1.00000000f),
    float2(-0.70710678f, 0.70710678f),
    float2(-1.00000000f, 0.00000000f),
    float2(-0.70710678f, -0.70710678f),
    float2(0.00000000f, -1.0000000f),
    float2(0.70710678f, -0.70710678f)
};

float4 Convert_WorldPosByDepth(float _Depth, float2 _TexCoord)
{
    // Depth = NDC   -> (InvProj) -> WorldSpace(InvView)
    float4 NDCWorldPos;
    
    // ViewSpace
    NDCWorldPos.x = _TexCoord.x * +2.f - 1.f;
    NDCWorldPos.y = _TexCoord.y * -2.f + 1.f;
    NDCWorldPos.z = _Depth;
    NDCWorldPos.w = 1.f;
    
    float4 WorldPos = mul(NDCWorldPos, g_matInvViewProj);
    
    return float4(WorldPos.xyz / WorldPos.w, 1.f);
}

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
	float2	LocalNormalXY	= _NormalTex.Sample(LinearWrap, _TexCoord).rg * 2.f - 1.f;
	float	LocalNormalZ	= sqrt(saturate(1.f - dot(LocalNormalXY, LocalNormalXY)));
	float3	LocalNormal		= normalize(float3(LocalNormalXY, LocalNormalZ));
	
	float3	Normal	= normalize(_InNormal.xyz);
	float3	Tangent = normalize(_InTangent.xyz);
    
	Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
	float3	BiNormal = normalize(cross(Normal, Tangent));

	float3	WorldNormal = LocalNormal.x * Tangent + LocalNormal.y * BiNormal + LocalNormal.z * Normal;
	return	normalize(WorldNormal); 
}
float Convert_ViewZPosByDepth(float _Depth)
{
	return g_matProj._m32 / (_Depth - g_matProj._m22);
}

bool Compute_DynamicLight(float3 _WorldPosition, DynamicLight Light, out float3 L, out float3 Radiance)
{

    [branch]
    if (Light.LightType == LIGHT_DIRECTIONAL)   // Directional Light PBR
    {
		float3	NormalizedDirection = normalize(Light.LightDirection.xyz);
		L = -NormalizedDirection;
        Radiance = Light.LightColor * Light.LightIntensity;
    }
    else if (Light.LightType == LIGHT_POINT)    // Point Light PBR
    {
        float3	LightVector = Light.Position - _WorldPosition;
		float	DistanceSQ = dot(LightVector, LightVector);
		
		float	OuterRange = max(Light.OuterAttanuation, 0.001f);
		float	OuterRangeSQ = OuterRange * OuterRange;
		
		[branch]
		if (DistanceSQ >= OuterRangeSQ)	return false; // 빛이 안 닿는 구역
		
		float	InvDistance = rsqrt(max(DistanceSQ, 0.00001f));
		float	Distance = DistanceSQ * InvDistance;
		
		L = LightVector * InvDistance;
		
		float	InnerRange = clamp(Light.InnerAttanuation, 0.f, OuterRange);
		float	FadeRatio =saturate((Distance - InnerRange) / max(OuterRange - InnerRange, 0.001f));
		
		float	DistanceRatio = DistanceSQ / OuterRangeSQ;
		
		float	RangeFade = 1.f - smoothstep(0.f, 1.f, FadeRatio);
		RangeFade *= RangeFade;
		
		//float Attenuation = RangeFade / max(DistanceSQ + 1.f, 1.f);
		Radiance = Light.LightColor * Light.LightIntensity * RangeFade;
	}
    else if (Light.LightType == LIGHT_SPOTLIGHT)    // SpotLight Light PBR
    {
        float3	LightVector = Light.Position - _WorldPosition;
		float	LightRange = max(Light.LightRange, 0.001f);
		float	DistanceSQ = dot(LightVector, LightVector);
		float	RangeSQ = LightRange * LightRange;


		[branch]
		if (DistanceSQ > RangeSQ)
			return false; // 빛이 안 닿는 구역
		
		float	InvDistance =	rsqrt(max(DistanceSQ, 0.00001f));
		float	Distance = DistanceSQ * InvDistance;
		
		float	DistanceRatio = saturate(Distance / Light.LightRange);
		
		L = LightVector * InvDistance;
		
        // Decrease By Distance
		float3	NormalizedDirection = normalize(Light.LightDirection.xyz);
		float	DistanceFade = 1.f - smoothstep(0.f, 1.f, DistanceRatio);
		float	CosAngle = dot(-L, NormalizedDirection);
		float	ConeFade = smoothstep(Light.OuterAttanuation, Light.InnerAttanuation, CosAngle);
	
        Radiance = Light.LightColor * Light.LightIntensity * DistanceFade * ConeFade;
    }
    
    return true;
}

bool Compute_EffectLight(float3 _WorldPosition, EffectLight Light, out float3 L, out float3 Radiance)
{
	float3	LightVector = Light.Position - _WorldPosition;
	float	DistanceSQ = dot(LightVector, LightVector);
		
	float	OuterRange = max(Light.OuterAttanuation, 0.001f);
	float	InnerRange = min(Light.InnerAttanuation, OuterRange);
	float	OuterRangeSQ = OuterRange * OuterRange;
	
	[branch]
	if (DistanceSQ > OuterRangeSQ)
	{
		L		 = float3(0.f, 0.f, 0.f);
		Radiance = float3(0.f, 0.f, 0.f);
		
		return false;
	}
	
	float InvDistance = rsqrt(max(DistanceSQ, 0.00001f));
	float Distance = DistanceSQ * InvDistance;
	
	L = LightVector * InvDistance;
		
	float DistanceRatio = DistanceSQ / OuterRangeSQ;
		
	float RangeFade = 1.f - smoothstep(InnerRange, OuterRange, Distance);
	RangeFade *= RangeFade;
		
	float Attenuation = RangeFade / max(DistanceSQ + 1.f, 1.f);
		
	Radiance = Light.LightColor * Light.LightIntensity * Attenuation;
	
	return true;
}

float3 Apply_DissolveEffect(Texture2D _NoiseTex, float3 _BaseEmissive, float2 _TexCoord, float _EdgeWidth, float _DissolveIntensity)
{
	float DissolveFactor = _NoiseTex.Sample(LinearWrap, _TexCoord).r - _DissolveIntensity;
	clip(DissolveFactor);
	
    float   DissolveEdge = 1.f - smoothstep(0.f, _EdgeWidth, DissolveFactor);
    
	float3 DissolveEmissive = DissolveColor * DissolveEdge;
    
	return _BaseEmissive + DissolveEmissive;
}

float3 Apply_DissolveEffect(Texture2D _NoiseTex, float3 _BaseEmissive, float2 _TexCoord, float _EdgeWidth)
{
	return Apply_DissolveEffect(_NoiseTex, _BaseEmissive, _TexCoord, _EdgeWidth, DissolveIntensity);
}

float Henyey_Greenstein_Phase(float _CosTheta, float _Anistropy)
{
	float Anistropy2 = _Anistropy * _Anistropy;
	float Denum = 1.f + Anistropy2 - 2.f * _Anistropy * _CosTheta;
	
	return (1.f - Anistropy2) / (4.f * PI * pow(max(Denum, 0.0001f), 1.5f));
}
float Henyey_Greenstein_DualPhase(float3 _RayDirection, float3 _FogLightDirection, float _FrontAnistropy, float _BackAnistropy, float k)
{
	float CosTheta = dot(_RayDirection, -_FogLightDirection);
	
	float PhaseValueA = Henyey_Greenstein_Phase(CosTheta, _FrontAnistropy);
	float PhaseValueB = Henyey_Greenstein_Phase(CosTheta, _BackAnistropy);
    
	return lerp(PhaseValueB, PhaseValueA, k);
}

uint Hash_ShadowPixel(uint2 PixelPos)
{
	uint Hash = PixelPos.x * 0x8DA6B343u;
	Hash ^= PixelPos.y * 0xD8163841u;

	Hash ^= Hash >> 16;
	Hash *= 0x7FEB352Du;
	Hash ^= Hash >> 15;

	return Hash;
}

float2x2 Get_RandomNoise(uint2 _PixelPos)
{
	uint RotationIndex = Hash_ShadowPixel(_PixelPos) & 7u;

	float2 CosSin = RandomRotationCS[RotationIndex];

	return float2x2(CosSin.x, -CosSin.y, CosSin.y, CosSin.x);
}

float DistributionGGX(float3 N, float3 H, float _Roughness)
{
	float R = _Roughness * _Roughness;
	float R2 = R * R;
    
	float NDH = max(0.f, dot(N, H));
    
	float Num = R2;
	float Denom = ((NDH * NDH) * (R2 - 1.f) + 1.f);
	Denom = PI * Denom * Denom;
	
	return Num / max(0.000001f, Denom);
}
float VisibilitySmithJointGGX(float NDY, float NDL, float _Roughness)
{
	float R = _Roughness * _Roughness;
	float R2 = R * R;
    
	float lambdaV = NDL * sqrt(max((-NDY * R2 + NDY) * NDY + R2, 0.001f));
	float lambdaL = NDY * sqrt(max((-NDL * R2 + NDL) * NDL + R2, 0.001f));
    
	float Denom = lambdaV + lambdaL;
	return Denom > 0.f ? 0.5f / Denom : 0.f;
}
float3 FresnelSchlick(float CTH, float3 MBR)
{
	float ClampCTH = clamp(CTH, 0.f, 1.f);
	return MBR + (1.f - MBR) * pow(clamp(1.f - ClampCTH, 0.f, 1.f), 5.f);
}
