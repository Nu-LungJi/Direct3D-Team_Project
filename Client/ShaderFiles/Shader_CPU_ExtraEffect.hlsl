#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"


struct VS_IN
{
	float3 vPosition : POSITION;
	float2 vTexcoord : TEXCOORD0;
	float4 vWorld0 : INSTANCE_WORLD0;
	float4 vWorld1 : INSTANCE_WORLD1;
	float4 vWorld2 : INSTANCE_WORLD2;
	float4 vWorld3 : INSTANCE_WORLD3;
	float4 vColor  : INSTANCE_COLOR0;
	float4 vInstEmissive : INSTANCE_EMISSIVE;
	float4 vInstEndEmissive : INSTANCE_EMISSIVE1;
	float4 vInstOriginalEmissive : INSTANCE_EMISSIVE2;
	float2 uvOffset : INSTANCE_UVOFFSET;
	float2 uvSize : INSTANCE_UVSIZE;
	float life : INSTANCE_LIFE; // 추가 
	float maxLife : INSTANCE_MAXLIFE; // 추가
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
    

VS_OUT VSMain(VS_IN In)
{
	VS_OUT Out = (VS_OUT) 0;
	
	float4x4 matWorld = float4x4(In.vWorld0, In.vWorld1, In.vWorld2, In.vWorld3);
	
	float3 vCenter = float3(matWorld._41, matWorld._42, matWorld._43);
	float3 vRow0 = float3(matWorld._11, matWorld._12, matWorld._13);
	float fScale = length(vRow0);
	float3 vRight, vUp;
	vRight = normalize(float3(matWorld._11, matWorld._12, matWorld._13));
	vUp = normalize(float3(matWorld._21, matWorld._22, matWorld._23));

	float scaleX = length(float3(matWorld._11, matWorld._12, matWorld._13));
	float scaleY = length(float3(matWorld._21, matWorld._22, matWorld._23));

	float3 vWorldPos =
    vCenter +
    vRight * In.vPosition.x * scaleX +
    vUp * In.vPosition.y * scaleY;
	
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

Texture2D g_DiffuseTexture		: register(t1);
Texture2D g_NormalTexture		: register(t2);
Texture2D g_DistortionTexture	: register(t3);
Texture2D g_NoiseTexture		: register(t4);
Texture2D g_AnyTexture			: register(t5);
Texture2D g_BackgroundTex		: register(t7);

struct PS_OUT
{
	float4 vDiffuse : SV_TARGET0;
};

PS_OUT PSMain_StarRail(VS_OUT IN)
{
	PS_OUT OUT;
	
	float2 CenterTexCoord = IN.vTexcoord - 0.5f;
	
	float3 ChromaSample = g_DiffuseTexture.Sample(LinearClamp, IN.vTexcoord).rgb;
	float4 MaskSample = g_AnyTexture.Sample(LinearClamp, IN.vTexcoord);
    
	MaskSample.rgb *= 2.f;
	
	//float4  MaskSampleB	 = g_AnyTexture.Sample(LinearWrap, IN.vTexcoord);

	float	Ratio = saturate(1.f - (IN.life / max(IN.maxLife, 0.0001f)));
	
	float4	Emissive = lerp(IN.vEmissive, IN.vEndEmissive, Ratio);
	
	float	ColorIntensity = 4.f;
	
	float	Mask = pow(saturate(MaskSample.a), 1.f);
	float3	ColoredLight = ChromaSample * Mask * ColorIntensity;
	
	OUT.vDiffuse = float4(ColoredLight.rgb + Emissive.rgb * Emissive.a, MaskSample.a);
	return OUT;

	float	DistanceFromCenter = length(CenterTexCoord);
	
	float	WhiteCoreSize = 180.f;
	float	WhiteCoreIntensity = 1.f;
	float	WhiteCoreMask = exp2(-DistanceFromCenter * DistanceFromCenter * max(WhiteCoreSize, 0.001f));
	
	float3	FinalColor = ColoredLight + WhiteCoreMask * WhiteCoreIntensity;
	
	OUT.vDiffuse = float4(ChromaSample.rgb + Emissive.rgb * Emissive.a, MaskSample.a);
	return OUT;
}
