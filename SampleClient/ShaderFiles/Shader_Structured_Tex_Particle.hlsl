#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"




cbuffer CB_PER_PARTICLE : register(b5)
{
    float g_fTimeDelta;
    uint g_iNumInstances;
    uint g_iFlipbookRows;
    uint g_iFlipbookColumns; 
    uint g_iTotalFrames;
    float3 g_fPadding;
};

StructuredBuffer<ParticleData> g_RenderBuffer : register(t0);
Texture2D g_Texture : register(t1);
Texture2D g_BackgroundTex : register(t7);
struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float4 vColor : COLOR0;
    float4 vEmissive : COLOR1;
    float4 vScreenPos : TEXCOORD1;
    uint  iBehaviorType : TEXCOORD2;
};

VS_OUT VSMain(uint vID : SV_VertexID, uint instID : SV_InstanceID)
{
    VS_OUT Out = (VS_OUT) 0;

    ParticleData p = g_RenderBuffer[instID];

    if (!p.alive)
    {
        Out.vColor = 0;
    }

    // 기존: 쿼드 전체(0~1)를 그대로 쓰던 UV
    float2 baseUV = float2(vID % 2, 1 - (vID / 2));

    // ---- 플립북 UV 계산 (추가) ----
    float2 finalUV = baseUV;

    if (g_iTotalFrames > 1 && g_iFlipbookColumns > 0 && g_iFlipbookRows > 0)
    {
        uint frame = min(p.frameIndex, g_iTotalFrames - 1);
        uint col = frame % g_iFlipbookColumns;
        uint row = frame / g_iFlipbookColumns;
        float2 uvSize = float2(1.0f / g_iFlipbookColumns, 1.0f / g_iFlipbookRows);
        float2 uvOffset = float2(col, row) * uvSize;

        finalUV = uvOffset + baseUV * uvSize;
    }

    Out.vTexcoord = finalUV;

    //float3 vLocalPos = float3((baseUV.x - 0.5f) * p.size, (baseUV.y - 0.5f) * p.size, 0.0f);
    //float4 vWorldPos = float4(vLocalPos + p.position, 1.0f);
    //
    //float4 vViewPos = mul(vWorldPos, g_matView);
    //Out.vPosition = mul(vViewPos, g_matProj);

    float3 camRight = g_matInvView[0].xyz;
    float3 camUp = g_matInvView[1].xyz;

    float3 local = float3((baseUV - 0.5f) * p.size, 0);
    
    float4 vWorldPos;

    if ((p.iBehaviorType & BEHAVIOR_BILLBOARD) != 0)
    {
        float3 worldPos =
        p.position +
        camRight * local.x +
        camUp * local.y;
        vWorldPos = float4(worldPos, 1.0f);
    }
    else
    {
        float3 rotatedLocal = RotateXYZ(local, p.rotation); // p.rotation 필요
    
        float3 worldPos = p.position + rotatedLocal;
        vWorldPos = float4(worldPos, 1.0f);
    }
  
    float4 vViewPos = mul(vWorldPos, g_matView);
    Out.vPosition = mul(vViewPos, g_matProj);
    Out.vScreenPos = Out.vPosition;
    Out.vColor = p.color;
    Out.vEmissive = p.emissive;
    
    Out.iBehaviorType = p.iBehaviorType;
    return Out;
}

struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;
};

PS_OUT PSMain(VS_OUT In)
{
    PS_OUT Out = (PS_OUT) 0;
  
    if (all(In.vColor <= 0.0f))
    {
        Out.vDiffuse = 0;
        return Out;
    }
    float4 vTextureColor = g_Texture.Sample(LinearWrap, In.vTexcoord);
    if ((In.iBehaviorType & BEHAVIOR_DISTORTION) != 0)
    {
        clip(In.vColor.a - 0.02f);
        
        float2 screenUV = In.vScreenPos.xy / In.vScreenPos.w;
        screenUV.x = screenUV.x * 0.5f + 0.5f;
        screenUV.y = -screenUV.y * 0.5f + 0.5f;

        float2 distortion = vTextureColor.rg * 2.0f - 1.0f;
        float distortionStrength = 0.03f * In.vColor.a;
        distortion *= distortionStrength;

        float4 distortedBackground = g_BackgroundTex.Sample(LinearWrap, screenUV + distortion);
        Out.vDiffuse = distortedBackground;
        return Out;
    }
 
    float4 vFinalColor = vTextureColor * In.vColor;
    clip(vFinalColor.a - 0.02f);
    Out.vDiffuse = float4(vFinalColor.xyz + In.vEmissive.xyz * In.vEmissive.w, vFinalColor.a);
    
    return Out;
}
