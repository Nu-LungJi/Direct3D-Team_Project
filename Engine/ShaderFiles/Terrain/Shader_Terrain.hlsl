#include "../ShaderDefines.hlsl"

Texture2D g_TileTextures[4] : register(t0);
Texture2D g_BlendMask : register(t4); // TerrainChunk의 개별 BlendMask

cbuffer CB_TERRAIN_CHUNK : register(b11)
{
	// 전체 Terrain UV에서 청크가 시작하는 위치
    float2 g_ChunkUVOffset;
	// 전체 Terrain UV에서 청크가 차지하는 크기
    float2 g_ChunkUVSpan;
};

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
    Out.vPosition = mul(float4(In.vPosition, 1.f), g_matWVP);
    Out.vNormal = normalize(mul(float4(In.vNormal, 0.f), g_matWorld));
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), g_matWorld);
    Out.vProjPos = Out.vPosition;
    return Out;
}

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
    vector vSMRO : SV_TARGET2;
    vector vEmissive : SV_TARGET3;
};

PS_OUT PSMain(PS_IN In)
{
    PS_OUT Out;
	
	// 전체 Terrain UV를 현재 청크의 0~1 Blend Mask UV로 변환
	
	// 청크의 Blend Mask는 청크 내부를 0~1로 표현
	// 하지만 정점 UV는 전체 Terrain 기준이므로 변환이 필요
    float2 maskUV = saturate((In.vTexcoord - g_ChunkUVOffset) / g_ChunkUVSpan);
    float4 weights = saturate(g_BlendMask.Sample(LinearClamp, maskUV));
    weights /= max(dot(weights, 1.f), 0.0001f);
    vector materialDiffuse =
        g_TileTextures[0].Sample(LinearWrap, In.vTexcoord * 50.f) * weights.r +
        g_TileTextures[1].Sample(LinearWrap, In.vTexcoord * 50.f) * weights.g +
        g_TileTextures[2].Sample(LinearWrap, In.vTexcoord * 50.f) * weights.b +
        g_TileTextures[3].Sample(LinearWrap, In.vTexcoord * 50.f) * weights.a;
    Out.vDiffuse = materialDiffuse;
    Out.vNormal = float4(In.vNormal.xyz * 0.5f + 0.5f, 0.f);
    Out.vSMRO = float4(0.f, 1.f, 1.f, 1.f);
    Out.vEmissive = float4(0.f, 0.f, 0.f, 1.f);
    return Out;
}
