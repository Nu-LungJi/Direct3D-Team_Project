#include "../../Engine/ShaderFiles/ShaderDefines.hlsl"

struct ParticleData
{
    float3 position;
    float pad1;
    float3 velocity;
    float life;
    float maxLife;
    float size;
    float startSize;
    uint alive;
    uint loop;
    float4 color;
    float4 emissive;
    uint frameIndex;
    float3 pad2;
};

cbuffer CB_PER_PARTICLE : register(b5)
{
    float g_fTimeDelta;
    uint g_iNumInstances;
    uint g_iBehaviorType;
    uint g_iFlipbookRows;
    uint g_iFlipbookColumns;
    uint g_iTotalFrames;
    float g_fPadding;
    float g_fPadding2;
};

StructuredBuffer<ParticleData> g_RenderBuffer : register(t0);
Texture2D g_Texture : register(t1);

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;
    float4 vColor : COLOR0;
    float4 vEmissive : COLOR1;
};

VS_OUT VSMain(uint vID : SV_VertexID, uint instID : SV_InstanceID)
{
    VS_OUT Out = (VS_OUT) 0;

    ParticleData p = g_RenderBuffer[instID];

    if (!p.alive)
    {
        p.color.a = 0.f;
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

    float3 vLocalPos = float3((baseUV.x - 0.5f) * p.size, (baseUV.y - 0.5f) * p.size, 0.0f);
    float4 vWorldPos = float4(vLocalPos + p.position, 1.0f);

    float4 vViewPos = mul(vWorldPos, g_matView);
    Out.vPosition = mul(vViewPos, g_matProj);

    Out.vColor = p.color;
    Out.vEmissive = p.emissive;

    return Out;
}

struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;
};

PS_OUT PSMain(VS_OUT In)
{
    PS_OUT Out = (PS_OUT) 0;

    float4 vTextureColor = g_Texture.Sample(LinearWrap, In.vTexcoord);


    //if (vTextureColor.x < 0.f)
    //    discard;

    float4 vFinalColor = vTextureColor * In.vColor;

    Out.vDiffuse = float4(vFinalColor.xyz + In.vEmissive.xyz * In.vEmissive.w, vFinalColor.a);
    return Out;
}
