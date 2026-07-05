#include "../ShaderDefines.hlsl"

/*
float4x4 g_WorldMatrix, g_ViewMatrix, g_ProjMatrix;
texture2D g_DiffuseTexture;
texture2D g_NormalTexture;
*/
Texture2D g_DiffuseTexture : register(t1);
SamplerState LinearSampler : register(s0);

/*
sampler LinearSampler = sampler_state
{    
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};
*/

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;    
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float4 vTangent : TANGENT;
    float4 vBinormal : BINORMAL;
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
    Out.vTangent = normalize(mul(float4(In.vTangent, 0.f), g_matWorld));
    Out.vBinormal = normalize(mul(float4(In.vBinormal, 0.f), g_matWorld));
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), g_matWorld);
    Out.vProjPos = Out.vPosition;
    return Out;
}

struct PS_IN
{
    float4 vPosition    : SV_POSITION;
    float4 vNormal      : NORMAL;
    float4 vTangent     : TANGENT;
    float4 vBinormal    : BINORMAL;
    float2 vTexcoord    : TEXCOORD0;
    float4 vWorldPos    : TEXCOORD1;
    float4 vProjPos     : TEXCOORD2;
};

struct PS_OUT
{
    vector vDiffuse     : SV_TARGET0;
    vector vNormal      : SV_TARGET1;
    vector vDepth       : SV_TARGET2;
    vector vPickPos     : SV_TARGET3;
};

PS_OUT PSMain(PS_IN In)
{
    PS_OUT Out;
    
    vector      vMtrlDiffuse = g_DiffuseTexture.Sample(LinearSampler, In.vTexcoord);
    
    if (vMtrlDiffuse.a <= 0.3f)
        discard;
    
   // vector      vNormalDesc = g_NormalTexture.Sample(LinearSampler, In.vTexcoord);
    //float3 vNormal = vNormalDesc.xyz * 2.0f - 1.f;
    
   // float3x3 WorldMatrix = float3x3(In.vTangent.xyz, In.vBinormal.xyz * -1.f, In.vNormal.xyz);
    
    //vNormal = normalize(mul(vNormal, WorldMatrix));
    
    
    Out.vDiffuse = vMtrlDiffuse;
    //Out.vNormal = vector(vNormal.xyz * 0.5f + 0.5f, 0.f);
    //Out.vDepth = vector(In.vProjPos.z / In.vProjPos.w, In.vProjPos.w / 1000.f, 0.f, 0.f);
   // Out.vPickPos = vector(In.vWorldPos.xyz, 1.f);
    
    return Out;
}

/*
technique11 DefaultTechnique
{
    pass DefaultPass
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }  

    pass DefaultPass1
    {
        SetRasterizerState(RS_Default);
        SetDepthStencilState(DSS_Default, 0);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 0.f), 0xffffffff);
        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN();
    }
}


*/
