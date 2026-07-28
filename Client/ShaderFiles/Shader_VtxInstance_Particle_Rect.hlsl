#include "Engine_Shader_Defines.hlsli"


float4x4 g_WorldMatrix, g_ViewMatrix, g_ProjMatrix;
texture2D g_Texture;

sampler LinearSampler = sampler_state
{    
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

struct VS_IN
{
    float3 vPosition : POSITION;
    float2 vTexcoord : TEXCOORD0;
    
    float4 vRight : TEXCOORD1;
    float4 vUp : TEXCOORD2;
    float4 vLook : TEXCOORD3;
    float4 vTranslation : TEXCOORD4;
    float2 vLifeTime : TEXCOORD5;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float2 vLifeTime : TEXCOORD1;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;    
    
    
    float4x4 LocalMatrix = float4x4(In.vRight, In.vUp, In.vLook, In.vTranslation);
    
    vector vPosition = mul(float4(In.vPosition, 1.f), LocalMatrix);
    
    
    float4x4 matWV, matWVP;
    
    matWV = mul(g_WorldMatrix, g_ViewMatrix);
    matWVP = mul(matWV, g_ProjMatrix);
    
    Out.vPosition = mul(vPosition, matWVP);
    Out.vTexcoord = In.vTexcoord;
    Out.vLifeTime = In.vLifeTime;
    
    return Out;
}

/* 투영변환 -> W나누기 */ 
/* 뷰포트로 변환해준다 */ 
/* 래스터라이즈 : 픽셀의 정보가 생성된다. */ 
struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float2 vLifeTime : TEXCOORD1;
};

struct PS_OUT
{
    vector vColor : SV_TARGET0;
};


PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;
    
    if (In.vLifeTime.y >= In.vLifeTime.x)
        discard;
    
    Out.vColor = g_Texture.Sample(LinearSampler, In.vTexcoord);
    
    if(Out.vColor.a < 0.1f)
        discard;
    
    return Out;
}

technique11 DefaultTechnique
{
    pass DefaultPass
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN();
    }  
}

