#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"


cbuffer CB_BEAM : register(b11)
{
    float g_fLifeRatio;
	float g_fAccumulationTime;
    float2 _pad;
};

struct VS_IN
{
    float3 vPosition : POSITION;
    float2 vUV : TEXCOORD0;
    float4 vColor : COLOR0;
	float4 vInstEmissive : COLOR1;
	float4 vInstEndEmissive : COLOR2;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vUV : TEXCOORD0;
    float4 vColor : COLOR0;
    float4 vEmissive : COLOR1;
    float4 vEndEmissive : COLOR2;
};

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;
    Out.vPosition = mul(float4(In.vPosition, 1.f), g_matViewProj);
    Out.vUV = In.vUV ;
    Out.vColor = In.vColor;
    Out.vEmissive = In.vInstEmissive;
    return Out;
}

Texture2D g_BeamTexture : register(t1);
Texture2D g_NormalTexture : register(t2);
Texture2D g_DistortionTexture : register(t3);
Texture2D g_NoiseTexture : register(t4);
Texture2D g_AnyTexture : register(t5);
Texture2D g_BackgroundTex : register(t7);
struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;

};

PS_OUT PSMain(VS_OUT In)
{


    PS_OUT Out = (PS_OUT) 0;
    float4 texColor = g_BeamTexture.Sample(LinearWrap, In.vUV) * In.vColor;
    
    if (texColor.a <= 0.01f)
        discard;

    if (all(texColor.rgb < 0.1f))
        discard;
    float3 instEmissive = In.vEmissive.rgb * In.vEmissive.w;
    float3 FinalColor = texColor.rgb * instEmissive;

    Out.vDiffuse = float4(FinalColor, texColor.a);
    
    
    return Out;
} 
PS_OUT PSAccio(VS_OUT In)
{
	PS_OUT Out = (PS_OUT)0;

	float gradient = g_BeamTexture.Sample(LinearClamp,In.vUV).r;
	float gradientMask = pow(saturate(gradient),0.25f);
	float alpha = gradientMask * In.vColor.a;

	clip(alpha - 0.001f);

	float3 baseColor = In.vColor.rgb * gradientMask;
	float3 emissiveColor = In.vEmissive.rgb * In.vEmissive.a * gradientMask;

	Out.vDiffuse = float4(baseColor + emissiveColor,alpha);
	return Out;
}
