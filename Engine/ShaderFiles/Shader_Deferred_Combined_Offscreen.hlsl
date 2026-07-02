#include "./ShaderHeader/SH_SamplerState.hlsli"
#include "./ShaderDefines.hlsl"

Texture2D g_DiffuseTexture  : register(t0);
Texture2D g_NormalTexture   : register(t1);
Texture2D g_ShadowMap       : register(t4);

struct VS_IN
{
    float3 vPosition : POSITION;    
    float2 vTexcoord : TEXCOORD0;    
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
};

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out;
    
    Out.vPosition = mul(vector(In.vPosition, 1.f), mul(mul(g_matWorld, g_matView), g_matProj));
    Out.vTexcoord = In.vTexcoord;
    
    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
};

struct PS_OUT_BACKBUFFER
{
    vector vBackBuffer : SV_TARGET0;
};

struct PS_OUT_LIGHT
{
    vector vShade : SV_TARGET0;
    vector vSpecular : SV_TARGET1;
};


PS_OUT_BACKBUFFER PS_MAIN_DEBUG(PS_IN In)
{
    PS_OUT_BACKBUFFER Out;
    
    Out.vBackBuffer = g_DiffuseTexture.Sample(, In.vTexcoord);
    
    return Out;
}


PS_OUT_BACKBUFFER PSMain(PS_IN In)
{
    PS_OUT_BACKBUFFER Out;
    
    vector vDiffuse = g_DiffuseTexture.Sample(SamplerWrap, In.vTexcoord);
    if (0.f == vDiffuse.a)
        discard;
    //vector vShade = g_ShadeTexture.Sample(LinearSampler, In.vTexcoord);
    
   // vector vSpecular = g_SpecularTexture.Sample(LinearSampler, In.vTexcoord);
    
    Out.vBackBuffer = vDiffuse /** vShade + vSpecular*/;
    
    //vector vDepthDesc = g_DepthTexture.Sample(LinearSampler, In.vTexcoord);
    
    //vector vWorldPos;

    //vWorldPos.x = In.vTexcoord.x * 2.f - 1.f;
    //vWorldPos.y = In.vTexcoord.y * -2.f + 1.f;
    //vWorldPos.z = vDepthDesc.x;
    //vWorldPos.w = 1.f;
    
      /* 뷰스페이스 상의 위치 */ 
    //float fViewSpaceZ = vDepthDesc.y * 1000.f;
    //vWorldPos = vWorldPos * fViewSpaceZ;
    //vWorldPos = mul(vWorldPos, g_ProjMatrixInverse);
    //vWorldPos = mul(vWorldPos, g_ViewMatrixInverse);
    
    //vWorldPos = mul(vWorldPos, g_LightViewMatrix);
    //vWorldPos = mul(vWorldPos, g_LightProjMatrix);
    
    //float2 vTexcoord;
    /* -1 -> 0, 1 -> 1 */
    //vTexcoord.x = (vWorldPos.x / vWorldPos.w) * 0.5f + 0.5f;
    
    /* 1 -> 0, -1 -> 1 */
    //vTexcoord.y = (vWorldPos.y / vWorldPos.w) * -0.5f + 0.5f;
    
    //float fOldZ = g_LightDepthTexture.Sample(LinearSampler, vTexcoord).x * 1000.f;
    
    
    
    
    
    //if (vWorldPos.w - 0.1f > fOldZ)
     //   Out.vBackBuffer *= 0.3f;
    
    return Out;

}