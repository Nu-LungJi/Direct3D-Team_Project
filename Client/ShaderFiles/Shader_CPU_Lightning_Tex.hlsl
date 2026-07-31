#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"

const static float  DistortionSTR = 0.03f;
const static float  EdgeWidth = 0.02f;
const static float4 EdgeColor = { 0.f, 0.f, 1.f, 1.f };

const static float PressingValue = 0.125f;  

const static float BranchDensity = 3.f;
const static float Sharpness = 6.f;
const static float FlickeringSpeed = 25.f;

const static float2 DistortionStrength = float2(0.04f, 0.008f);

Texture2D g_DiffuseTexture : register(t1);
Texture2D g_NormalTexture : register(t2);
Texture2D g_DistortionTexture : register(t3);
Texture2D g_NoiseTexture : register(t4);
Texture2D g_AnyTexture : register(t5);
Texture2D g_BackgroundTex : register(t7);

struct VS_IN
{
	float3 vPosition : POSITION;
	float2 vTexcoord : TEXCOORD0;
	float4 vWorld0 : INSTANCE_WORLD0;
	float4 vWorld1 : INSTANCE_WORLD1;
	float4 vWorld2 : INSTANCE_WORLD2;
	float4 vWorld3 : INSTANCE_WORLD3;
	float4 vColor : INSTANCE_COLOR0;
	float4 vInstEmissive : INSTANCE_EMISSIVE;
	float4 vInstEndEmissive : INSTANCE_EMISSIVE1;
	float4 vInstOriginalEmissive : INSTANCE_EMISSIVE2;
	float2 uvOffset : INSTANCE_UVOFFSET;
	float2 uvSize : INSTANCE_UVSIZE;
	float life : INSTANCE_LIFE;
	float maxLife : INSTANCE_MAXLIFE;
	uint iBehaviorType : INSTANCE_BEHAVIORTYPE;
};

struct VS_OUT
{
	float4 vPosition : SV_POSITION;
	float2 vTexcoord : TEXCOORD0;

	float4 vColor : TEXCOORD1;
	float4 vEmissive : TEXCOORD2;
	float4 vEndEmissive : TEXCOORD3;

	uint iBehaviorType : TEXCOORD4;
	float4 vScreenPos : TEXCOORD5;
	float3 vNormal : TEXCOORD6;
	float3 vTangent : TEXCOORD7;
	float3 vWorldPos : TEXCOORD8;
	float life : TEXCOORD9;
	float maxLife : TEXCOORD10;
};


struct PS_OUT
{
	float4 vDiffuse : SV_TARGET0;
};
float Jitter(float _Ratio)
{
	return frac(sin(_Ratio * 123.45f) * 43758.5453f) * 0.02f;
}

float Get_BrightnessMask(Texture2D _Texture, float2 _TexCoord)
{
	float4 DiffuseTex = _Texture.Sample(LinearWrap, _TexCoord);
	return max(DiffuseTex.r, max(DiffuseTex.g, DiffuseTex.b));
}

float Enhance_Glowness(float2 _TexCoord, float2 _TexelSize, float _Radius)
{
	float2 Offset = _TexelSize * _Radius;
	
	float EnhancedMask = Get_BrightnessMask(g_DiffuseTexture, _TexCoord);
	
	EnhancedMask = max(EnhancedMask, Get_BrightnessMask(g_DiffuseTexture, _TexCoord + float2(+Offset.x, 0.f)));
	EnhancedMask = max(EnhancedMask, Get_BrightnessMask(g_DiffuseTexture, _TexCoord + float2(-Offset.x, 0.f)));
	EnhancedMask = max(EnhancedMask, Get_BrightnessMask(g_DiffuseTexture, _TexCoord + float2(0.f, +Offset.y)));
	EnhancedMask = max(EnhancedMask, Get_BrightnessMask(g_DiffuseTexture, _TexCoord + float2(0.f, -Offset.y)));

	EnhancedMask = max(EnhancedMask, Get_BrightnessMask(g_DiffuseTexture, _TexCoord + float2(+Offset.x, +Offset.y)));
	EnhancedMask = max(EnhancedMask, Get_BrightnessMask(g_DiffuseTexture, _TexCoord + float2(-Offset.x, +Offset.y)));
	EnhancedMask = max(EnhancedMask, Get_BrightnessMask(g_DiffuseTexture, _TexCoord + float2(+Offset.x, -Offset.y)));
	EnhancedMask = max(EnhancedMask, Get_BrightnessMask(g_DiffuseTexture, _TexCoord + float2(-Offset.x, -Offset.y)));
	
	return EnhancedMask;
}

VS_OUT VSMain(VS_IN In)
{
	VS_OUT Out = (VS_OUT) 0;

	float4x4 matWorld	= float4x4(In.vWorld0, In.vWorld1, In.vWorld2, In.vWorld3);
	
	float3	 vCenter	= float3(matWorld._41, matWorld._42, matWorld._43);
			 
	float3	 vRight		= normalize(float3(matWorld._11, matWorld._12, matWorld._13));
	float3	 vUp		= normalize(float3(matWorld._21, matWorld._22, matWorld._23));
			 
	float	 fscaleX	= length(float3(matWorld._11, matWorld._12, matWorld._13));
	float	 fscaleY	= length(float3(matWorld._21, matWorld._22, matWorld._23));
			 
	float3	 vWorldPos  = vCenter + vRight * In.vPosition.x * fscaleX + vUp * In.vPosition.y * fscaleY;

	Out.vPosition = mul(float4(vWorldPos, 1.f), g_matViewProj);
	Out.vTexcoord = In.uvOffset + In.vTexcoord * In.uvSize;
	Out.vColor = In.vColor;
	Out.vEmissive = In.vInstEmissive;
	Out.vEndEmissive = In.vInstEndEmissive;
	Out.vScreenPos = Out.vPosition;
	Out.iBehaviorType = In.iBehaviorType;
	Out.vWorldPos = vWorldPos;
	Out.vTangent = vRight;
	Out.vNormal = normalize(cross(vRight, vUp));
	Out.life = In.life;
	Out.maxLife = In.maxLife;
    
	return Out;
}
VS_OUT VSMain_Extra(VS_IN In)
{
	VS_OUT Out = (VS_OUT) 0;

	float4x4 matWorld	= float4x4(In.vWorld0, In.vWorld1, In.vWorld2, In.vWorld3);
	
	float3	 vCenter	= float3(matWorld._41, matWorld._42, matWorld._43);

	float3	 vRight		= normalize(float3(matWorld._11, matWorld._12, matWorld._13));
	float3	 vUp		= normalize(float3(matWorld._21, matWorld._22, matWorld._23));

	float	 fscaleX	= length(float3(matWorld._11, matWorld._12, matWorld._13));
	float	 fscaleY	= length(float3(matWorld._21, matWorld._22, matWorld._23));

	float3	 vWorldPos	= vCenter + vRight * In.vPosition.x * fscaleX + vUp * In.vPosition.y * fscaleY;

	Out.vPosition = mul(float4(vWorldPos, 1.f), g_matViewProj);
	Out.vTexcoord = In.uvOffset + In.vTexcoord * In.uvSize;
	Out.vColor = In.vColor;
	Out.vEmissive = In.vInstEmissive;
	Out.vEndEmissive = In.vInstEndEmissive;
	Out.vScreenPos = Out.vPosition;
	Out.iBehaviorType = In.iBehaviorType;
	Out.vWorldPos = vWorldPos;
	Out.vTangent = vRight;
	Out.vNormal = normalize(cross(vRight, vUp));
	Out.life = In.life;
	Out.maxLife = In.maxLife;
    
	return Out;
}

PS_OUT PSMain_RChannel(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;
	
	float Ratio = saturate(In.life / max(In.maxLife, 0.0001f));
	float4 Emissive = lerp(In.vEmissive, In.vEndEmissive, Ratio);
	
	[branch]
	if ((In.iBehaviorType & BEHAVIOR_DISTORTION) != 0)
	{
		float2 ScrollUV = In.vTexcoord + float2(Ratio * 0.002f, Ratio * -0.025f);
		float4 DistortTex = g_DistortionTexture.Sample(LinearWrap, ScrollUV);

		float2 Offset = (DistortTex.rg - 0.5f) * 2.f * DistortionSTR;
			
		float Jittering = Jitter(Ratio);
		float2 DissolveTiling = float2(8.f, 15.f);
		float2 DissolveUV = ScrollUV + Offset * 0.3f * float2(Jittering, Jittering);
		
		float4 DissolveTex = g_NoiseTexture.Sample(LinearWrap, DissolveUV * DissolveTiling);
		
		DissolveTex.r = pow(DissolveTex.r, 1.5f);
		//float4 FinalColor = g_DiffuseTexture.Sample(LinearClamp, In.vTexcoord * float2(1.f, smoothstep(0.0, 0.2, In.vTexcoord.y) * (1 - smoothstep(0.8, 1.f, In.vTexcoord.y)) + In.life * 0.5f));
		float DiffuseMask = (1.f - (In.vTexcoord.y - Ratio * 0.05f)) * smoothstep(0.0f, 0.5f, Ratio);
		float DiffuseUVY = In.vTexcoord.y - DiffuseMask * PressingValue;
		float4 FinalColor = g_DiffuseTexture.Sample(LinearClamp, float2(In.vTexcoord.x, DiffuseUVY + Offset.y));
		
		float DissolveMask = FinalColor.a;

		float FadeInDuration = 0.08f; // 시작 후, N초
		float FadeOutDuration = In.maxLife; // 종료 전, N초
		
		float FadeInRatio = saturate(FadeInDuration / In.maxLife);
		float FadeOutRatio = saturate(FadeOutDuration / In.maxLife);
		
		float FadeOutStart = 1.f - FadeOutRatio;

		float DissolveProgress = 0.f;

		
		[branch]
		if (Ratio < FadeInRatio)
		{
			DissolveProgress = 1.f - (Ratio / FadeInRatio);
		}
		else if (Ratio > FadeOutStart)
		{
			DissolveProgress = (Ratio - FadeOutStart) / FadeOutRatio;
		}
		else
		{
			DissolveProgress = 0.f;
		}
		DissolveProgress *= 2.2f;
		clip(DissolveMask - 0.3f);
		clip(FinalColor.r - DissolveProgress);
		
		if (DissolveProgress > 0.001f && (FinalColor.r - DissolveProgress < EdgeWidth))
		{
			float EdgeGlow = 1.0f - saturate((FinalColor.r - DissolveProgress) / EdgeWidth);
			FinalColor.rgb += EdgeColor.rgb * EdgeGlow * 12.0f;
		}

		Out.vDiffuse = float4(FinalColor.rrr / 2.f + Emissive.rgb, FinalColor.r * In.vColor.a);
	}
	else
	{
		float4 vFinalColor = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);
		Out.vDiffuse = float4(vFinalColor.rrr / 2.f + Emissive.rgb, vFinalColor.r * In.vColor.a);
	}
	
	return Out;
}

PS_OUT PSMain_GChannel(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;
	
	float Ratio = saturate(In.life / max(In.maxLife, 0.0001f));
	float4 Emissive = lerp(In.vEmissive, In.vEndEmissive, Ratio);
	
	[branch]
	if ((In.iBehaviorType & BEHAVIOR_DISTORTION) != 0)
	{
		float2 ScrollUV = In.vTexcoord + float2(Ratio * 0.002f, Ratio * -0.025f);
		float4 DistortTex = g_DistortionTexture.Sample(LinearWrap, ScrollUV);

		float2 Offset = (DistortTex.rg - 0.5f) * 2.f * DistortionSTR;
			
		float Jittering = Jitter(Ratio);
		float2 DissolveTiling = float2(8.f, 15.f);
		float2 DissolveUV = ScrollUV + Offset * 0.3f * float2(Jittering, Jittering);
		
		float4 DissolveTex = g_NoiseTexture.Sample(LinearWrap, DissolveUV * DissolveTiling);
		
		DissolveTex.r = pow(DissolveTex.r, 1.5f);
		float DiffuseMask = (1.f - (In.vTexcoord.y - Ratio * 0.05f)) * smoothstep(0.0f, 0.5f, Ratio);
		float DiffuseUVY = In.vTexcoord.y - DiffuseMask * PressingValue;
		float4 FinalColor = g_DiffuseTexture.Sample(LinearClamp, float2(In.vTexcoord.x, DiffuseUVY + Offset.y));
		
		float DissolveMask = FinalColor.a;

		float FadeInDuration = 0.08f; // 시작 후, N초
		float FadeOutDuration = In.maxLife; // 종료 전, N초
		
		float FadeInRatio  = saturate(FadeInDuration  / In.maxLife);
		float FadeOutRatio = saturate(FadeOutDuration / In.maxLife);
		
		float FadeOutStart = 1.f - FadeOutRatio;

		float DissolveProgress = 0.f;
		
		[branch]
		if (Ratio < FadeInRatio)
		{
			DissolveProgress = 1.f - (Ratio / FadeInRatio);
		}
		else if (Ratio > FadeOutStart)
		{
			DissolveProgress = (Ratio - FadeOutStart) / FadeOutRatio;
		}
		else
		{
			DissolveProgress = 0.f;
		}
		DissolveProgress *= 2.2f;
		clip(DissolveMask - 0.3f);
		clip(FinalColor.g - DissolveProgress);
		
		if (DissolveProgress > 0.001f && (FinalColor.g - DissolveProgress < EdgeWidth))
		{
			float EdgeGlow = 1.0f - saturate((FinalColor.g - DissolveProgress) / EdgeWidth);
			FinalColor.rgb += EdgeColor.rgb * EdgeGlow * 12.0f;
		}

		Out.vDiffuse = float4(FinalColor.ggg / 2.f + Emissive.rgb * EmissiveIntensity, FinalColor.g * In.vColor.a);
	}
	else
	{
		float4 vFinalColor = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);
		Out.vDiffuse = float4(vFinalColor.ggg / 2.f + Emissive.rgb * EmissiveIntensity, vFinalColor.g * In.vColor.a);
	}
 
	return Out;
}

PS_OUT PSMain_BChannel(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;
	
	float Ratio = saturate(In.life / max(In.maxLife, 0.0001f));
	float4 Emissive = lerp(In.vEmissive, In.vEndEmissive, Ratio);
	
	[branch]
	if ((In.iBehaviorType & BEHAVIOR_DISTORTION) != 0)
	{
		float2 ScrollUV = In.vTexcoord + float2(Ratio * 0.002f, Ratio * -0.025f);
		float4 DistortTex = g_DistortionTexture.Sample(LinearWrap, ScrollUV);

		float2 Offset = (DistortTex.rg - 0.5f) * 2.f * DistortionSTR;
			
		float Jittering = Jitter(Ratio);
		float2 DissolveTiling = float2(8.f, 15.f);
		float2 DissolveUV = ScrollUV + Offset * 0.3f * float2(Jittering, Jittering);
		
		float4 DissolveTex = g_NoiseTexture.Sample(LinearWrap, DissolveUV * DissolveTiling);
		
		DissolveTex.r = pow(DissolveTex.r, 1.5f);
		//float4 FinalColor = g_DiffuseTexture.Sample(LinearClamp, In.vTexcoord * float2(1.f, smoothstep(0.0, 0.2, In.vTexcoord.y) * (1 - smoothstep(0.8, 1.f, In.vTexcoord.y)) + In.life * 0.5f));
		float DiffuseMask = (1.f - (In.vTexcoord.y - Ratio * 0.05f)) * smoothstep(0.0f, 0.5f, Ratio);
		float DiffuseUVY = In.vTexcoord.y - DiffuseMask * PressingValue;
		float4 FinalColor = g_DiffuseTexture.Sample(LinearClamp, float2(In.vTexcoord.x, DiffuseUVY + Offset.y));
		
		float DissolveMask = FinalColor.a;

		float FadeInDuration = 0.08f; // 시작 후, N초
		float FadeOutDuration = In.maxLife; // 종료 전, N초
		
		float FadeInRatio = saturate(FadeInDuration / In.maxLife);
		float FadeOutRatio = saturate(FadeOutDuration / In.maxLife);
		
		float FadeOutStart = 1.f - FadeOutRatio;

		float DissolveProgress = 0.f;
		
		[branch]
		if (Ratio < FadeInRatio)
		{
			DissolveProgress = 1.f - (Ratio / FadeInRatio);
		}
		else if (Ratio > FadeOutStart)
		{
			DissolveProgress = (Ratio - FadeOutStart) / FadeOutRatio;
		}
		else
		{
			DissolveProgress = 0.f;
		}
		DissolveProgress *= 2.2f;
		clip(DissolveMask - 0.3f);
		clip(FinalColor.b - DissolveProgress);
		
		if (DissolveProgress > 0.001f && (FinalColor.b - DissolveProgress < EdgeWidth))
		{
			float EdgeGlow = 1.0f - saturate((FinalColor.b - DissolveProgress) / EdgeWidth);
			FinalColor.rgb += EdgeColor.rgb * EdgeGlow * 12.0f;
		}

		Out.vDiffuse = float4(FinalColor.bbb / 2.f + Emissive.rgb * EmissiveIntensity, FinalColor.b * In.vColor.a);
	}
	else
	{
		float4 vFinalColor = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);
		Out.vDiffuse = float4(vFinalColor.bbb / 2.f + Emissive.rgb * EmissiveIntensity, vFinalColor.b * In.vColor.a);
	}
 
	return Out;
}

PS_OUT PSMain_ExtraLightning(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float2	ScrollUVA  = In.vTexcoord * float2(1.f, BranchDensity) + float2(+In.life * 2.f, In.life * 12.f);
	float2	ScrollUVB  = In.vTexcoord * float2(1.f, BranchDensity) + float2(-In.life * 3.f, In.life * 18.f);
	
	float	DistortTex = g_DiffuseTexture.Sample(LinearWrap, ScrollUVA).r;
	
	float2	DistortUV  = ScrollUVB + (DistortTex - 0.5f) * 0.2f;
	float	RawNoise = g_DiffuseTexture.Sample(LinearWrap, DistortUV).r;
	
	float	LightningMask = pow(RawNoise, Sharpness);
	
	float	Flickering = frac(sin(floor(In.life * FlickeringSpeed)) * 43758.5453);
	float	ActiveMask = smoothstep(0.1f, 0.4f, LightningMask * Flickering);
	
	if (ActiveMask > 0.f)	clip(-1);
		
	float3	CoreDiffuse	= { 12.f, 14.f, 15.f };
	float3	GlowDiffuse = { 0.5f, 3.f, 10.f };
	
	float	CoreFactor  = pow(LightningMask, 2.0);
	float3	FinalColor  = lerp(GlowDiffuse, CoreDiffuse, CoreFactor) * ActiveMask;
	
	Out.vDiffuse = float4(FinalColor, ActiveMask);
	return Out;
}

PS_OUT PSMain_GlowLightning(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;
	
	float	Ratio = saturate(In.life / max(In.maxLife, 0.0001f)); // 0.f -> 1.f
	
	float	BaseMask = Get_BrightnessMask(g_DiffuseTexture, In.vTexcoord);
	
	float	EdgeFactor = 1.f - smoothstep(0.15f, 0.8f, BaseMask);
	EdgeFactor = pow(saturate(EdgeFactor), 1.5f);
	
	float2	NoiseTexCoordA = In.vTexcoord * float2(2.f, 7.f)  + float2(0.2f, -1.5f) * In.life;
	float2	NoiseTexCoordB = In.vTexcoord * float2(4.f, 11.f) + float2(-0.15f, 1.f) * In.life;
	
	float	NoiseA = g_NoiseTexture.Sample(LinearWrap, NoiseTexCoordA).r;
	float	NoiseB = g_NoiseTexture.Sample(LinearWrap, NoiseTexCoordB).g;

	float2	SignedNoise = float2(NoiseA, NoiseB) * 2.f - 1.f;
	
	float2	DistortedUV = In.vTexcoord + SignedNoise * DistortionStrength * EdgeFactor;

	float	WhiteMask = Get_BrightnessMask(g_DiffuseTexture, DistortedUV);
	float	TotalMask = smoothstep(0.02f, 0.45f, WhiteMask);
	float	CoreMask  = smoothstep(0.50f, 0.95f, WhiteMask);
	float	GlowMask  = saturate(TotalMask - CoreMask);
	
	float2	CombinedNoise = saturate((NoiseA + NoiseB) * 0.5f);
	
	float	EdgeNoiseValue = smoothstep(0.35f, 0.65f, CombinedNoise);
	float	GradatedNoise = smoothstep(1.f, EdgeNoiseValue, EdgeFactor);
	
	float	EdgeAlpha = lerp(1.f, 0.95f, EdgeFactor);
	float	Alpha = (GradatedNoise * TotalMask) * EdgeAlpha * In.vColor.a / 2.f;
	if (Alpha < 0.01f) discard;
	
	float	CoreBrightness = 1.f, GlowBrightness = 5.f;
	
	float3	CoreDiffuse = In.vColor.rgb * CoreBrightness;
	float3	GlowDiffuse = lerp(In.vEmissive, In.vEndEmissive, Ratio).rgb * GlowBrightness;

	float3 Emissive = CoreDiffuse * CoreMask + GlowDiffuse * GlowMask;
	
	Out.vDiffuse = float4(Emissive, Alpha);
	return Out;
}
