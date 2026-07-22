#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"
#define MAX_LIGHT_COUNT     8
#define LIGHT_DIRECTIONAL   0
#define LIGHT_POINT         1
#define LIGHT_SPOTLIGHT     2

cbuffer CB_PER_PARTICLE : register(b11)
{
    float g_fTimeDelta;
    uint g_iNumInstances;
    uint g_iFlipbookRows;
    uint g_iFlipbookColumns;
    uint g_iTotalFrames;
    float g_fTime;
    float2 g_fPadding;
};

StructuredBuffer<ParticleData> g_RenderBuffer : register(t4);

//픽셀 쉐이더용
Texture2D AlbedoMap : register(t0);
Texture2D NormalMap : register(t1);
Texture2D SMROMap : register(t2);
Texture2D EmissiveMap : register(t3);
Texture2D NoiseMap : register(t5);



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
    float2 vTexcoord : TEXCOORD0;
    float4 vColor : COLOR0;
    float3 vNormal : NORMAL0;
    float3 vTangent : TANGENT0;
    float3 vBinormal : BINORMAL0;
    float4 vEmissive : EMISSIVE0;
    float4 vEndEmissive : EMISSIVE1;
    float3 vWorldPos : TEXCOORD1; // 추가: 라이팅 계산에 필요
    float life : TEXCOORD2;
    float maxLife : TEXCOORD3;
    float3 vLocalPos : TEXCOORD4;
};

VS_OUT VSMain(VS_IN In, uint instID : SV_InstanceID)
{
    VS_OUT Out = (VS_OUT) 0;
    ParticleData p = g_RenderBuffer[instID];
    float2 finalUV = In.vTexcoord;
	float3 scale = p.alive ? p.size : float3(0.0f, 0.0f, 0.0f);
    
    if (g_iTotalFrames > 1 && g_iFlipbookColumns > 0 && g_iFlipbookRows > 0)
    {
        uint frame = min(p.frameIndex, g_iTotalFrames - 1);
        uint col = frame % g_iFlipbookColumns;
        uint row = frame / g_iFlipbookColumns;
        float2 uvSize = float2(1.0f / g_iFlipbookColumns, 1.0f / g_iFlipbookRows);
        float2 uvOffset = float2(col, row) * uvSize;

        finalUV = uvOffset + In.vTexcoord * uvSize; // baseUV 대신 실제 메쉬 UV 사용
    }

    Out.vTexcoord = finalUV;
    Out.vLocalPos = In.vPosition;

    float3 localPos = In.vPosition * scale; 
    float3 rotatedLocal = RotateXYZ(localPos, p.rotation); 
    float3 vWorldPos = rotatedLocal + p.position;


    Out.vPosition = mul(float4(vWorldPos, 1.0f), g_matViewProj);
    Out.vWorldPos = vWorldPos;
    //Out.vTexcoord = In.vTexcoord;
    Out.vNormal = In.vNormal;
    Out.vTangent = In.vTangent;
    Out.vBinormal = In.vBinormal;
    Out.vColor = p.alive ? p.color : float4(p.color.rgb, 0.0f);
    Out.vEmissive = p.emissive;
    Out.vEndEmissive = p.endEmissive;
    Out.life = p.life;
    Out.maxLife = p.maxLife;
    
    return Out;
}

struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;
};


PS_OUT PSMain(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;
 
	float2 distortionUV = In.vTexcoord * float2(2.f, 1.f);
	distortionUV.y += g_fTime * 0.04f;

	float2 cloudUV = In.vTexcoord * float2(8.f, 1.f);
	cloudUV.y += g_fTime * 0.6f;

	float2 swirlUV = In.vTexcoord * float2(5.f, 1.f);
	swirlUV.x -= g_fTime * 0.06f;

	float2 distortion = NoiseMap.Sample(LinearWrap, distortionUV).rg * 2.f - 1.f;

	cloudUV += distortion * 0.04f;
	swirlUV += distortion * 0.04f;

	float4 cloud = AlbedoMap.Sample(LinearWrap, cloudUV);
	float3 swirl = NormalMap.Sample(LinearWrap, swirlUV).rgb;
	
	float cloudShape = smoothstep(0.2f, 0.5f, cloud.a);
	float topMask = 1.f - smoothstep(0.f, 0.5f, In.vTexcoord.y);
	
	float shapeMask = lerp(1.f, cloudShape, topMask);
	if(shapeMask <0.3f)
		discard;
	float3 pattern = cloud.rgb + swirl.rgb * 0.5f;
	float4 final = float4(pattern * float3(0.3f, 0.5f, 0.8f) * shapeMask, 1.f);

	Out.vDiffuse = final;
	return Out;
}
