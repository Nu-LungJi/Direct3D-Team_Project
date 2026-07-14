#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"


cbuffer CB_BEAM : register(b0)
{
    float g_fScrollOffset;
    float3 _pad;
};

struct VS_IN
{
    float3 vPosition : POSITION;
    float2 vUV : TEXCOORD0;
    float4 vColor : COLOR0;
    float4 vInstEmissive : EMISSIVE;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vUV : TEXCOORD0;
    float4 vColor : COLOR0;
    float4 vEmissive : COLOR1;
};

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;
    Out.vPosition = mul(float4(In.vPosition, 1.f), g_matViewProj);
    Out.vUV = In.vUV + float2(g_fScrollOffset, 0.f);
    Out.vColor = In.vColor;
    Out.vEmissive = In.vInstEmissive;
    return Out;
}

Texture2D g_BeamTexture : register(t0);

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
    if (texColor.x < 0.2f)
        discard;
    float3 instEmissive = In.vEmissive.rgb * In.vEmissive.w;
    float3 FinalColor = texColor.rgb + instEmissive;

    Out.vDiffuse = float4(FinalColor, texColor.a);
    
    
    return Out;
}
