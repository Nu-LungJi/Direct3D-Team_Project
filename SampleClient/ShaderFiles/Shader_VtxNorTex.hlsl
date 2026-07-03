#include "../../Engine/ShaderFiles/ShaderDefines.hlsl"

Texture2D gDiffuseTexture : register(t0);
SamplerState gSamLinearWrap : register(s0);

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
};

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out;
    
    float4x4 matWV, matWVP;
    
    matWV = mul(g_matWorld, g_matView);
    matWVP = mul(matWV, g_matProj);
    
    Out.vPosition = mul(float4(In.vPosition, 1.f), matWVP);
    Out.vNormal = normalize(mul(float4(In.vNormal, 0.f), g_matWorld));
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), g_matWorld);
    Out.vProjPos = Out.vPosition;
    //Out.vNormal = float4(In.vNormal, 1.f);
    return Out;
}

/* 투영변환 -> W나누기 */ 
/* 뷰포트로 변환해준다 */ 
/* 래스터라이즈 : 픽셀의 정보가 생성된다. */ 
struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
};

struct PS_OUT
{
    vector vDiffuse : SV_TARGET0;
    vector vNormal : SV_TARGET1;
//    vector vDepth : SV_TARGET2;
//    vector vPickPos : SV_TARGET3;
};


PS_OUT PSMain(PS_IN In)
{
    PS_OUT Out;
    
    vector vMtrlDiffuse = gDiffuseTexture.Sample(gSamLinearWrap, In.vTexcoord * 50.f);
    
    Out.vDiffuse = vMtrlDiffuse;
    //Out.vNormal = vector(In.vNormal.xyz * 0.5f + 0.5f, 0.f);
    //Out.vNormal = vector(In.vNormal.xyz , 1.f);
    Out.vNormal = vector(In.vNormal.xyz * 0.5f + 0.5f, 0.f);
    //Out.vNormal = float4(1, 0, 0, 1);;
    //Out.vDepth = vector(In.vProjPos.z / In.vProjPos.w, In.vProjPos.w / 1000.f, 0.f, 0.f);
    //Out.vPickPos = vector(In.vWorldPos.xyz, 1.f);
    
    return Out;
}
